#include "gamewidget.h"
#include "board.h"

#include <algorithm>
#include <QPainter>
#include <QScrollArea>
#include <QApplication>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QPaintEvent>

void GameWidget::startFlashing()
{
    m_flashTimer.start();
}

void GameWidget::stopFlashing()
{
    m_flashTimer.stop();
    m_game->flashBoard(0, 0, -1, -1);
    update();
}

GameWidget::GameWidget(Game *game, QWidget *parent)
    : QWidget(parent), m_game(game)
{
    setFocusPolicy(Qt::StrongFocus);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

    m_flashTimer.setInterval(60); // original flashing timer: 60 ms
    connect(&m_flashTimer, &QTimer::timeout, this, [this]() {
        if (m_game->flashTimerTick())
            update();
        else
            m_flashTimer.stop();
    });

    m_cursorCell = QPoint(m_game->m_rectBoardLeft, m_game->m_rectBoardTop);
}

QSize GameWidget::sizeHint() const
{
    QSize cells = m_game->boardCellCount();
    int margin = boardMargin();
    return QSize(margin * 2 + cells.width() * m_game->m_sGridSize,
                 margin * 2 + cells.height() * m_game->m_sGridSize);
}

QSize GameWidget::minimumSizeHint() const
{
    return sizeHint();
}

void GameWidget::refreshGeometry()
{
    updateGeometry();
    update();
}

int GameWidget::boardMargin() const
{
    return m_game->m_sBorderWidth * 4;
}

int GameWidget::cellPixelX(int x) const
{
    return boardMargin() + (x - m_game->m_rectBoardLeft) * m_game->m_sGridSize;
}

int GameWidget::cellPixelY(int y) const
{
    return boardMargin() + (y - m_game->m_rectBoardTop) * m_game->m_sGridSize;
}

QRect GameWidget::cellRect(int x, int y) const
{
    return QRect(cellPixelX(x) + 1, cellPixelY(y) + 1,
                 m_game->m_sGridBitmapSize, m_game->m_sGridBitmapSize);
}

QPoint GameWidget::cellFromPoint(const QPoint &pos) const
{
    int g = m_game->m_sGridSize;
    int margin = boardMargin();
    if (pos.x() < margin || pos.y() < margin)
        return QPoint(-1, -1);
    int x = (pos.x() - margin) / g + m_game->m_rectBoardLeft;
    int y = (pos.y() - margin) / g + m_game->m_rectBoardTop;
    if (!pointInBoard(x, y))
        return QPoint(-1, -1);
    return QPoint(x, y);
}

bool GameWidget::pointInBoard(int x, int y) const
{
    return x >= m_game->m_rectBoardLeft && x <= m_game->m_rectBoardRight &&
           y >= m_game->m_rectBoardTop && y <= m_game->m_rectBoardBottom;
}

void GameWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    int g = m_game->m_sGridSize;
    int margin = boardMargin();
    int boardW = m_game->boardCellCount().width() * g;
    int boardH = m_game->boardCellCount().height() * g;
    QRect boardContent(margin, margin, boardW, boardH);
    (void)boardContent;

    // Background: board colour.
    p.fillRect(rect(), QColor(m_game->m_BoardRGB[0],
                              m_game->m_BoardRGB[1],
                              m_game->m_BoardRGB[2]));

    // Bevel border around the board content (raised 3-D look, like the original).
    QRect bevel = boardContent.adjusted(-margin, -margin, margin, margin);
    QColor boardCol(m_game->m_BoardRGB[0], m_game->m_BoardRGB[1], m_game->m_BoardRGB[2]);
    QColor light = boardCol.lighter(125);
    QColor dark = boardCol.darker(125);
    p.setPen(light);
    p.drawLine(bevel.left(), bevel.top(), bevel.right(), bevel.top());
    p.drawLine(bevel.left(), bevel.top(), bevel.left(), bevel.bottom());
    p.setPen(dark);
    p.drawLine(bevel.left(), bevel.bottom(), bevel.right(), bevel.bottom());
    p.drawLine(bevel.right(), bevel.top(), bevel.right(), bevel.bottom());

    // Grid lines + cells.
    drawGrid(p);
    drawPieces(p);
}

void GameWidget::drawGrid(QPainter &p)
{
    int g = m_game->m_sGridSize;
    int lw = m_game->m_sGridLineWidth;
    QPen gridPen(Qt::black, lw, Qt::SolidLine);
    p.setPen(gridPen);

    int boardW = m_game->boardCellCount().width() * g;
    int boardH = m_game->boardCellCount().height() * g;
    int margin = boardMargin();
    int left = margin;
    int top = margin;
    int right = margin + boardW + 1;
    int bottom = margin + boardH + 1;

    // Vertical lines.
    for (int x = m_game->m_rectBoardLeft; x <= m_game->m_rectBoardRight + 1; x++) {
        int px = boardMargin() + (x - m_game->m_rectBoardLeft) * g;
        p.drawLine(px, top, px, bottom - 1);
    }
    // Horizontal lines.
    for (int y = m_game->m_rectBoardTop; y <= m_game->m_rectBoardBottom + 1; y++) {
        int py = boardMargin() + (y - m_game->m_rectBoardTop) * g;
        p.drawLine(left, py, right - 1, py);
    }
}

