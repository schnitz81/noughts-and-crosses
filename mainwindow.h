#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class Game;
class GameWidget;
class ComputerWorker;
class QAction;
class QMenu;
class QScrollArea;
class QThread;

/*
 * Main window for the Qt port of the Windows 3.x "Noughts and Crosses" game.
 * Mirrors the original menu structure (Game / Skill / View / Options / Cheat / Help)
 * and runs the computer's thinking on a background thread so the GUI stays responsive.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Game menu
    void newGame();
    void exitApp();
    void showStatistics();
    void showAbout();
    void howToPlay();

    // Skill / Options menus
    void setSkill(int level);
    void setUsersPiece(int piece);
    void setFirstMove(int mode);
    void setGameEndRows(int rows);
    void toggleBackgroundProcessing();

    // View menu
    void zoomIn();
    void zoomOut();
    void centerView();
    void flashComputersLast();

    // Cheat menu
    void undoMove();
    void hintMove();

    // Board / AI
    void onCellClicked(int x, int y);
    void onComputerResultReady(int x, int y);
    void onEscapePressed();
    void onScrollTo(int x, int y, int w, int h);

    // UI refresh
    void updateTitle();
    void updateStatus(const QString &msg);
    void updateMenuStates();

    // Settings
    void loadSettings();
    void saveSettings();

private:
    void startComputerMove();
    void changeGridSize(int newSize);
    void onComputerMoveTimeout();   // watchdog: AI took too long
    void onTestAutoClick();         // test: auto-click after CPU moves

    Game *m_game = nullptr;
    GameWidget *m_board = nullptr;
    QScrollArea *m_scroll = nullptr;

    QThread *m_compThread = nullptr;
    ComputerWorker *m_compWorker = nullptr;
    QTimer *m_compWatchdog = nullptr; // AI response watchdog
    QTimer *m_testTimer = nullptr;    // test auto-click timer
    int m_testClickX = 0;
    int m_testClickY = 0;

    // menu actions
    QMenu *m_menuGame = nullptr;
    QMenu *m_menuSkill = nullptr;
    QMenu *m_menuView = nullptr;
    QMenu *m_menuOptions = nullptr;
    QMenu *m_menuCheat = nullptr;
    QMenu *m_menuHelp = nullptr;

    QAction *m_actNew = nullptr;
    QAction *m_actUndo = nullptr;
    QAction *m_actHint = nullptr;
    QAction *m_actBackground = nullptr;
};

#endif // MAINWINDOW_H
