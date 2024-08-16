#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>

class Message;

class Settings : public QObject
{
    Q_OBJECT

public:
    virtual QVariant get(const QString& key) = 0;
    virtual void Set(const QString& key, const QVariant& value) = 0;

    virtual QStringList keysInGroup(const QString& group) const = 0;
    virtual QString fileName() const = 0;

signals:
    void problemOccured(const Message& problem);
};

Q_DECLARE_INTERFACE(Settings, "Settings")

#endif
