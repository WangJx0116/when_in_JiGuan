#include "mainwigt.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    mainwigt w;
    w.show();

    return a.exec();
}
