#include "mainwindow.h"
#include "board.h"
#include "gamewidget.h"
#include "aboutdialog.h"
#include "statisticsdialog.h"

#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QScrollArea>
#include <QStatusBar>
#include <QSettings>
#include <QKeySequence>
#include <QMessageBox>
#include <QThread>
#include <QMetaObject>
#include <QTimer>
#include <algorithm>
#include <cstdlib>

#include "computerworker.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_game = new Game;
    m_board = new GameWidget(m_game, this);
    m_scroll = new QScrollArea(this);
    m_scroll->setWidget(m_board);
    m_scroll->setWidgetResizable(false);
    setCentralWidget(m_scroll);

    setWindowTitle("Noughts and Crosses");
    setMinimumSize(400, 300);
    resize(720, 600);

    connect(m_board, &GameWidget::cellClicked, this, &MainWindow::onCellClicked);
    connect(m_board, &GameWidget::escapePressed, this, &MainWindow::onEscapePressed);
    connect(m_board, &GameWidget::scrollTo, this, &MainWindow::onScrollTo);

    // ---------- menu bar ----------
    QMenuBar *mb = menuBar();

    m_menuGame = mb->addMenu("&Game");
    QAction *actNew = m_menuGame->addAction("&New\tF2", this, &MainWindow::newGame);
    actNew->setShortcut(QKeySequence(Qt::Key_F2));
    m_actNew = actNew;
    m_menuGame->addSeparator();
    m_menuGame->addAction("&Statistics", this, &MainWindow::showStatistics);
    m_menuGame->addSeparator();
    QAction *actExit = m_menuGame->addAction("E&xit", this, &MainWindow::exitApp);
    actExit->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));

    m_menuSkill = mb->addMenu("&Skill");
    QActionGroup *skillGroup = new QActionGroup(this);
    skillGroup->setExclusive(true);
    struct { const char *name; int level; } skills[] = {
        { "Beginner", 0 }, { "Novice", 2 }, { "Expert", 3 }, { "Master", 5 }
    };
    for (auto &s : skills) {
        QAction *a = m_menuSkill->addAction(s.name);
        a->setCheckable(true);
        a->setData(s.level);
        skillGroup->addAction(a);
        if (s.level == m_game->m_sSkillLevel) a->setChecked(true);
    }
    connect(skillGroup, &QActionGroup::triggered,
            this, [this](QAction *a) { setSkill(a->data().toInt()); });

    m_menuView = mb->addMenu("&View");
    m_menuView->addAction("Zoom &In\tCtrl+I", this, &MainWindow::zoomIn)->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    m_menuView->addAction("Zoom &Out\tCtrl+O", this, &MainWindow::zoomOut)->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
    m_menuView->addSeparator();
    m_menuView->addAction("&Center", this, &MainWindow::centerView);
    m_menuView->addSeparator();
    m_menuView->addAction("&Computers last", this, &MainWindow::flashComputersLast);

    m_menuOptions = mb->addMenu("&Options");

    // User's piece
    QMenu *pieceMenu = m_menuOptions->addMenu("&User's piece");
    QActionGroup *pieceGroup = new QActionGroup(this);
    pieceGroup->setExclusive(true);
    QAction *actNought = pieceMenu->addAction("&Nought");
    actNought->setCheckable(true); actNought->setData(Game::Nought); pieceGroup->addAction(actNought);
    QAction *actCross = pieceMenu->addAction("&Cross");
    actCross->setCheckable(true); actCross->setData(Game::Cross); pieceGroup->addAction(actCross);
    connect(pieceGroup, &QActionGroup::triggered,
            this, [this](QAction *a) { setUsersPiece(a->data().toInt()); });
    if (m_game->m_UsersPiece == Game::Nought) actNought->setChecked(true);
    else actCross->setChecked(true);

    // Who begins
    QMenu *firstMenu = m_menuOptions->addMenu("&Who begins");
    QActionGroup *firstGroup = new QActionGroup(this);
    firstGroup->setExclusive(true);
    struct { const char *name; int mode; } firsts[] = {
        { "&User", 0 }, { "&Computer", 1 }, { "&Alternating", 2 }, { "&Random", 3 }
    };
    for (auto &f : firsts) {
        QAction *a = firstMenu->addAction(f.name);
        a->setCheckable(true); a->setData(f.mode); firstGroup->addAction(a);
        if (f.mode == m_game->m_bComputersFirstMove) a->setChecked(true);
    }
    connect(firstGroup, &QActionGroup::triggered,
            this, [this](QAction *a) { setFirstMove(a->data().toInt()); });

    // Rows of five
    QMenu *rowsMenu = m_menuOptions->addMenu("&Rows of five");
    QActionGroup *rowsGroup = new QActionGroup(this);
    rowsGroup->setExclusive(true);
    for (int r = 1; r <= 9; r++) {
        QString name = QString::number(r);
        QAction *a = rowsMenu->addAction(name);
        a->setCheckable(true); a->setData(r); rowsGroup->addAction(a);
        if (r == m_game->m_sGameEndRows) a->setChecked(true);
    }
    connect(rowsGroup, &QActionGroup::triggered,
            this, [this](QAction *a) { setGameEndRows(a->data().toInt()); });

    m_menuOptions->addSeparator();
    m_actBackground = m_menuOptions->addAction("&Background processing", this, &MainWindow::toggleBackgroundProcessing);
    m_actBackground->setCheckable(true);
    m_actBackground->setChecked(m_game->m_bBackgroundProcessingEnabled);

    m_menuCheat = mb->addMenu("&Cheat");
    m_actUndo = m_menuCheat->addAction("&Undo", this, &MainWindow::undoMove);
    m_actHint = m_menuCheat->addAction("&Hint", this, &MainWindow::hintMove);

    m_menuHelp = mb->addMenu("&Help");
    m_menuHelp->addAction("&How to play", this, &MainWindow::howToPlay);
    m_menuHelp->addSeparator();
    m_menuHelp->addAction("&About Noughts and Crosses...", this, &MainWindow::showAbout);

    loadSettings();
    m_board->refreshGeometry();
    centerView();
    updateTitle();
    updateMenuStates();

    // Start a game automatically so the user can play immediately.
    newGame();
}

