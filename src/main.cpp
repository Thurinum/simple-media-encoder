#include "encoder.hpp"
#include "mainwindow.hpp"
#include "message_box_notifier.hpp"
#include "platform_info.hpp"
#include "serializer.hpp"
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
    if (std::holds_alternative<Message>(maybeSettings)) {
        notifier.Notify(std::get<Message>(maybeSettings));
        return EXIT_FAILURE;
    }

    const QSharedPointer<Settings> settings = std::get<QSharedPointer<Settings>>(maybeSettings);
    QObject::connect(settings.get(), &Settings::problemOccured, [notifier](const Message& problem) {
        notifier.Notify(problem);
    });

    const QSharedPointer<Serializer> serializer(new Serializer(settings));

    MainWindow w(*encoder, settings, serializer, notifier, platformInfo);
    w.setWindowIcon(QIcon("appicon.ico"));
    w.show();

    return app.exec();
}
