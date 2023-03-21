#include "sane_base.h"
#include <QCoreApplication>
#include <QDebug>
#include <sane/saneopts.h>

using namespace QSane;

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);
    // init lib first
    SDevive::init();
    /* using SDevice to open scanner */
    SScanner* brother = SDevive::open("brother4:bus1;dev6");
    brother->setResolution(200);
    brother->setMode("Black & White");

    brother->setPaperSizeA4();

    brother->scan("/var/tmp/test.tif");

    if (brother)
        delete brother;

    // close all resource
    SDevive::exit();

    qDebug() << "end of code";
    return a.exec();
}