MainWindow::~MainWindow()
{
    saveSettings();
    // Stop any background thinking and wait for the thread to finish
    // before the game object is destroyed.
    m_game->m_cancelRequested = true;
    if (m_compThread) {
        m_compThread->quit();
        m_compThread->wait();
        m_compThread = nullptr;
        m_compWorker = nullptr;
    }
    delete m_game;
}

void MainWindow::newGame()
{
    // Interrupt any computer thinking in progress.
    m_game->m_cancelRequested = true;

    m_game->initAlgorithmNewGame();

    int first = m_game->m_bComputersFirstMove;
    bool computerStarts = false;
    if (first == 1)
        computerStarts = true;
    else if (first == 2)
        computerStarts = !m_game->m_bCurrentGameStartedByComputer;
    else if (first == 3)
        computerStarts = (std::rand() % 10) > 5;
    m_game->m_bCurrentGameStartedByComputer = computerStarts;

    m_game->m_bGameGoing = true;
    m_game->m_bUsersTurn = !computerStarts;
    m_game->m_bProcessing = false;
    m_game->m_sGameCount = 0;
    m_game->flashBoard(0, 0, -1, -1); // stop all flashing

    updateMenuStates();
    updateTitle();
    m_board->update();
    m_board->stopFlashing();

    if (computerStarts) {
        // Computer opens in the centre (original behaviour).
        int cx = (m_game->m_rectBoardLeft + m_game->m_rectBoardRight) / 2;
        int cy = (m_game->m_rectBoardTop + m_game->m_rectBoardBottom) / 2;
        BoardChange bc;
        m_game->recordLastPieces(cx, cy, m_game->m_ComputersPiece, &bc);
        m_game->m_bcComputerLast = bc;
        m_game->m_bHasComputerLast = true;
        m_game->m_ptComputerLastX = cx;
        m_game->m_ptComputerLastY = cy;
        m_game->m_sGameCount = 1;
        m_game->m_bUsersTurn = true;
        m_game->flashBoard(cx, cy, 3, 0);
        m_board->startFlashing();
        updateTitle();
        m_board->update();
    }
}

