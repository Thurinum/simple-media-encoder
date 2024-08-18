#include "ini_settings.hpp"

#include <QDir>
#include <QFile>
#include <QSettings>

IniSettings::IniSettings(const QString& fileName, const QString& defaultFileName)
{
    if (const QFile file(defaultFileName); file.exists()) {
        defaultSettings = new QSettings(defaultFileName, QSettings::IniFormat);
    }

    settings = new QSettings(fileName, QSettings::IniFormat);
}

QVariant IniSettings::get(const QString& key) const
{
    if (settings->contains(key))
        return settings->value(key);

    if (!defaultSettings.isNull() && defaultSettings->contains(key))
        return defaultSettings->value(key);

    return {};
}

QStringList IniSettings::keysInGroup(const QString& group) const
{
    settings->beginGroup(group);
    QStringList keys = settings->childKeys();
    settings->endGroup();

    return keys;
}

QString IniSettings::fileName() const
{
    return settings->fileName();
}

void IniSettings::Set(const QString& key, const QVariant& value)
{
    settings->setValue(key, value);
}

QStringList IniSettings::groups() const
{
    return settings->childGroups();
}
