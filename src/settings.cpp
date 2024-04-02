#include "settings.hpp"

#include <QFile>

QVariant Settings::get(const QString& key)
{
    if (!m_isInit) {
        emit notInitialized();
        return {};
    }

    if (m_settings->contains(key))
        return m_settings->value(key);

    if (m_defaultSettings->contains(key)) {
        QVariant fallback = m_defaultSettings->value(key);
        emit keyFallbackUsed(key, fallback);
        return fallback;
    }

    emit keyNotFound(key);
    return {};
}

QStringList Settings::keysInGroup(const QString& group)
{
    if (!m_isInit) {
        emit notInitialized();
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
        emit notInitialized();
        return;
    }

    if (!m_settings->contains(key) && !m_defaultSettings->contains(key))
        emit keyCreated(key);

    m_settings->setValue(key, value);
}

void Settings::Init(const QString& fileName)
{
    QString defaultFileName = fileName + "_default.ini";
    QString currentFileName = fileName + ".ini";

    if (!QFile::exists(defaultFileName)) {
        emit configNotFound(defaultFileName);
        return;
    }

    if (!QFile::exists(currentFileName))
        QFile::copy(defaultFileName, currentFileName);

    m_defaultSettings = new QSettings(defaultFileName, QSettings::IniFormat);
    m_settings = new QSettings(currentFileName, QSettings::IniFormat);
    m_isInit = true;
}
