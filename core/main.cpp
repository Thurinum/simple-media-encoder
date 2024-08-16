#include "encoder/encoder.hpp"
#include "formats/ffmpeg_format_support_loader.hpp"
#include "mainwindow.hpp"
#include "notifier/message_box_notifier.hpp"
#include "settings/serializer.hpp"
#include "settings/settings_factory.hpp"
#include "utils/platform_info.hpp"
#include "thirdparty/boost-di/di.hpp"

namespace di = boost::di;

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Simple Media Converter");
    app.setStyle("Fusion");

    PlatformInfo platformInfo;
    MetadataLoader metadataLoader(platformInfo);
    MediaEncoder encoder(&app);
    MessageBoxNotifier notifier;

    auto maybeSettings = SettingsFactory::createIniSettings("config.ini", "config_default.ini");
    if (std::holds_alternative<Message>(maybeSettings)) {
        notifier.Notify(std::get<Message>(maybeSettings));
        return EXIT_FAILURE;
    }

    const std::shared_ptr<Settings> settings = std::get<std::shared_ptr<Settings>>(maybeSettings);
    QObject::connect(settings.get(), &Settings::problemOccured, [notifier](const Message& problem) {
        notifier.Notify(problem);
    });

    const auto serializer = std::make_shared<Serializer>(settings);

    FFmpegFormatSupportLoader formats;

    const auto injector = make_injector(
        di::bind<MediaEncoder>.to(encoder),
        di::bind<Settings>.to(settings),
        di::bind<Serializer>.to(serializer),
        di::bind<MetadataLoader>.to(metadataLoader),
        di::bind<Notifier>.to(notifier),
        di::bind<PlatformInfo>.to(platformInfo),
        di::bind<FormatSupportLoader>.to(formats)
    );

    auto w = injector.create<std::shared_ptr<MainWindow>>();
    w->setWindowIcon(QIcon("appicon.ico"));
    w->show();

    return app.exec();
}
