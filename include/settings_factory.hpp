#ifndef SETTINGS_FACTORY_H
#define SETTINGS_FACTORY_H

#include "either.hpp"
#include <QPointer>

class Settings;
class QString;
class Message;

class SettingsFactory
{
public:
    static Either<QSharedPointer<Settings>, Message> createIniSettings(const QString& fileName, const QString& defaultFileName);
};

#endif
