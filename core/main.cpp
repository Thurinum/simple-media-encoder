#include "encoder/encoder.hpp"
#include "formats/ffmpeg_format_support_loader.hpp"
#include "mainwindow.hpp"
#include "notifier/message_box_notifier.hpp"
#include "settings/serializer.hpp"
#include "settings/settings_factory.hpp"
#include "thirdparty/boost-di/di.hpp"

namespace di = boost::di;

#include <QApplication>

bool createSettings(std::shared_ptr<Settings>& settings) {
    MessageBoxNotifier notifier;

    const auto maybeSettings = SettingsFactory::createIniSettings("config.ini", "config_default.ini");
    if (std::holds_alternative<Message>(maybeSettings)) {
        notifier.Notify(std::get<Message>(maybeSettings));
        return false;
    }

    settings = std::get<std::shared_ptr<Settings>>(maybeSettings);
    auto settingsConnection = QObject::connect(settings.get(), &Settings::problemOccured, [notifier](const Message& problem) {
        notifier.Notify(problem);
    });

    return true;
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Simple Media Converter");
    app.setStyle("Fusion");

    std::shared_ptr<Settings> settings;
    if (!createSettings(settings))
        return EXIT_FAILURE;

    const auto injector = make_injector(
        di::bind<Settings>.to(settings),
        di::bind<Notifier>.to<MessageBoxNotifier>(),
        di::bind<FormatSupportLoader>.to<FFmpegFormatSupportLoader>()
    );

    const auto w = injector.create<std::shared_ptr<MainWindow>>();
    w->setWindowIcon(QIcon("appicon.ico"));
    w->show();

    return app.exec();
}
