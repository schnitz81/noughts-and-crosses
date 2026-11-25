#ifndef BOARD_H
#define BOARD_H

#include <atomic>
#include <cstddef>
#include <vector>

#include <QPoint>
#include <QSize>

/*
 * Noughts and Crosses (connect-N / Gomoku style) game logic.
 *
 * Faithful port of the 1993-1994 Windows 3.x C sources
 * (ALG_V11.C, MSW-MISC.C, 0x.h, type.h, macro.h, vardef.h, varext.h)
 * by Pentti Moilanen (GPL v2+).
 *
 * Board: flat array of `short`, indexed (x << BoardXExp) + (y << BoardYExp) + piece,
 * piece in {0,1}; BoardXExp=7, BoardYExp=1. Each cell occupies two shorts
 * (slot 0 == nought, slot 1 == cross).
 *
 * Slot marker values (original was 16-bit, so 0x8FFF etc. were negative shorts;
 * we compare 16-bit bit-patterns so behaviour is identical on 32/64-bit):
 *   0x0000       truly empty, value 0
 *   0x0001..0x7FFF heuristic value (empty square scored by the AI)
 *   0x8FFC       speculative occupation (temporary during AI search)
 *   0x8FFF       real occupation (slot tells which piece)
 *   0x8FFE       winning-row marker
 */

struct ValuePoint {
    QPoint point;
    short value = 0;
};

struct ValueList {
    ValuePoint p[2][20];   // [piece][UPDATECOUNT]
};

struct BoardChange {
    short wBoard[17][2];
    QPoint Coord[17];
};

class Game {
public:
    // -------- constants (from 0x.h / type.h) --------
    static constexpr int BoardXExp = 7;
    static constexpr int BoardYExp = 1;
    static constexpr int BOARD_SIZE = 62;
    static constexpr int BOARD_SAFETY = 1;
    static constexpr int TOTAL_BOARD_SIZE = 64;   // address space 0..63
    static constexpr int MAX_GRID_SIZE = 64;
    static constexpr int MIN_GRID_SIZE = 4;
    static constexpr int UPDATECOUNT = 20;

    static constexpr int CreditRowLength = 5;     // "you cannot change this"

    static constexpr short sMasterAttackEvaluateMin[8] = {4, 1, 1, 1, 1, 1, 1, 1};
    static constexpr short sMasterAttackEvaluateMax[8] = {16, 16, 12, 8, 8, 8, 8, 6};
    static constexpr short sMasterLosersEvaluateMax[8] = {4, 4, 4, 1, 1, 1, 1, 1};
    static constexpr short sMasterDefenceEvaluateMax[8] = {16, 16, 12, 6, 6, 6, 6, 4};

    enum Piece { Nought = 0, Cross = 1 };

    Game();
    ~Game();

    // -------- runtime game state --------
    bool m_bGameGoing = false;
    int m_sGameCount = 0;          // plies in the current game
    int m_sSkillLevel = 5;         // 0..5 (0 beginner, 2 novice, 3 expert, 5 master)
    int m_sBorderWidth = 4;
    int m_sGridSize = 16;          // pixels per cell

    int m_UsersPiece = Cross;      // 0 nought / 1 cross
    int m_ComputersPiece = Nought;
    int m_sUserWins = 0;
    int m_sUserLosses = 0;

    int m_sGameEndRows = 1;        // 1..9 rows needed to win
    int m_bComputersFirstMove = 0; // 0 user, 1 computer, 2 alternating, 3 random

    bool m_bBackgroundProcessingEnabled = true; // compute computer moves off GUI thread
    bool m_bProcessing = false;     // computer is currently thinking
    std::atomic<bool> m_cancelRequested{false};

    int m_sAlreadyFoundRows[2] = {0, 0};

    int m_sGridLineWidth = 1;
    int m_sGridBitmapSize = 0;
    int m_sBoardBorderLineWidth = 0;
    int m_crossLineWidth = 2;
    int m_noughtLineWidth = 2;
    int m_circleInset = 0;

    int m_CrossRGB[3] = {0, 0, 255};
    int m_NoughtRGB[3] = {255, 0, 0};
    int m_BoardRGB[3] = {192, 192, 192};

    int m_rectBoardLeft = BOARD_SAFETY;
    int m_rectBoardRight = BOARD_SAFETY + BOARD_SIZE - 1;
    int m_rectBoardTop = BOARD_SAFETY;
    int m_rectBoardBottom = BOARD_SAFETY + BOARD_SIZE - 1;

    int m_rectUsedLeft = BOARD_SAFETY;
    int m_rectUsedRight = BOARD_SAFETY;
    int m_rectUsedTop = BOARD_SAFETY;
    int m_rectUsedBottom = BOARD_SAFETY;

    int m_sXClientOrigin = 0;
    int m_sYClientOrigin = 0;

