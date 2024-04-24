#include "either.hpp"
#include "encoder.hpp"
#include "mainwindow.hpp"
#include "platform_info.hpp"
#include "settings_factory.hpp"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Efficient Media Encoder");
    app.setStyle("Fusion");

    MediaEncoder* encoder = new MediaEncoder(&app);
    const PlatformInfo platformInfo;
    const MessageBoxNotifier notifier;

    auto maybeSettings = SettingsFactory::createIniSettings("config.ini", "config_default.ini");
    if (maybeSettings.isLeft) {
        notifier.Notify(maybeSettings.getLeft());
        return EXIT_FAILURE;
    }

    const QPointer<Settings> settings = maybeSettings.getRight();
    QObject::connect(settings, &Settings::problemOccured, [notifier](const Message& problem) {
        notifier.Notify(problem);
    });

    MainWindow w(*encoder, settings, notifier, platformInfo);
    w.setWindowIcon(QIcon("appicon.ico"));
    w.show();

    return app.exec();
}
