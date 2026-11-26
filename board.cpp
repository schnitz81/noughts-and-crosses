#include "board.h"

#include <algorithm>
#include <ctime>
#include <cstdlib>

Game::Game()
{
    m_board.assign((BOARD_SIZE + 2 * BOARD_SAFETY + 1) *
                   (BOARD_SIZE + 2 * BOARD_SAFETY) * 2, 0);
    initAlgorithmNewGame();
    updateGridSize();
}

Game::~Game() = default;

void Game::initAlgorithmNewGame()
{
    for (int x = 0; x < (BOARD_SIZE + 2 * BOARD_SAFETY); x++)
        for (int y = 0; y < (BOARD_SIZE + 2 * BOARD_SAFETY); y++)
            cellRef(x, y, 0) = cellRef(x, y, 1) = 0;

    m_sAlreadyFoundRows[0] = 0;
    m_sAlreadyFoundRows[1] = 0;

    for (int i = 0; i < 16; i++)
        for (int p = 0; p < 2; p++)
            m_ptAlreadyFoundRowEnds[i][p][0] = m_ptAlreadyFoundRowEnds[i][p][1] = QPoint(0, 0);

    m_rectUsedLeft = m_rectBoardLeft;
    m_rectUsedRight = m_rectBoardLeft;
    m_rectUsedTop = m_rectBoardTop;
    m_rectUsedBottom = m_rectBoardTop;

    m_sGameCount = 0;
    m_bGameGoing = false;
    m_bUsersTurn = true;
    // m_bCurrentGameStartedByComputer is intentionally preserved here so the
    // "Alternating" first-move option keeps toggling between games.
    m_flashing.clear();
    m_cancelRequested = false;
}

void Game::updateGridSize()
{
    if (m_sGridSize >= 32)
        m_sGridLineWidth = 2;
    else
        m_sGridLineWidth = 1;
    m_sGridBitmapSize = m_sGridSize - m_sGridLineWidth;
    m_sBoardBorderLineWidth = m_sGridSize / 4;

    m_crossLineWidth = std::max(3, m_sGridSize / 4);
    m_noughtLineWidth = std::max(4, m_sGridSize / 4);
    m_circleInset = m_sGridBitmapSize / 16 - (m_sGridBitmapSize < 8 ? 1 : 0);
}

void Game::translateBoardSkillLevel()
{
    updateValueArray();

    for (int x = m_rectUsedLeft; x <= m_rectUsedRight; x++)
        for (int y = m_rectUsedTop; y <= m_rectUsedBottom; y++) {
            if (!bEmptySpec(x, y))
                continue;

            short NewValue[2] = {0, 0};

            bool Open[8][4];
            QPoint OpenPos[8][4];
            short N[8][4];
            short NotPiece[8];
            findNeighbours(x, y, Open, OpenPos, N, NotPiece);

            for (int dir = 0; dir < 8; dir = dir + 2) {
                for (int tempPiece = 0; tempPiece < 2; tempPiece++) {
                    short Change1 = 0;
                    countValue(
                        static_cast<short>(N[dir + 1][1] * (NotPiece[dir + 1] != tempPiece)),
                        static_cast<short>(N[dir + 1][0] * (NotPiece[dir + 1] != tempPiece)),
                        static_cast<short>(N[dir][0] * (NotPiece[dir] != tempPiece)),
                        static_cast<short>(N[dir][1] * (NotPiece[dir] != tempPiece)),
                        Open[dir + 1][1] && (NotPiece[dir + 1] != tempPiece),
                        Open[dir + 1][0] && ((N[dir + 1][0] == 0) || (NotPiece[dir + 1] != tempPiece)),
                        Open[dir][0] && ((N[dir][0] == 0) || (NotPiece[dir] != tempPiece)),
                        Open[dir][1] && (NotPiece[dir] != tempPiece),
                        &Change1);
                    NewValue[tempPiece] = static_cast<short>(NewValue[tempPiece] + Change1);
                }
            }

            cellRef(x, y, 0) = NewValue[0];
            cellRef(x, y, 1) = NewValue[1];
        }
}

short Game::nearValueFunction(short sLength, bool open)
{
    if (m_sSkillLevel <= 1) {
        if (sLength == 1) return 0x1;
        if (sLength == 2) return 0x10;
        if (sLength == 3) return 0x100;
        if (sLength >= 4) return 0x1000;
    } else {
        if (sLength == 1) return open ? 9 : 1;
        if (sLength == 2) return open ? 0xA1 : 0x21;
        if (sLength == 3) return open ? 0x401 : 0xD1;
        return open ? 0x1101 : 0x1001;
    }
    return 0;
}

short Game::farValueFunction(short sLength, bool open)
{
    if (m_sSkillLevel <= 1) return 0;
    if (sLength == 1) return open ? 8 : 1;
    if (sLength == 2) return open ? 0xA0 : 0x20;
    return open ? 0xD1 : 0xD0;
}

void Game::updateValueArray()
{
    for (short RowLength = 0; RowLength < 16; RowLength++)
        for (int AreBothEndsOpen = 0; AreBothEndsOpen < 2; AreBothEndsOpen++) {
            m_NearValue[RowLength][AreBothEndsOpen] = nearValueFunction(RowLength, AreBothEndsOpen != 0);
            m_FarValue[RowLength][AreBothEndsOpen] = farValueFunction(RowLength, AreBothEndsOpen != 0);
        }
}

void Game::countValue(short NA, short NB, short NC, short ND,
                      bool OA, bool OB, bool OC, bool OD, short *sChange1)
{
    if (!OA && !OD && ((OB + OC + NA + NB + NC + ND) < 4))
        return;

    short a = 0, b = 0;
    short c = 0;
    bool d = false;
    auto idx = [](short v) -> int { return std::min(15, std::max(0, static_cast<int>(v))); };

    if (NB || NC) {
        if ((NB == 1) && (NC == 1) && (NA || ND)) {
            c = 3;
            d = (NA && ND) != 0;
        } else {
            c = static_cast<short>(NB + NC);
            d = (OB && OC) != 0;
        }
        a = m_NearValue[idx(c)][d ? 1 : 0];
    }

    if (NA && !NC) {
        b = m_FarValue[idx(static_cast<short>(NA + NB))][(OA && OC) ? 1 : 0];
        if (b > a) a = b;
    } else if (NA && NC && !NB && OB) {
        b = m_FarValue[idx(static_cast<short>(NA + NC))][(OA && OC) ? 1 : 0];
        if (b > a) a = b;
    }
    if (ND && !NB) {
        b = m_FarValue[idx(static_cast<short>(NC + ND))][(OB && OD) ? 1 : 0];
        if (b > a) a = b;
    } else if (NB && ND && !NC && OC) {
        b = m_FarValue[idx(static_cast<short>(NB + ND))][(OB && OD) ? 1 : 0];
        if (b > a) a = b;
    }

    *sChange1 = static_cast<short>(*sChange1 + a);
}