    int m_ptComputerLastX = 0;
    int m_ptComputerLastY = 0;
    int m_ptUserLastX = 0;
    int m_ptUserLastY = 0;
    bool m_bHasComputerLast = false;
    bool m_bHasUserLast = false;
    BoardChange m_bcComputerLast;
    BoardChange m_bcUserLast;
    short m_sBeforeComputersPieceAlreadyFoundRows[2] = {0, 0};
    short m_sBeforeUsersPieceAlreadyFoundRows[2] = {0, 0};

    // found row ends for win detection (16 rows x 2 pieces x 2 ends)
    QPoint m_ptAlreadyFoundRowEnds[16][2][2];

    // -------- initialisation / settings --------
    void initAlgorithmNewGame();
    void updateGridSize();
    void translateBoardSkillLevel();

    // -------- moves --------
    bool userMove(int x, int y);                 // place user piece; false if not allowed
    void computeComputerMove(ValuePoint *result); // worker entry point (AI)
    void applyComputerMove(int x, int y);        // apply a computed computer move
    void undo();
    void hint(int &hx, int &hy) const;           // where the computer would play

    bool isGameGoing() const { return m_bGameGoing; }
    bool isUsersTurn() const { return m_bUsersTurn; }
    int usersPiece() const { return m_UsersPiece; }
    int computersPiece() const { return m_ComputersPiece; }

    // -------- board queries (for painting) --------
    int cellOwner(int x, int y) const;           // 0 nought, 1 cross, -1 empty, 2 nought-win, 3 cross-win
    bool isCellEmpty(int x, int y) const;
    QSize boardCellCount() const;                // playable cells (width, height)
    int gridSize() const { return m_sGridSize; }

    // -------- flashing --------
    void flashBoard(int x, int y, int count, int end);
    bool flashTimerTick();                       // call each flash timer tick; true if still flashing
    bool cellFlashing(int x, int y, int &remaining, int &end) const; // view query

    friend class MainWindow;

private:
    bool m_bUsersTurn = true;
    bool m_bCurrentGameStartedByComputer = false;

    std::vector<short> m_board;                  // (65*64*2) shorts

    struct FlashCell {
        int x, y;
        int remaining;   // ticks left
        int end;         // parity end flag
    };
    std::vector<FlashCell> m_flashing;

    short m_NearValue[16][2];
    short m_FarValue[16][2];
    short m_sAttackValueMin[32];
    short m_sDefenceValueMin[32];
    short m_sAttackEvaluateMin[32];
    short m_sAttackEvaluateMax[32];
    short m_sLosersEvaluateMax[32];
    short m_sDefenceEvaluateMax[32];
    int m_sDepth = 0;
    long m_lCounter = 0;
    short m_sUpdateValueMin = 0;
    static constexpr int sAlgTest = 1;

    // board helpers
    inline short &cellRef(int x, int y, int p) {
        return m_board[((x) << BoardXExp) + ((y) << BoardYExp) + p];
    }
    inline short cell(int x, int y, int p) const {
        return m_board[((x) << BoardXExp) + ((y) << BoardYExp) + p];
    }
    static inline int mFFC(short v) { return static_cast<unsigned short>(v) & 0xFFFC; }
    static inline int mFFE(short v) { return static_cast<unsigned short>(v) & 0xFFFE; }
    static bool isOccupiedMarker(short v) { return static_cast<unsigned short>(v) == 0x8FFFu; }
    static bool isWinMarker(short v)    { return static_cast<unsigned short>(v) == 0x8FFEu; }
    bool bEmptySpec(int x, int y) const;
    bool bEmptyReal(int x, int y) const;

    // algorithm (ported from ALG_V11.C)
    void updateValueArray();
    short nearValueFunction(short sLength, bool open);
    short farValueFunction(short sLength, bool open);
    void countValue(short NA, short NB, short NC, short ND,
                    bool OA, bool OB, bool OC, bool OD, short *sChange1);
    void findNeighbours(int xM, int yM,
                        bool Open[8][4], QPoint OpenPos[8][4],
                        short N[8][4], short NotPiece[8]) const;
    void putPiece(int x, int y, int newPiece, BoardChange *changed);
    void takeAwayPiece(BoardChange *changed);
    void updateBest(BoardChange *changed, int piece,
                    const ValueList &vpIN, ValueList &vpOUT);
    bool findSimplyBest(int piece, int countMax, ValueList &vpBest);
    bool isThereValuablePointsForBoth(int piece, short pMin, short pMax,
                                      short oMin, short oMax,
                                      const QPoint &center, short radius);
    void isThereTrivialMove(int piece, int sLevelCount,
                            const ValueList &vpPoints, ValuePoint *pvpResult);
    void findMasterMove(int piece, int sLevelCount,
                        const ValueList &vpSelectedPoints, ValuePoint *pvpResult);
    void findMyNextPoint(int piece, ValuePoint *pvpResult);
    bool checkIfTestedBefore(QPoint *pptNew, QPoint ptAlreadyChecked[], short *psPointsAlreadyChecked);
    QPoint randomOrder(QPoint ptBase, short sSize, bool bInitialize);
    void recordLastPieces(int x, int y, int piece, BoardChange *bc);
    bool gameOver(int x, int y, int piece);

    int getRandom(int mn, int mx);
};

#endif // BOARD_H
