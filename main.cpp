#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("noughtsandcrosses");
    app.setOrganizationName("0x");

    MainWindow window;
    window.show();
    return app.exec();
}
