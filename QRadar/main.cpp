#include "widget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;

    // Default to COM8; command-line can override:  QRadar.exe COM9
    QString preferredPort = "COM8";
    if (argc >= 2)
        preferredPort = QString::fromLocal8Bit(argv[1]).toUpper();
    w.setPreferredPort(preferredPort);

    w.setWindowTitle("STM32 Ultrasonic Radar");
    w.show();
    return a.exec();
}