void Game::findNeighbours(int xM, int yM,
                          bool Open[8][4], QPoint OpenPos[8][4],
                          short N[8][4], short NotPiece[8]) const
{
    for (int dir = 0; dir < 8; dir += 2) {
        int xMul = 0, yMul = 0;
        switch (dir) {
        case 0: xMul = 0; yMul = 1; break;
        case 2: xMul = 1; yMul = 0; break;
        case 4: xMul = 1; yMul = 1; break;
        case 6: xMul = 1; yMul = -1; break;
        }
        for (int dir2 = 0; dir2 < 2; dir2++) {
            xMul *= -1;
            yMul *= -1;

            const int idx = dir + dir2;

            int curX = xM + xMul;
            int curY = yM + yMul;

            N[idx][0] = N[idx][1] = N[idx][2] = N[idx][3] = 0;
            Open[idx][0] = Open[idx][1] = Open[idx][2] = Open[idx][3] = false;
            NotPiece[idx] = -1;

            for (int i = 0; i < 4; i++) {
                short v0 = cell(curX, curY, 0);
                short v1 = cell(curX, curY, 1);

                if (mFFC(v0) == 0x8FFC) {
                    if (NotPiece[idx] == 0) break;
                    NotPiece[idx] = 1;
                    bool bStop = false;
                    do {
                        N[idx][i] = static_cast<short>(N[idx][i] + 1);
                        curX += xMul;
                        curY += yMul;
                        if (curX < m_rectBoardLeft || curX > m_rectBoardRight ||
                            curY < m_rectBoardTop || curY > m_rectBoardBottom) {
                            bStop = true;
                            break;
                        }
                        v0 = cell(curX, curY, 0);
                    } while (mFFC(v0) == 0x8FFC);
                    if (bStop) break;
                } else if (mFFC(v1) == 0x8FFC) {
                    if (NotPiece[idx] == 1) break;
                    NotPiece[idx] = 0;
                    bool bStop = false;
                    do {
                        N[idx][i] = static_cast<short>(N[idx][i] + 1);
                        curX += xMul;
                        curY += yMul;
                        if (curX < m_rectBoardLeft || curX > m_rectBoardRight ||
                            curY < m_rectBoardTop || curY > m_rectBoardBottom) {
                            bStop = true;
                            break;
                        }
                        v1 = cell(curX, curY, 1);
                    } while (mFFC(v1) == 0x8FFC);
                    if (bStop) break;
                }

                v0 = cell(curX, curY, 0);
                v1 = cell(curX, curY, 1);

                if (mFFC(v0) != 0x8FFC && mFFC(v1) != 0x8FFC) {
                    Open[idx][i] = true;
                    if (i < 2) {
                        OpenPos[idx][i].setX(curX);
                        OpenPos[idx][i].setY(curY);
                    }
                    curX += xMul;
                    curY += yMul;
                }
            }
        }
    }
}

