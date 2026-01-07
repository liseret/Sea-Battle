#include "mainwindow.h"
#include <QIcon>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QString iconPath;
#ifdef Q_OS_WINDOWS
    iconPath = ":/icons/icon.ico";
#else
    iconPath = ":/icons/icon.png";
#endif
    a.setWindowIcon(QIcon(":new/prefix1/icons/icon.png"));
    MainWindow w;
    w.show();
    return a.exec();
}