void MainWindow::exitApp()
{
    qApp->quit();
}

void MainWindow::showStatistics()
{
    StatisticsDialog dlg(m_game->m_sUserWins, m_game->m_sUserLosses, this);
    dlg.exec();
}

void MainWindow::showAbout()
{
    AboutDialog dlg(this);
    dlg.exec();
}

void MainWindow::howToPlay()
{
    QMessageBox::information(this, "How to play",
        "Click on an empty square to place your piece.\n"
        "Build a row of five (or the chosen number of rows) before the computer.\n"
        "Use the Cheat menu for Undo / Hint, and the Options menu to choose "
        "your piece, who starts, and how many rows are needed to win.");
}

void MainWindow::setSkill(int level)
{
    if (level < 0 || level > 5) return;
    m_game->m_sSkillLevel = level;
    m_game->m_sUserWins = 0;
    m_game->m_sUserLosses = 0;
    m_game->translateBoardSkillLevel();
    updateMenuStates();
    updateTitle();
}

void MainWindow::setUsersPiece(int piece)
{
    if (m_game->m_bGameGoing) return;
    m_game->m_UsersPiece = piece;
    m_game->m_ComputersPiece = 1 - piece;
    updateMenuStates();
}

void MainWindow::setFirstMove(int mode)
{
    m_game->m_bComputersFirstMove = mode;
    updateMenuStates();
}

void MainWindow::setGameEndRows(int rows)
{
    if (rows < 1 || rows > 9) return;
    m_game->m_sGameEndRows = rows;
    updateMenuStates();
    updateTitle();
}

void MainWindow::toggleBackgroundProcessing()
{
    m_game->m_bBackgroundProcessingEnabled = !m_game->m_bBackgroundProcessingEnabled;
    m_actBackground->setChecked(m_game->m_bBackgroundProcessingEnabled);
}

void MainWindow::zoomIn()
{
    changeGridSize(std::min(Game::MAX_GRID_SIZE, m_game->m_sGridSize * 4 / 3));
}

void MainWindow::zoomOut()
{
    changeGridSize(std::max(Game::MIN_GRID_SIZE, m_game->m_sGridSize * 3 / 4));
}

void MainWindow::changeGridSize(int newSize)
{
    if (newSize == m_game->m_sGridSize) return;
    m_game->m_sGridSize = newSize;
    m_game->updateGridSize();
    m_board->refreshGeometry();
    centerView();
    m_board->update();
}

void MainWindow::centerView()
{
    int cx, cy;
    if (m_game->m_sGameCount == 0) {
        cx = (m_game->m_rectBoardLeft + m_game->m_rectBoardRight) / 2;
        cy = (m_game->m_rectBoardTop + m_game->m_rectBoardBottom) / 2;
    } else {
        cx = (m_game->m_rectUsedLeft + m_game->m_rectUsedRight) / 2;
        cy = (m_game->m_rectUsedTop + m_game->m_rectUsedBottom) / 2;
    }
    QRect cr = m_board->cellRect(cx, cy);
    m_scroll->ensureVisible(cr.center().x(), cr.center().y(), 0, 0);
}

void MainWindow::flashComputersLast()
{
    if (!m_game->m_bHasComputerLast) return;
    m_game->flashBoard(m_game->m_ptComputerLastX, m_game->m_ptComputerLastY, 3, 0);
    m_board->startFlashing();
    m_board->update();
}

void MainWindow::undoMove()
{
    if (m_game->m_bProcessing) return;
    m_game->m_cancelRequested = true;
    m_game->undo();
    m_game->flashBoard(0, 0, -1, -1);
    m_board->stopFlashing();
    updateMenuStates();
    updateTitle();
    m_board->update();
}

