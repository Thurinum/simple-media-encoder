#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include "notifier.hpp"

#include <QObject>
#include <QSettings>
#include <QString>

class Settings : public QObject
{
    Q_OBJECT
public:
    void Init(const QString& fileName);

    QVariant get(const QString& key);
    void Set(const QString& key, const QVariant& value);

    QStringList keysInGroup(const QString& group);
    QString fileName();

signals:
    void problemOccured(const Message& problem);

private:
    QSettings* m_settings;
    QSettings* m_defaultSettings;

    bool m_isInit = false;
};

#endif // SETTINGS_HPP