void Game::putPiece(int x, int y, int newPiece, BoardChange *changed)
{
    bool Open[8][4];
    QPoint OpenPos[8][4];
    short N[8][4];
    short NotPiece[8];

    findNeighbours(x, y, Open, OpenPos, N, NotPiece);

    short sCount = 1; // Coord[0] reserved for the new square itself

    for (int dir = 0; dir < 8; dir = dir + 2) {
        bool bAlreadyChanged[4] = {false, false, false, false};
        for (int tempPiece = 0; tempPiece < 2; tempPiece++) {

            short sN0_0, sN0_1, sN0_2, sN0_3;
            short sN1_0, sN1_1, sN1_2, sN1_3;

            if (NotPiece[dir] != tempPiece) {
                sN0_0 = N[dir][0]; sN0_1 = N[dir][1];
                sN0_2 = N[dir][2]; sN0_3 = N[dir][3];
            } else {
                sN0_0 = 0; sN0_1 = 0; sN0_2 = 0; sN0_3 = 0;
            }
            if (NotPiece[dir + 1] != tempPiece) {
                sN1_0 = N[dir + 1][0]; sN1_1 = N[dir + 1][1];
                sN1_2 = N[dir + 1][2]; sN1_3 = N[dir + 1][3];
            } else {
                sN1_0 = 0; sN1_1 = 0; sN1_2 = 0; sN1_3 = 0;
            }

            bool bOpen0_0 = Open[dir][0] && (((N[dir][0]) == 0) || (NotPiece[dir] != tempPiece));
            bool bOpen0_1 = Open[dir][1] && (((N[dir][0] + N[dir][1]) == 0) || (NotPiece[dir] != tempPiece));
            bool bOpen0_2 = Open[dir][2] && (((N[dir][0] + N[dir][1] + N[dir][2]) == 0) || (NotPiece[dir] != tempPiece));
            bool bOpen0_3 = Open[dir][3] && (NotPiece[dir] != tempPiece);
            bool bOpen1_0 = Open[dir + 1][0] && (((N[dir + 1][0]) == 0) || (NotPiece[dir + 1] != tempPiece));
            bool bOpen1_1 = Open[dir + 1][1] && (((N[dir + 1][0] + N[dir + 1][1]) == 0) || (NotPiece[dir + 1] != tempPiece));
            bool bOpen1_2 = Open[dir + 1][2] && (((N[dir + 1][0] + N[dir + 1][1] + N[dir + 1][2]) == 0) || (NotPiece[dir + 1] != tempPiece));
            bool bOpen1_3 = Open[dir + 1][3] && (NotPiece[dir + 1] != tempPiece);

            auto recordChange = [&](int bcIdx, const QPoint &pt, short change, int slot) {
                if (change == 0 || sCount >= 17) return;
                if (!bAlreadyChanged[bcIdx]) {
                    changed->Coord[sCount] = pt;
                    changed->wBoard[sCount][0] = cell(pt.x(), pt.y(), 0);
                    changed->wBoard[sCount][1] = cell(pt.x(), pt.y(), 1);
                    bAlreadyChanged[bcIdx] = true;
                    sCount++;
                }
                cellRef(pt.x(), pt.y(), slot) = static_cast<short>(
                    cell(pt.x(), pt.y(), slot) + change);
            };

            if (bOpen0_0) {
                short Change1 = 0;
                countValue(sN1_0, sN0_0, sN0_1, sN0_2,
                           bOpen1_0, true, bOpen0_1, bOpen0_2, &Change1);
                Change1 = static_cast<short>(-Change1);
                countValue(
                    static_cast<short>(sN1_1 * (newPiece == tempPiece)),
                    static_cast<short>((sN1_0 + 1) * (newPiece == tempPiece) + sN0_0),
                    sN0_1, sN0_2,
                    bOpen1_1 && (newPiece == tempPiece),
                    bOpen1_0 && (newPiece == tempPiece),
                    bOpen0_1, bOpen0_2,
                    &Change1);
                recordChange(0, OpenPos[dir][0], Change1, tempPiece);
            }

            if (bOpen0_1) {
                short Change2 = 0;
                countValue(sN0_0, sN0_1, sN0_2, sN0_3,
                           true, bOpen0_0, bOpen0_2, bOpen0_3, &Change2);
                Change2 = static_cast<short>(-Change2);
                countValue(
                    static_cast<short>((sN1_0 + 1) * (newPiece == tempPiece) + sN0_0),
                    sN0_1, sN0_2, sN0_3,
                    bOpen1_0 && (newPiece == tempPiece),
                    bOpen0_0, bOpen0_2, bOpen0_3,
                    &Change2);
                recordChange(1, OpenPos[dir][1], Change2, tempPiece);
            }

            if (bOpen1_0) {
                short Change1 = 0;
                countValue(sN0_0, sN1_0, sN1_1, sN1_2,
                           bOpen0_0, true, bOpen1_1, bOpen1_2, &Change1);
                Change1 = static_cast<short>(-Change1);
                countValue(
                    static_cast<short>(sN0_1 * (newPiece == tempPiece)),
                    static_cast<short>((sN0_0 + 1) * (newPiece == tempPiece) + sN1_0),
                    sN1_1, sN1_2,
                    bOpen0_1 && (newPiece == tempPiece),
                    bOpen0_0 && (newPiece == tempPiece),
                    bOpen1_1, bOpen1_2,
                    &Change1);
                recordChange(2, OpenPos[dir + 1][0], Change1, tempPiece);
            }

            if (bOpen1_1) {
                short Change2 = 0;
                countValue(sN1_0, sN1_1, sN1_2, sN1_3,
                           true, bOpen1_0, bOpen1_2, bOpen1_3, &Change2);
                Change2 = static_cast<short>(-Change2);
                countValue(
                    static_cast<short>((sN0_0 + 1) * (newPiece == tempPiece) + sN1_0),
                    sN1_1, sN1_2, sN1_3,
                    bOpen0_0 && (newPiece == tempPiece),
                    bOpen1_0, bOpen1_2, bOpen1_3,
                    &Change2);
                recordChange(3, OpenPos[dir + 1][1], Change2, tempPiece);
            }
        }
    }

    // Coord[0] = the new square itself (its old values recorded for undo).
    changed->Coord[0] = QPoint(x, y);
    changed->wBoard[0][0] = cell(x, y, 0);
    changed->wBoard[0][1] = cell(x, y, 1);

    // Terminate the change list with a null point.
    if (sCount < 17)
        changed->Coord[sCount] = QPoint(0, 0);

    /* Mark the square occupied (speculatively: 0x8FFC means not-yet-real). */
    cellRef(x, y, newPiece) = static_cast<short>(0x8FFC);
    cellRef(x, y, 1 - newPiece) = 0;
}

void Game::takeAwayPiece(BoardChange *changed)
{
    int i = 0;
    while (i < 17 && changed->Coord[i] != QPoint(0, 0)) {
        cellRef(changed->Coord[i].x(), changed->Coord[i].y(), 0) = changed->wBoard[i][0];
        cellRef(changed->Coord[i].x(), changed->Coord[i].y(), 1) = changed->wBoard[i][1];
        i++;
    }
}

void Game::updateBest(BoardChange *changed, int piece,
                      const ValueList &vpIN, ValueList &vpOUT)
{
    short INindex = 0, CHindex = 0, OUTindex = 0, sCount = 0;
    bool INEnable = true, ChangedEnable = true;

    ValuePoint vpCH[17];
    for (int i = 0; i < 17; i++)
        vpCH[i].value = 0;

    /* sort changed squares (descending by value) */
    for (int i = 1; i < 17 && changed->Coord[i] != QPoint(0, 0); i++) {
        QPoint ptI = changed->Coord[i];
        short TempValue = cell(ptI.x(), ptI.y(), piece);
        if (TempValue == changed->wBoard[i][piece]) continue;

        short insertAt = sCount;
        while (insertAt > 0 && TempValue > vpCH[insertAt - 1].value) {
            vpCH[insertAt] = vpCH[insertAt - 1];
            insertAt--;
        }
        vpCH[insertAt].value = TempValue;
        vpCH[insertAt].point = ptI;
        if (++sCount > 16) sCount = 16;
    }

    if (vpCH[sCount].value == 0)
        vpCH[sCount].point.setX(0);

    INindex = 0;
    CHindex = 0;
    OUTindex = 0;

    do {
        /* drop old list entries whose value no longer matches the board */
        while (INindex < UPDATECOUNT &&
               vpIN.p[piece][INindex].point.x() != 0 &&
               vpIN.p[piece][INindex].value !=
                   cell(vpIN.p[piece][INindex].point.x(),
                        vpIN.p[piece][INindex].point.y(), piece))
            INindex++;

        if (INindex >= UPDATECOUNT || vpIN.p[piece][INindex].point.x() == 0) {
            INEnable = false;
            if (!ChangedEnable) break;
        }

        if (CHindex > 15 || vpCH[CHindex].point.x() == 0) {
            ChangedEnable = false;
            if (!INEnable) break;
        }

        if (ChangedEnable && ((!INEnable) ||
                              (vpIN.p[piece][INindex].value < vpCH[CHindex].value))) {
            vpOUT.p[piece][OUTindex].point = vpCH[CHindex].point;
            vpOUT.p[piece][OUTindex].value = vpCH[CHindex].value;
            CHindex++;
            OUTindex++;
        } else {
            vpOUT.p[piece][OUTindex].point = vpIN.p[piece][INindex].point;
            vpOUT.p[piece][OUTindex].value = vpIN.p[piece][INindex].value;
            INindex++;
            OUTindex++;
        }
    } while (OUTindex < UPDATECOUNT);

    if (OUTindex < UPDATECOUNT) {
        vpOUT.p[piece][OUTindex].point.setX(0);
        vpOUT.p[piece][OUTindex].value = 0;
    }
}

