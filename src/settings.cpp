#include "settings.hpp"

#include <QDir>
#include <QFile>

QVariant Settings::get(const QString& key)
{
    if (!m_isInit) {
        emit problemOccured(
            { Severity::Critical,
              tr("Config not initialized"),
              tr("Configuration has not been initialized. Please contact the developers.") }
        );
        return {};
    }

    if (m_settings->contains(key))
        return m_settings->value(key);

    if (m_defaultSettings->contains(key)) {
        QVariant fallback = m_defaultSettings->value(key);
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

QStringList Settings::keysInGroup(const QString& group)
{
    if (!m_isInit) {
        emit problemOccured(
            { Severity::Critical,
              tr("Config not initialized"),
              tr("Configuration has not been initialized. Please contact the developers.") }
        );
        return {};
    }

    QStringList keys;

    m_settings->beginGroup(group);
    keys = m_settings->childKeys();
    m_settings->endGroup();

    return keys;
}

QString Settings::fileName()
{
    return m_settings->fileName();
}

void Settings::Set(const QString& key, const QVariant& value)
{
    if (!m_isInit) {
        emit problemOccured(
            { Severity::Critical,
              tr("Config not initialized"),
              tr("Configuration has not been initialized. Please contact the developers.") }
        );
        return;
    }

    if (!m_settings->contains(key) && !m_defaultSettings->contains(key))
        emit problemOccured({ Severity::Warning, tr("Config key created"), tr("A new configuration key '%1' has been created.").arg(key) });

    m_settings->setValue(key, value);
}

void Settings::Init(const QString& fileName)
{
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QDir::current().absolutePath());

    QString defaultFileName = fileName + "_default.ini";
    QString currentFileName = fileName + ".ini";

    if (!QFile::exists(defaultFileName)) {
        emit problemOccured({ Severity::Critical, tr("Config not found"), tr("Default configuration file '%1' is missing. Please reinstall the program.").arg(defaultFileName) });
        return;
    }

    if (!QFile::exists(currentFileName))
        QFile::copy(defaultFileName, currentFileName);

    m_defaultSettings = new QSettings(defaultFileName, QSettings::IniFormat);
    m_settings = new QSettings(currentFileName, QSettings::IniFormat);
    m_isInit = true;
}
