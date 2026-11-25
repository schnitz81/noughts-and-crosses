#ifndef COMPUTERWORKER_H
#define COMPUTERWORKER_H

#include <QObject>

class Game;

/*
 * Runs the computer's AI search (Game::computeComputerMove) on a worker
 * thread so the GUI stays responsive while the computer is "thinking".
 */
class ComputerWorker : public QObject {
    Q_OBJECT
public:
    explicit ComputerWorker(Game *game, QObject *parent = nullptr);

public slots:
    void compute();

signals:
    void finished(int x, int y);   // best move found

private:
    Game *m_game;
};

#endif // COMPUTERWORKER_H
