#include "aboutdialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("About Noughts and Crosses");

    auto *layout = new QVBoxLayout(this);

    QLabel *text = new QLabel(
        "<h2>Noughts and Crosses</h2>"
        "<p>Version 1.1</p>"
        "<p>Copyright &copy; 1993-1994 Pentti Moilanen</p>"
        "<p>Ported to Qt for the Linux desktop.</p>"
        "<p>This program is free software; you can redistribute it and/or "
        "modify it under the terms of the GNU General Public License as "
        "published by the Free Software Foundation; either version 2 of the "
        "License, or (at your option) any later version.</p>");
    text->setTextFormat(Qt::RichText);
    text->setWordWrap(true);
    layout->addWidget(text);

    QDialogButtonBox *buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, this);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
