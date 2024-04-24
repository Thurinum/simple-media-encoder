#include "settings_factory.hpp"

#include "ini_settings.hpp"

#include <QDir>
#include <QFile>
#include <QSettings>

Either<QPointer<Settings>, Message> SettingsFactory::createIniSettings(const QString& fileName, const QString& defaultFileName)
{
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QDir::current().absolutePath());

    if (!QFile::exists(defaultFileName)) {
        return Message {
            Severity::Critical,
            QObject::tr("Config not found"),
            QObject::tr("Default configuration file '%1' is missing. Please reinstall the program.").arg(defaultFileName)
        };
    }

    if (!QFile::exists(fileName))
        QFile::copy(defaultFileName, fileName);

    return QPointer<Settings>(new IniSettings(fileName, defaultFileName));
}
