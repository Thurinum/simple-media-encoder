#include "mainwindow.hpp"
#include "platform_info.hpp"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Efficient Media Encoder");
    app.setStyle("Fusion");

    PlatformInfo platformInfo;

    MainWindow w(platformInfo);
    w.setWindowIcon(QIcon("appicon.ico"));
    w.show();

    return app.exec();
}