bool Game::findSimplyBest(int piece, int countMax, ValueList &vpBest)
{
    for (int i = 0; i < countMax; i++)
        vpBest.p[piece][i].value = 0;

    short sSize = 4;
    for (int i = 4; i <= 64; i = i * 2) {
        if (std::max(m_rectUsedRight - m_rectUsedLeft,
                     m_rectUsedBottom - m_rectUsedTop) > i)
            sSize = static_cast<short>(2 * i);
        else
            break;
    }
    int sSize2 = sSize * sSize;

    QPoint ptBase(m_rectUsedLeft, m_rectUsedTop);

    /* prime the LFSR random order */
    randomOrder(ptBase, sSize, true);

    short sCount = 0;
    bool bEmptyFound = false;

    for (int i = 0; i < sSize2; i++) {
        if (((i & 0x0020) == 0) && m_cancelRequested)
            return true;

        QPoint ptI = randomOrder(ptBase, sSize, false);

        if (ptI.x() >= m_rectUsedLeft && ptI.x() <= m_rectUsedRight &&
            ptI.y() >= m_rectUsedTop && ptI.y() <= m_rectUsedBottom &&
            bEmptySpec(ptI.x(), ptI.y())) {
            bEmptyFound = true;
            short TempValue = cell(ptI.x(), ptI.y(), piece);

            if (TempValue > vpBest.p[piece][countMax - 1].value) {
                short j = sCount;
                while (j > 0 && TempValue > vpBest.p[piece][j - 1].value) {
                    vpBest.p[piece][j] = vpBest.p[piece][j - 1];
                    j--;
                }
                vpBest.p[piece][j].value = TempValue;
                vpBest.p[piece][j].point = ptI;
                if ((++sCount) >= countMax)
                    sCount = countMax - 1;
            }
        }
    }

    if (vpBest.p[piece][sCount].value == 0)
        vpBest.p[piece][sCount].point.setX(0);

    return bEmptyFound;
}

bool Game::isThereValuablePointsForBoth(int piece, short pMin, short pMax,
                                        short oMin, short oMax,
                                        const QPoint &center, short radius)
{
    int rectBoundsLeft = std::max(center.x() - radius, m_rectBoardLeft);
    int rectBoundsRight = std::min(center.x() + radius, m_rectBoardRight);
    int rectBoundsTop = std::max(center.y() - radius, m_rectBoardTop);
    int rectBoundsBottom = std::min(center.y() + radius, m_rectBoardBottom);

    QPoint ptI;
    for (ptI.setX(rectBoundsLeft); ptI.x() <= rectBoundsRight; ptI.setX(ptI.x() + 1))
        for (ptI.setY(rectBoundsTop); ptI.y() <= rectBoundsBottom; ptI.setY(ptI.y() + 1)) {
            if (ptI.x() == center.x() && ptI.y() == center.y())
                continue;
            if (bEmptySpec(ptI.x(), ptI.y()) &&
                cell(ptI.x(), ptI.y(), piece) >= pMin &&
                cell(ptI.x(), ptI.y(), piece) <= pMax &&
                cell(ptI.x(), ptI.y(), 1 - piece) >= oMin &&
                cell(ptI.x(), ptI.y(), 1 - piece) <= oMax)
                return true;
        }
    return false;
}

void Game::isThereTrivialMove(int piece, int sLevelCount,
                              const ValueList &vpPoints, ValuePoint *pvpResult)
{
    short BestPieceValue = vpPoints.p[piece][0].value;
    short BestOppositePieceValue = vpPoints.p[1 - piece][0].value;

    pvpResult->value = 0;

    if (BestPieceValue == 0)
        pvpResult->value = static_cast<short>(-2000 + sLevelCount); // LoserValue
    else if (BestOppositePieceValue == 0)
        pvpResult->value = static_cast<short>(2000 - sLevelCount);  // WinValue
    else if ((BestPieceValue >= 0x1000) && (BestOppositePieceValue >= 0x1000)) {
        if ((m_sAlreadyFoundRows[piece] >= m_sAlreadyFoundRows[1 - piece]) || (sLevelCount >= 1))
            pvpResult->value = static_cast<short>(2000 - sLevelCount);
        else
            pvpResult->value = static_cast<short>(-2000 + sLevelCount);
    } else if (BestPieceValue >= 0x1000) {
        pvpResult->value = static_cast<short>(2000 - sLevelCount);
    } else if (BestOppositePieceValue >= 0x1000) {
        pvpResult->value = static_cast<short>(-2000 + sLevelCount);
    } else if ((BestPieceValue >= 0x170) &&
               !isThereValuablePointsForBoth(piece, 0xA0, 0x7FFF, 0xD0, 0x7FFF,
                                             vpPoints.p[piece][0].point, 1) &&
               isThereValuablePointsForBoth(piece, 0xD0,
                                            static_cast<short>(0x13F + (sLevelCount >= 6 ? 0x1000 : 0)),
                                            0x00, 0x7FFF,
                                            vpPoints.p[piece][0].point, 1)) {
        pvpResult->value = static_cast<short>(2000 - sLevelCount);
    } else if ((BestPieceValue >= 0x140) &&
               (BestOppositePieceValue < 0xE8) &&
               !isThereValuablePointsForBoth(piece, 0xA0, 0x7FFF, 0xD0, 0x7FFF,
                                             vpPoints.p[piece][0].point, 1)) {
        pvpResult->value = static_cast<short>(2000 - sLevelCount);
    }

    if (pvpResult->value <= 0)
        pvpResult->value = 0;
    else
        pvpResult->point = vpPoints.p[piece][0].point;
}