void GameWidget::drawPieces(QPainter &p)
{
    for (int y = m_game->m_rectBoardTop; y <= m_game->m_rectBoardBottom; y++)
        for (int x = m_game->m_rectBoardLeft; x <= m_game->m_rectBoardRight; x++) {
            int owner = m_game->cellOwner(x, y);
            if (owner < 0)
                continue;
            int flashRemaining = 0, flashEnd = 0;
            bool flashing = m_game->cellFlashing(x, y, flashRemaining, flashEnd);
            drawCell(p, x, y, owner, flashing, flashRemaining, flashEnd);
        }
}

void GameWidget::drawCell(QPainter &p, int x, int y, int owner, bool flashing,
                          int flashRemaining, int flashEnd)
{
    QRect cr = cellRect(x, y);

    QColor noughtCol(m_game->m_NoughtRGB[0], m_game->m_NoughtRGB[1], m_game->m_NoughtRGB[2]);
    QColor crossCol(m_game->m_CrossRGB[0], m_game->m_CrossRGB[1], m_game->m_CrossRGB[2]);
    QColor boardCol(m_game->m_BoardRGB[0], m_game->m_BoardRGB[1], m_game->m_BoardRGB[2]);

    // Determine the piece colour for this owner (2/3 are winning rows).
    QColor pieceCol;
    if (owner == 0 || owner == 2) pieceCol = noughtCol;
    else pieceCol = crossCol;

    // Flashing: alternate between normal and inverted colours (like the
    // original Windows 3 bitmap inversion, Piece+2).
    bool inverted = flashing && (((flashRemaining + flashEnd) % 2) == 1);
    QColor bg = inverted ? pieceCol : boardCol;
    QColor fg = inverted ? boardCol : pieceCol;

    p.fillRect(cr, bg);

    int inset = m_game->m_sGridBitmapSize / 6;
    QRect inner = cr.adjusted(inset, inset, -inset, -inset);

    if (owner == 0 || owner == 2) { // nought: ellipse outline
        QPen pen(fg, m_game->m_noughtLineWidth, Qt::SolidLine);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        if (inner.width() > 0 && inner.height() > 0)
            p.drawEllipse(inner);
    } else { // cross: two diagonals inside the inset rect
        QPen pen(fg, m_game->m_crossLineWidth, Qt::SolidLine);
        p.setPen(pen);
        p.drawLine(inner.topLeft(), inner.bottomRight());
        p.drawLine(inner.topRight(), inner.bottomLeft());
    }
}

void GameWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPoint c = cellFromPoint(event->pos());
        if (c.x() >= 0) {
            m_cursorCell = c;
            emit cellClicked(c.x(), c.y());
        }
    }
    QWidget::mousePressEvent(event);
}

void GameWidget::mouseMoveEvent(QMouseEvent *event)
{
    m_lastMouseCell = cellFromPoint(event->pos());
    QWidget::mouseMoveEvent(event);
}

void GameWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
        int nx = m_cursorCell.x() + ((event->key() == Qt::Key_Left) ? -1 : 1);
        nx = std::clamp(nx, m_game->m_rectBoardLeft, m_game->m_rectBoardRight);
        m_cursorCell.setX(nx);
        centerViewOnCell(m_cursorCell.x(), m_cursorCell.y());
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) {
        int ny = m_cursorCell.y() + ((event->key() == Qt::Key_Up) ? -1 : 1);
        ny = std::clamp(ny, m_game->m_rectBoardTop, m_game->m_rectBoardBottom);
        m_cursorCell.setY(ny);
        centerViewOnCell(m_cursorCell.x(), m_cursorCell.y());
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit cellClicked(m_cursorCell.x(), m_cursorCell.y());
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        emit escapePressed();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void GameWidget::focusInEvent(QFocusEvent * /*event*/)
{
    m_cursorInClientArea = true;
}

void GameWidget::focusOutEvent(QFocusEvent * /*event*/)
{
    m_cursorInClientArea = false;
}

void GameWidget::updateCursorCell()
{
    // no-op placeholder; cursor tracked via key/mouse events.
}

void GameWidget::centerViewOnCell(int x, int y)
{
    QRect cr = cellRect(x, y);
    emit scrollTo(cr.x(), cr.y(), cr.width(), cr.height());
}
