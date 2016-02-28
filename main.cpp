#include "mixerwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MixerWindow w;
    w.show();

    return a.exec();
}