void Game::findMasterMove(int piece, int sLevelCount,
                          const ValueList &vpSelectedPoints, ValuePoint *pvpResult)
{
    short sPointsAlreadyChecked = 0;
    short sDefencePointsAlreadyChecked = 0;
    QPoint ptAlreadyChecked[16];
    ValueList vpSelectedPointsForNextLevel;
    BoardChange BoardChanged;
    ValuePoint vpBestResultSoFar;
    ValuePoint vpTemp;

    if ((sLevelCount & 0x0001) == 1)
        m_lCounter++;

    isThereTrivialMove(piece, sLevelCount, vpSelectedPoints, &vpTemp);
    if (vpTemp.value != 0) {
        pvpResult->value = vpTemp.value;
        pvpResult->point = vpTemp.point;
        return;
    }

    if (sLevelCount > (m_sDepth - 1)) {
        if ((vpSelectedPoints.p[piece][0].value > 0x100) &&
            ((vpSelectedPoints.p[1 - piece][0].value < 0x170) ||
             (vpSelectedPoints.p[1 - piece][1].value < 0xF0))) {
            vpTemp.value = static_cast<short>(2000 - sLevelCount);
            pvpResult->value = vpTemp.value;
            pvpResult->point = vpTemp.point;
            return;
        }
        if ((vpSelectedPoints.p[piece][0].value >= 0xD1) &&
            (vpSelectedPoints.p[1 - piece][0].value < 0x100)) {
            vpTemp.value = static_cast<short>(
                300 + std::min(500, std::max(0,
                    vpSelectedPoints.p[piece][0].value -
                    vpSelectedPoints.p[1 - piece][0].value)));
            pvpResult->value = vpTemp.value;
            pvpResult->point = vpTemp.point;
            return;
        }
        if (sLevelCount > m_sDepth) {
            pvpResult->value = 0;
            pvpResult->point = vpTemp.point;
            return;
        }
    }

    if (vpSelectedPoints.p[1 - piece][0].value > 0x1000) {
        vpBestResultSoFar.point = vpSelectedPoints.p[1 - piece][0].point;
        vpBestResultSoFar.value =
            static_cast<short>(-2000 * (vpSelectedPoints.p[1 - piece][1].value > 0x1000 ? 1 : 0));
        if ((sLevelCount == 0) || (vpSelectedPoints.p[1 - piece][1].value > 0x1000)) {
            pvpResult->value = vpBestResultSoFar.value;
            pvpResult->point = vpBestResultSoFar.point;
            return;
        }
        putPiece(vpBestResultSoFar.point.x(), vpBestResultSoFar.point.y(), piece, &BoardChanged);
        updateBest(&BoardChanged, piece, vpSelectedPoints, vpSelectedPointsForNextLevel);
        updateBest(&BoardChanged, 1 - piece, vpSelectedPoints, vpSelectedPointsForNextLevel);
        findMasterMove(1 - piece, sLevelCount + 1, vpSelectedPointsForNextLevel, &vpTemp);
        vpTemp.value = static_cast<short>(-vpTemp.value);
        takeAwayPiece(&BoardChanged);
        vpBestResultSoFar.value = vpTemp.value;

        pvpResult->value = vpBestResultSoFar.value;
        pvpResult->point = vpBestResultSoFar.point;
        return;
    }

    if (m_cancelRequested)
        return;

    vpBestResultSoFar.point = vpSelectedPoints.p[1 - piece][0].point;
    vpBestResultSoFar.value = static_cast<short>(-2000); // LoserValue (level 0)

    for (int i = 0; i < UPDATECOUNT; i++) {
        if (sPointsAlreadyChecked >= m_sAttackEvaluateMax[sLevelCount])
            break;
        QPoint ptI = vpSelectedPoints.p[piece][i].point;
        if (ptI.x() == 0)
            break;
        if ((sPointsAlreadyChecked >= 1) &&
            (cell(ptI.x(), ptI.y(), piece) < m_sAttackValueMin[sLevelCount]))
            break;
        if ((vpBestResultSoFar.value < -1000) &&
            (sPointsAlreadyChecked >= m_sLosersEvaluateMax[sLevelCount]))
            break;
        if ((sPointsAlreadyChecked >= m_sAttackEvaluateMin[sLevelCount]) &&
            (vpBestResultSoFar.value > 1000))
            break;

        if ((sPointsAlreadyChecked >= 1) &&
            (cell(ptI.x(), ptI.y(), piece) <= 0xD1) &&
            (cell(ptI.x(), ptI.y(), piece) >= 0xD0))
            continue;

        if (checkIfTestedBefore(&ptI, ptAlreadyChecked, &sPointsAlreadyChecked))
            continue;

        putPiece(ptI.x(), ptI.y(), piece, &BoardChanged);
        updateBest(&BoardChanged, piece, vpSelectedPoints, vpSelectedPointsForNextLevel);
        updateBest(&BoardChanged, 1 - piece, vpSelectedPoints, vpSelectedPointsForNextLevel);

        findMasterMove(1 - piece, sLevelCount + 1, vpSelectedPointsForNextLevel, &vpTemp);
        takeAwayPiece(&BoardChanged);

        vpTemp.value = static_cast<short>(-vpTemp.value);

        if (vpTemp.value > vpBestResultSoFar.value) {
            vpBestResultSoFar.value = vpTemp.value;
            vpBestResultSoFar.point = ptI;
        }

        if (m_cancelRequested)
            return;
    }

    if (vpBestResultSoFar.value > -1000) {
        pvpResult->value = vpBestResultSoFar.value;
        pvpResult->point = vpBestResultSoFar.point;
        return;
    }

    for (int i = 0; i < UPDATECOUNT; i++) {
        if (sDefencePointsAlreadyChecked >= m_sDefenceEvaluateMax[sLevelCount])
            break;
        QPoint ptI = vpSelectedPoints.p[1 - piece][i].point;
        if (ptI.x() == 0)
            break;
        if (cell(ptI.x(), ptI.y(), 1 - piece) < m_sDefenceValueMin[sLevelCount])
            break;

        if (checkIfTestedBefore(&ptI, ptAlreadyChecked, &sPointsAlreadyChecked))
            continue;

        sDefencePointsAlreadyChecked++;

        putPiece(ptI.x(), ptI.y(), piece, &BoardChanged);
        updateBest(&BoardChanged, piece, vpSelectedPoints, vpSelectedPointsForNextLevel);
        updateBest(&BoardChanged, 1 - piece, vpSelectedPoints, vpSelectedPointsForNextLevel);

        findMasterMove(1 - piece, sLevelCount + 1, vpSelectedPointsForNextLevel, &vpTemp);
        takeAwayPiece(&BoardChanged);

        vpTemp.value = static_cast<short>(-vpTemp.value);

        if (vpTemp.value > vpBestResultSoFar.value) {
            vpBestResultSoFar.value = vpTemp.value;
            vpBestResultSoFar.point = ptI;
        }

        if (sAlgTest && (vpTemp.value == vpBestResultSoFar.value) &&
            (cell(ptI.x(), ptI.y(), piece) >
             (cell(vpBestResultSoFar.point.x(), vpBestResultSoFar.point.y(), piece) + 0x10)) &&
            (cell(ptI.x(), ptI.y(), 1 - piece) >
             (vpSelectedPoints.p[1 - piece][0].value - 0x04))) {
            vpBestResultSoFar.value = vpTemp.value;
            vpBestResultSoFar.point = ptI;
        }

        if (vpBestResultSoFar.value > -1000)
            break;

        if (m_cancelRequested)
            return;
    }

    pvpResult->value = vpBestResultSoFar.value;
    pvpResult->point = vpBestResultSoFar.point;
}

