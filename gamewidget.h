#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QWidget>
#include <QTimer>

class Game;

/*
 * The game board view.
 *
 * Paints the board, grid lines, noughts/crosses, winning rows and the
 * flashing animation.  It is designed to live inside a QScrollArea so
 * large boards scroll naturally; the widget exposes its fixed content
 * size and the view layer drives scrolling/zooming/centering.
 */
class GameWidget : public QWidget {
    Q_OBJECT
public:
    explicit GameWidget(Game *game, QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    // Recompute geometry after the grid size changed.
    void refreshGeometry();

signals:
    // Emitted when the user clicks a board cell (cell coords).
    void cellClicked(int x, int y);
    // Emitted when the user presses Escape.
    void escapePressed();
    // Emitted to ask the view to scroll so the given cell rect is visible.
    void scrollTo(int x, int y, int w, int h);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    Game *m_game;
    bool m_cursorInClientArea = false;
    QPoint m_cursorCell;          // keyboard cursor cell (board coords)
    QPoint m_lastMouseCell;       // last cell the mouse hovered over

    QTimer m_flashTimer;          // 60 ms flashing ticks

    // geometry helpers
    int boardMargin() const;
    int cellPixelX(int x) const;
    int cellPixelY(int y) const;
    QRect cellRect(int x, int y) const;          // public: used by MainWindow

    // flashing control
    void startFlashing();
    void stopFlashing();
    QPoint cellFromPoint(const QPoint &pos) const;
    bool pointInBoard(int x, int y) const;

    // painting
    void drawBoardBackground(QPainter &p);
    void drawBevelBorder(QPainter &p, const QRect &viewRect);
    void drawGrid(QPainter &p);
    void drawPieces(QPainter &p);
    void drawCell(QPainter &p, int x, int y, int owner, bool flashing,
                  int flashRemaining, int flashEnd);

    void updateCursorCell();
    void centerViewOnCell(int x, int y);

    friend class MainWindow;
};

#endif // GAMEWIDGET_H
