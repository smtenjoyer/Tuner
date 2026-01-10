#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qApp->setWindowIcon(QIcon(":/image/music.png"));
    MainWindow w;
    w.show();
    return a.exec();
}