void Game::findMyNextPoint(int piece, ValuePoint *pvpResult)
{
    ValueList vpTemp;

    // Default result in case of early cancellation.
    pvpResult->point.setX(0);
    pvpResult->point.setY(0);
    pvpResult->value = 0;

    updateValueArray();
    if (m_sGameCount == 1) {
        int i = getRandom(0, 19);
        int dx = 0, dy = 0;
        if (i < 4) { dx = 1; dy = 0; }
        else if (i < 8) { dx = -1; dy = 0; }
        else if (i < 12) { dx = 0; dy = 1; }
        else if (i < 16) { dx = 0; dy = -1; }
        else if (i < 17) { dx = 1; dy = 1; }
        else if (i < 18) { dx = 1; dy = -1; }
        else if (i < 19) { dx = -1; dy = 1; }
        else { dx = -1; dy = -1; }

        if (m_ptUserLastX <= (m_rectBoardLeft + 3)) dx = 1;
        if (m_ptUserLastX >= (m_rectBoardRight - 3)) dx = -1;
        if (m_ptUserLastY <= (m_rectBoardTop + 3)) dy = 1;
        if (m_ptUserLastY >= (m_rectBoardBottom - 3)) dy = -1;

        pvpResult->point.setX(m_ptUserLastX + dx);
        pvpResult->point.setY(m_ptUserLastY + dy);
        pvpResult->value = 0;
        return;
    }

    m_lCounter = 0;
    switch (m_sSkillLevel) {
    case 0: /* beginner */
    case 1: /* not used */
    case 2: /* novice */
        m_sDepth = 1;
        break;
    case 3: /* expert */
        m_sDepth = 3;
        break;
    case 4: /* not used */
        m_sDepth = 5;
        break;
    case 5: /* master */
        m_sDepth = 9;
        break;
    }

    for (int i = 0; i < 32; i++) {
        if (i < 3) {
            m_sAttackValueMin[i] = 0xA2;
            m_sDefenceValueMin[i] = 0xA2;
        } else if (i < 7) {
            m_sAttackValueMin[i] = 0xA9;
            m_sDefenceValueMin[i] = 0xA9;
        } else {
            m_sAttackValueMin[i] = 0xD0;
            m_sDefenceValueMin[i] = 0xD0;
        }

        m_sAttackEvaluateMin[i] = sMasterAttackEvaluateMin[std::min(i, 7)];
        m_sAttackEvaluateMax[i] = sMasterAttackEvaluateMax[std::min(i, 7)];
        m_sLosersEvaluateMax[i] = sMasterLosersEvaluateMax[std::min(i, 7)];
        m_sDefenceEvaluateMax[i] = sMasterDefenceEvaluateMax[std::min(i, 7)];
    }
    m_sUpdateValueMin = 0xA0;

    findSimplyBest(piece, UPDATECOUNT, vpTemp);
    if (m_cancelRequested) return;
    findSimplyBest(1 - piece, UPDATECOUNT, vpTemp);
    if (m_cancelRequested) return;

    findMasterMove(piece, 0, vpTemp, pvpResult);
}

bool Game::checkIfTestedBefore(QPoint *pptNew, QPoint ptAlreadyChecked[], short *psPointsAlreadyChecked)
{
    static constexpr int MAX_POINT_ALREADY_CHECKED = 15;
    short l = 0;
    while (l < *psPointsAlreadyChecked &&
           ((ptAlreadyChecked[l].x() != pptNew->x()) ||
            (ptAlreadyChecked[l].y() != pptNew->y())))
        l++;
    if (l < *psPointsAlreadyChecked) return true;
    ptAlreadyChecked[*psPointsAlreadyChecked] = *pptNew;
    (*psPointsAlreadyChecked)++;
    if (*psPointsAlreadyChecked > MAX_POINT_ALREADY_CHECKED)
        *psPointsAlreadyChecked = MAX_POINT_ALREADY_CHECKED;
    return false;
}

