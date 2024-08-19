#include "encoder/encoder.hpp"
#include "formats/ffmpeg_format_support_loader.hpp"
#include "mainwindow.hpp"
#include "notifier/message_box_notifier.hpp"
#include "settings/ini_settings.hpp"
#include "settings/serializer.hpp"
#include "thirdparty/boost-di/di.hpp"

namespace di = boost::di;

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Simple Media Encoder");
    app.setStyle("Fusion");

    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QDir::current().absolutePath());

    const auto injector = make_injector(
        di::bind<Settings>.named(di_settings).to([]
                                                 { return std::make_shared<IniSettings>("config.ini", "config_default.ini"); }),
        di::bind<Settings>.named(di_presets).to([]
                                                { return std::make_shared<IniSettings>("presets.ini"); }),
        di::bind<Notifier>.to<MessageBoxNotifier>(), di::bind<FormatSupportLoader>.to<FFmpegFormatSupportLoader>()
    );

    const auto w = injector.create<std::shared_ptr<MainWindow>>();
    w->setWindowIcon(QIcon("appicon.ico"));
    w->show();

    return app.exec();
}
