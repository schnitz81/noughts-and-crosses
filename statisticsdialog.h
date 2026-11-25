#ifndef STATISTICSDIALOG_H
#define STATISTICSDIALOG_H

#include <QDialog>

class StatisticsDialog : public QDialog {
    Q_OBJECT
public:
    StatisticsDialog(int wins, int losses, QWidget *parent = nullptr);
};

#endif // STATISTICSDIALOG_H
