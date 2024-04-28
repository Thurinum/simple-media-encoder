#ifndef SETTINGS_FACTORY_H
#define SETTINGS_FACTORY_H

#include <QPointer>
#include <variant>

class Settings;
class QString;
class Message;

class SettingsFactory
{
public:
    static std::variant<QSharedPointer<Settings>, Message> createIniSettings(const QString& fileName, const QString& defaultFileName);
};

#endif
