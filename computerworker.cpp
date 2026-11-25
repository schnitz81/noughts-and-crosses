#include "computerworker.h"
#include "board.h"

ComputerWorker::ComputerWorker(Game *game, QObject *parent)
    : QObject(parent), m_game(game)
{
}

void ComputerWorker::compute()
{
    ValuePoint r;
    m_game->computeComputerMove(&r);
    emit finished(r.point.x(), r.point.y());
}