QPoint Game::randomOrder(QPoint ptBase, short sSize, bool bInitialize)
{
    static unsigned short wShiftReg = 0;
    static short sRegLength = 0, sRegTap = 0, sHalfLength = 0, wTestPattern = 0;
    static short i = 0;
    static QPoint point;

    if (bInitialize) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));

        wShiftReg = 0;
        for (i = 1; i < 16; i++)
            wShiftReg = static_cast<unsigned short>(
                wShiftReg | ((0x0001 & static_cast<unsigned short>(getRandom(0, 1))) << i));

        switch (sSize) {
        case 4:  sRegLength = 5;  sRegTap = 2;  wTestPattern = 0x0001; sHalfLength = 2; break;
        case 8:  sRegLength = 7;  sRegTap = 3;  wTestPattern = 0x0001; sHalfLength = 3; break;
        case 16: sRegLength = 9;  sRegTap = 4;  wTestPattern = 0x0001; sHalfLength = 4; break;
        case 32: sRegLength = 11; sRegTap = 2;  wTestPattern = 0x0001; sHalfLength = 5; break;
        case 64: sRegLength = 15; sRegTap = 14; wTestPattern = 0x0007; sHalfLength = 6; break;
        case 128:sRegLength = 15; sRegTap = 4;  wTestPattern = 0x0001; sHalfLength = 7; break;
        default: sRegLength = 9;  sRegTap = 4;  wTestPattern = 0x0001; sHalfLength = 4; break;
        }
    }

    do {
        wShiftReg = static_cast<unsigned short>(wShiftReg << 1);
        wShiftReg = static_cast<unsigned short>(
            (wShiftReg & 0xFFFE) ^
            ((wShiftReg & (0x0001 << sRegTap)) >> sRegTap) ^
            ((wShiftReg & (0x0001 << sRegLength)) >> sRegLength) ^
            0x0001);
    } while ((wShiftReg & wTestPattern) != 0);

    point.setX(ptBase.x() +
               ((wShiftReg & ((sSize - 1) << (sRegLength - sHalfLength))) >>
                (sRegLength - sHalfLength)));
    point.setY(ptBase.y() +
               ((wShiftReg & ((sSize - 1) << (sRegLength - 2 * sHalfLength))) >>
                (sRegLength - 2 * sHalfLength)));

    return point;
}

void Game::recordLastPieces(int x, int y, int piece, BoardChange *bc)
{
    updateValueArray();
    m_ptUserLastX = x;
    m_ptUserLastY = y;

    putPiece(x, y, piece, bc);

    /* putPiece marked the square speculatively (0x8FFC); make it real. */
    cellRef(x, y, piece) = static_cast<short>(0x8FFF);
    cellRef(x, y, 1 - piece) = 0;

    if (m_sGameCount == 0) {
        m_rectUsedLeft = std::max(x - 3, m_rectBoardLeft);
        m_rectUsedRight = std::min(x + 3, m_rectBoardRight);
        m_rectUsedTop = std::max(y - 3, m_rectBoardTop);
        m_rectUsedBottom = std::min(y + 3, m_rectBoardBottom);
    } else {
        if (m_rectUsedLeft > (x - 3)) m_rectUsedLeft = std::max(x - 3, m_rectBoardLeft);
        if (m_rectUsedRight < (x + 3)) m_rectUsedRight = std::min(x + 3, m_rectBoardRight);
        if (m_rectUsedTop > (y - 3)) m_rectUsedTop = std::max(y - 3, m_rectBoardTop);
        if (m_rectUsedBottom < (y + 3)) m_rectUsedBottom = std::min(y + 3, m_rectBoardBottom);
    }
}

bool Game::gameOver(int x, int y, int piece)
{
    bool Open[8][4];
    QPoint OpenPos[8][4];
    short N[8][4];
    short NotPiece[8];
    QPoint ptFoundRow[32];

    findNeighbours(x, y, Open, OpenPos, N, NotPiece);

    bool bGameEndFound = false;

    for (int dir = 0; dir < 8; dir = dir + 2) {
        int xMul = 0, yMul = 0;
        switch (dir) {
        case 0: xMul = 0; yMul = 1; break;
        case 2: xMul = 1; yMul = 0; break;
        case 4: xMul = 1; yMul = 1; break;
        case 6: xMul = 1; yMul = -1; break;
        }

        N[dir][0] = static_cast<short>(N[dir][0] * ((NotPiece[dir] != piece) ? 1 : 0));
        N[dir + 1][0] = static_cast<short>(N[dir + 1][0] * ((NotPiece[dir + 1] != piece) ? 1 : 0));

        short sNewRowLength = static_cast<short>(N[dir][0] + N[dir + 1][0] + 1);
        if (sNewRowLength >= CreditRowLength) {
            for (int i = 0; i < sNewRowLength; i++) {
                ptFoundRow[i].setX(x + (i - N[dir][0]) * xMul);
                ptFoundRow[i].setY(y + (i - N[dir][0]) * yMul);
            }

            bool skip = false;
            for (int i2 = m_sAlreadyFoundRows[piece] - 1; i2 >= 0; i2--) {
                short sDifferenceToOldRow = 0;
                for (int j = 0; j < 2; j++) {
                    QPoint ptTemp = ptFoundRow[j * (sNewRowLength - 1)];
                    while (sDifferenceToOldRow < sNewRowLength &&
                           ((ptTemp.x() != m_ptAlreadyFoundRowEnds[i2][piece][j].x()) ||
                            (ptTemp.y() != m_ptAlreadyFoundRowEnds[i2][piece][j].y()))) {
                        sDifferenceToOldRow++;
                        ptTemp.setX(ptTemp.x() + xMul);
                        ptTemp.setY(ptTemp.y() + yMul);
                    }
                    xMul *= -1;
                    yMul *= -1;
                }
                if (sDifferenceToOldRow == 0) {
                    sNewRowLength = 0;
                    skip = true;
                    break;
                }
                if (sDifferenceToOldRow < sNewRowLength) {
                    skip = true;
                    break;
                }
            }
            if (skip) continue;

            short sDifferenceToOldRow = static_cast<short>(sNewRowLength - CreditRowLength + 1);
            if (sNewRowLength) {
                m_ptAlreadyFoundRowEnds[m_sAlreadyFoundRows[piece]][piece][0] = ptFoundRow[0];
                m_ptAlreadyFoundRowEnds[m_sAlreadyFoundRows[piece]][piece][1] = ptFoundRow[sNewRowLength - 1];
                m_sAlreadyFoundRows[piece] += sDifferenceToOldRow;
                if (m_sAlreadyFoundRows[piece] >= m_sGameEndRows)
                    bGameEndFound = true;

                for (int i = 0; i < sNewRowLength; i++) {
                    cellRef(ptFoundRow[i].x(), ptFoundRow[i].y(), piece) = static_cast<short>(0x8FFE);
                    flashBoard(ptFoundRow[i].x(), ptFoundRow[i].y(), 8, 1);
                }
            }
        }
    }

    return bGameEndFound;
}

int Game::getRandom(int mn, int mx)
{
    return (std::rand() % (int)(((mx) + 1) - (mn))) + (mn);
}