void MainWindow::hintMove()
{
    if (!m_game->m_bGameGoing || !m_game->isUsersTurn() || m_game->m_bProcessing)
        return;
    int hx = 0, hy = 0;
    m_game->hint(hx, hy);
    m_game->flashBoard(hx, hy, 3, 0);
    m_board->startFlashing();
    m_board->centerViewOnCell(hx, hy);
    m_board->update();
}

void MainWindow::onCellClicked(int x, int y)
{
    if (m_game->m_bProcessing) return;
    bool accepted = m_game->userMove(x, y);
    if (!accepted) return;

    m_board->startFlashing();
    updateTitle();
    updateMenuStates();
    m_board->update();

    if (!m_game->m_bGameGoing)
        return; // draw or user won: game over

    // Computer to move.
    if (!m_game->isUsersTurn() && m_game->m_bProcessing) {
        startComputerMove();
    }
}

void MainWindow::startComputerMove()
{
    m_game->m_cancelRequested = false;
    m_game->m_bProcessing = true;
    updateStatus("Thinking...");
    QApplication::setOverrideCursor(Qt::WaitCursor);
    updateMenuStates();

    // Watchdog: if the AI takes longer than 10 seconds, cancel it.
    if (!m_compWatchdog) {
        m_compWatchdog = new QTimer(this);
        m_compWatchdog->setSingleShot(true);
        connect(m_compWatchdog, &QTimer::timeout, this, &MainWindow::onComputerMoveTimeout);
    }
    m_compWatchdog->start(10000);

    m_compThread = new QThread(this);
    m_compWorker = new ComputerWorker(m_game);
    m_compWorker->moveToThread(m_compThread);

    connect(m_compThread, &QThread::started, m_compWorker, &ComputerWorker::compute);
    connect(m_compWorker, &ComputerWorker::finished, this, &MainWindow::onComputerResultReady);
    connect(m_compWorker, &ComputerWorker::finished, m_compThread, &QThread::quit);
    connect(m_compThread, &QThread::finished, m_compWorker, &QObject::deleteLater);
    connect(m_compThread, &QThread::finished, m_compThread, &QObject::deleteLater);

    m_compThread->start();
}

void MainWindow::onComputerMoveTimeout()
{
    m_game->m_cancelRequested = true;
    updateStatus("Computer move cancelled (took too long)");
}

void MainWindow::onComputerResultReady(int x, int y)
{
    QApplication::restoreOverrideCursor();

    if (m_compWatchdog)
        m_compWatchdog->stop();

    m_compThread = nullptr;
    m_compWorker = nullptr;

    m_game->m_bProcessing = false;

    if (m_game->m_cancelRequested) {
        // AI was cancelled (timeout): give turn back to the user.
        m_game->m_bUsersTurn = true;
        updateStatus("");
        updateTitle();
        updateMenuStates();
        m_board->update();
        return;
    }

    m_game->applyComputerMove(x, y);

    // Start flashing so the computer's move is visible (the piece toggles
    // on/off via the flashing timer; without it the piece stays hidden).
    m_board->startFlashing();

    updateTitle();
    updateMenuStates();
    m_board->update();

    if (!m_game->m_bGameGoing) {
        updateStatus("Game over");
    } else if (m_game->isUsersTurn()) {
        updateStatus("Your turn");
    }
}

void MainWindow::onEscapePressed()
{
    // original: minimize on Escape
    showMinimized();
}

void MainWindow::onScrollTo(int x, int y, int w, int h)
{
    m_scroll->ensureVisible(x, y, w, h);
}

void MainWindow::updateTitle()
{
    QString status;
    if (!m_game->m_bGameGoing) {
        status = "Ready";
    } else if (m_game->m_bProcessing) {
        status = QString("Thinking (%1 - %2)").arg(
                    m_game->m_sAlreadyFoundRows[m_game->m_UsersPiece]).arg(
                    m_game->m_sAlreadyFoundRows[m_game->m_ComputersPiece]);
    } else {
        status = m_game->isUsersTurn() ? "Your turn" : "Computer's turn";
    }
    setWindowTitle(QString("Noughts and Crosses - %1").arg(status));
}

void MainWindow::updateStatus(const QString &msg)
{
    statusBar()->showMessage(msg);
}

