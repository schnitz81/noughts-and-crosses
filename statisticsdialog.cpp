#include "statisticsdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <cmath>

StatisticsDialog::StatisticsDialog(int wins, int losses, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Statistics");

    auto *layout = new QVBoxLayout(this);

    int total = wins + losses;
    int percent = 0;
    if (total > 0)
        percent = static_cast<int>(std::floor(0.5 + 100.0 * wins / total));

    auto addRow = [&](const QString &label, const QString &value) {
        auto *h = new QHBoxLayout;
        h->addWidget(new QLabel(label));
        h->addStretch();
        h->addWidget(new QLabel(value));
        layout->addLayout(h);
    };

    layout->addWidget(new QLabel("<b>Statistics</b>"));
    addRow("Wins:", QString::number(wins));
    addRow("Losses:", QString::number(losses));
    addRow("Wins / all:", QString::number(percent) + "%");

    QDialogButtonBox *buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, this);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