bool Game::bEmptySpec(int x, int y) const
{
    return mFFC(cell(x, y, 0)) != 0x8FFC && mFFC(cell(x, y, 1)) != 0x8FFC;
}

bool Game::bEmptyReal(int x, int y) const
{
    return mFFE(cell(x, y, 0)) != 0x8FFE && mFFE(cell(x, y, 1)) != 0x8FFE;
}

int Game::cellOwner(int x, int y) const
{
    short v0 = cell(x, y, 0);
    short v1 = cell(x, y, 1);
    if (isWinMarker(v0)) return 2; // nought win
    if (isWinMarker(v1)) return 3; // cross win
    if (isOccupiedMarker(v0)) return 0;
    if (isOccupiedMarker(v1)) return 1;
    return -1; // empty
}

bool Game::isCellEmpty(int x, int y) const
{
    return bEmptyReal(x, y);
}

QSize Game::boardCellCount() const
{
    return QSize(m_rectBoardRight - m_rectBoardLeft + 1,
                 m_rectBoardBottom - m_rectBoardTop + 1);
}

void Game::flashBoard(int x, int y, int count, int end)
{
    if (count == -1) {       // stop all flashing
        m_flashing.clear();
        return;
    }

    for (auto &fc : m_flashing) {
        if (fc.x == x && fc.y == y) {
            fc.remaining = 2 * count;
            fc.end = end;
            return;
        }
    }
    FlashCell fc;
    fc.x = x;
    fc.y = y;
    fc.remaining = 2 * count;
    fc.end = end;
    m_flashing.push_back(fc);
}

bool Game::flashTimerTick()
{
    bool foundMore = false;
    for (auto it = m_flashing.begin(); it != m_flashing.end(); ) {
        if (it->remaining > 0) {
            it->remaining--;
            if (it->remaining > 0)
                foundMore = true;
        }
        if (it->remaining == 0)
            it = m_flashing.erase(it);
        else
            ++it;
    }
    return foundMore;
}

bool Game::cellFlashing(int x, int y, int &remaining, int &end) const
{
    for (const auto &fc : m_flashing)
        if (fc.x == x && fc.y == y) {
            remaining = fc.remaining;
            end = fc.end;
            return true;
        }
    return false;
}

void Game::computeComputerMove(ValuePoint *result)
{
    m_cancelRequested = false;
    findMyNextPoint(m_ComputersPiece, result);
}

bool Game::userMove(int x, int y)
{
    if (!m_bGameGoing || m_bProcessing)
        return false;
    if (x < m_rectBoardLeft || x > m_rectBoardRight ||
        y < m_rectBoardTop || y > m_rectBoardBottom)
        return false;
    if (!bEmptyReal(x, y))
        return false;

    m_sBeforeUsersPieceAlreadyFoundRows[0] = m_sAlreadyFoundRows[0];
    m_sBeforeUsersPieceAlreadyFoundRows[1] = m_sAlreadyFoundRows[1];

    BoardChange bc;
    recordLastPieces(x, y, m_UsersPiece, &bc);
    m_bcUserLast = bc;
    m_bHasUserLast = true;

    ValueList dummy;
    if (!findSimplyBest(0, 1, dummy) && !findSimplyBest(1, 1, dummy)) {
        m_bGameGoing = false;
        flashBoard(x, y, 2, 0);
        return true; // move accepted: draw
    }
    if (gameOver(x, y, m_UsersPiece)) {
        m_sUserWins++;
        m_bGameGoing = false;
        return true; // move accepted: user wins
    }

    m_sGameCount++;
    m_bUsersTurn = false;
    m_bProcessing = true;
    return true; // move accepted: computer to move
}

void Game::applyComputerMove(int x, int y)
{
    if (!m_bGameGoing)
        return;
    // Defensive: ignore moves outside the board or on occupied cells.
    if (x < m_rectBoardLeft || x > m_rectBoardRight ||
        y < m_rectBoardTop || y > m_rectBoardBottom ||
        !bEmptyReal(x, y)) {
        m_bProcessing = false;
        m_bUsersTurn = true;
        return;
    }

    m_sBeforeComputersPieceAlreadyFoundRows[0] = m_sAlreadyFoundRows[0];
    m_sBeforeComputersPieceAlreadyFoundRows[1] = m_sAlreadyFoundRows[1];

    BoardChange bc;
    recordLastPieces(x, y, m_ComputersPiece, &bc);
    m_bcComputerLast = bc;
    m_bHasComputerLast = true;

    ValueList dummy;
    if (!findSimplyBest(0, 1, dummy) && !findSimplyBest(1, 1, dummy)) {
        m_bGameGoing = false;
        flashBoard(x, y, 3, 0);
        m_bProcessing = false;
        return;
    }
    if (gameOver(x, y, m_ComputersPiece)) {
        m_sUserLosses++;
        m_bGameGoing = false;
        m_bProcessing = false;
        return;
    }

    m_bUsersTurn = true;
    flashBoard(x, y, 3, 0);
    m_bProcessing = false;
}

void Game::undo()
{
    if (m_bProcessing)
        return;

    if (m_bHasComputerLast) {
        takeAwayPiece(&m_bcComputerLast);
        m_sAlreadyFoundRows[0] = m_sBeforeComputersPieceAlreadyFoundRows[0];
        m_sAlreadyFoundRows[1] = m_sBeforeComputersPieceAlreadyFoundRows[1];
    } else {
        m_sAlreadyFoundRows[0] = m_sBeforeUsersPieceAlreadyFoundRows[0];
        m_sAlreadyFoundRows[1] = m_sBeforeUsersPieceAlreadyFoundRows[1];
    }
    takeAwayPiece(&m_bcUserLast);

    m_bcComputerLast.Coord[0] = QPoint(0, 0);
    m_bcUserLast.Coord[0] = QPoint(0, 0);
    m_bHasComputerLast = false;
    m_bHasUserLast = false;

    m_bGameGoing = true;
    m_bUsersTurn = true;
}

void Game::hint(int &hx, int &hy) const
{
    ValuePoint vp;
    Game *self = const_cast<Game *>(this);
    self->findMyNextPoint(self->m_ComputersPiece, &vp);
    hx = vp.point.x();
    hy = vp.point.y();
}