void MainWindow::updateMenuStates()
{
    bool going = m_game->m_bGameGoing;
    bool processing = m_game->m_bProcessing;
    bool usersTurn = m_game->isUsersTurn();

    m_actNew->setEnabled(!processing);
    m_actUndo->setEnabled(going && !processing &&
                          (m_game->m_bHasComputerLast || m_game->m_bHasUserLast));
    m_actHint->setEnabled(going && usersTurn && !processing);

    // Skill / piece / first-move / rows only configurable between games.
    bool configurable = !going;
    for (QAction *a : m_menuSkill->actions()) a->setEnabled(configurable);
    for (QAction *a : m_menuOptions->actions()) a->setEnabled(configurable);
    m_actBackground->setEnabled(true); // always toggleable
}

void MainWindow::loadSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "0x", "0x");
    m_game->m_sGridSize = settings.value("GridSize", 16).toInt();
    m_game->m_sGridSize = std::clamp(m_game->m_sGridSize, Game::MIN_GRID_SIZE, Game::MAX_GRID_SIZE);
    m_game->m_sSkillLevel = settings.value("SkillLevel", 5).toInt();
    m_game->m_sSkillLevel = std::clamp(m_game->m_sSkillLevel, 0, 5);
    m_game->m_bComputersFirstMove = settings.value("FirstMove", 2).toInt();
    m_game->m_bComputersFirstMove = std::clamp(m_game->m_bComputersFirstMove, 0, 3);
    m_game->m_UsersPiece = settings.value("UsersPiece", 1).toInt();
    m_game->m_UsersPiece = std::clamp(m_game->m_UsersPiece, 0, 1);
    m_game->m_ComputersPiece = 1 - m_game->m_UsersPiece;
    m_game->m_bBackgroundProcessingEnabled = settings.value("BackgroundProcessing", true).toBool();
    m_game->m_sGameEndRows = settings.value("GameEndRows", 1).toInt();
    m_game->m_sGameEndRows = std::clamp(m_game->m_sGameEndRows, 1, 9);
    m_game->m_CrossRGB[0] = settings.value("CrossR", 0).toInt();
    m_game->m_CrossRGB[1] = settings.value("CrossG", 0).toInt();
    m_game->m_CrossRGB[2] = settings.value("CrossB", 255).toInt();
    m_game->m_NoughtRGB[0] = settings.value("NoughtR", 255).toInt();
    m_game->m_NoughtRGB[1] = settings.value("NoughtG", 0).toInt();
    m_game->m_NoughtRGB[2] = settings.value("NoughtB", 0).toInt();
    m_game->m_BoardRGB[0] = settings.value("BoardR", 192).toInt();
    m_game->m_BoardRGB[1] = settings.value("BoardG", 192).toInt();
    m_game->m_BoardRGB[2] = settings.value("BoardB", 192).toInt();
    m_game->updateGridSize();
}

void MainWindow::saveSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "0x", "0x");
    settings.setValue("GridSize", m_game->m_sGridSize);
    settings.setValue("SkillLevel", m_game->m_sSkillLevel);
    settings.setValue("FirstMove", m_game->m_bComputersFirstMove);
    settings.setValue("UsersPiece", m_game->m_UsersPiece);
    settings.setValue("BackgroundProcessing", m_game->m_bBackgroundProcessingEnabled);
    settings.setValue("GameEndRows", m_game->m_sGameEndRows);
    settings.setValue("CrossR", m_game->m_CrossRGB[0]);
    settings.setValue("CrossG", m_game->m_CrossRGB[1]);
    settings.setValue("CrossB", m_game->m_CrossRGB[2]);
    settings.setValue("NoughtR", m_game->m_NoughtRGB[0]);
    settings.setValue("NoughtG", m_game->m_NoughtRGB[1]);
    settings.setValue("NoughtB", m_game->m_NoughtRGB[2]);
    settings.setValue("BoardR", m_game->m_BoardRGB[0]);
    settings.setValue("BoardG", m_game->m_BoardRGB[1]);
    settings.setValue("BoardB", m_game->m_BoardRGB[2]);
}
