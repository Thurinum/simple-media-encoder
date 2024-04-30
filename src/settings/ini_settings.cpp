#include "ini_settings.hpp"
#include "notifier.hpp"

#include <QDir>
#include <QFile>
#include <QSettings>

IniSettings::IniSettings(const QString& fileName, const QString& defaultFileName)
{
    settings = new QSettings(fileName, QSettings::IniFormat);
    defaultSettings = new QSettings(defaultFileName, QSettings::IniFormat);
}

QVariant IniSettings::get(const QString& key)
{
    if (settings->contains(key))
        return settings->value(key);

    if (defaultSettings->contains(key)) {
        QVariant fallback = defaultSettings->value(key);
        emit problemOccured(
            { Severity::Warning,
              tr("Config fallback used"),
              tr("Configuration key '%1' was missing in '%2' and was created from the fallback value %3 in the default configuration file.")
                  .arg(key, fileName(), fallback.toString()) }
        );
        return fallback;
    }

    emit problemOccured(
        { Severity::Critical,
          tr("Config key not found"),
          tr("Configuration key '%1' is missing. Please add it manually in %2 or reinstall the program.")
              .arg(key, fileName()) }
    );
    return {};
}

QStringList IniSettings::keysInGroup(const QString& group) const
{
    QStringList keys;

    settings->beginGroup(group);
    keys = settings->childKeys();
    settings->endGroup();

    return keys;
}

QString IniSettings::fileName() const
{
    return settings->fileName();
}

void IniSettings::Set(const QString& key, const QVariant& value)
{
    if (!settings->contains(key) && !defaultSettings->contains(key))
        emit problemOccured({ Severity::Warning, tr("Config key created"), tr("A new configuration key '%1' has been created.").arg(key) });

    settings->setValue(key, value);
}
