#ifndef INISETTINGS_H
#define INISETTINGS_H

#include "settings.hpp"

#include <QObject>
#include <QPointer>

class QSettings;
class QString;

class IniSettings : public Settings
{
    Q_OBJECT

public:
    IniSettings(const QString& fileName, const QString& defaultFileName);

    QVariant get(const QString& key) override;
    void Set(const QString& key, const QVariant& value) override;

    QStringList keysInGroup(const QString& group) const override;
    QString fileName() const override;

private:
    QPointer<QSettings> settings;
    QPointer<QSettings> defaultSettings;
};

#endif // INISETTINGS_H
