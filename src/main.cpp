#include "encoder.hpp"
#include "mainwindow.hpp"
#include "platform_info.hpp"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Efficient Media Encoder");
    app.setStyle("Fusion");

    MediaEncoder* encoder = new MediaEncoder(&app);
    PlatformInfo platformInfo;

    MainWindow w(*encoder, platformInfo);
    w.setWindowIcon(QIcon("appicon.ico"));
    w.show();

    return app.exec();
}
