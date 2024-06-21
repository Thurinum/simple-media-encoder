#ifndef NOTIFIER_H
#define NOTIFIER_H

#include <QMessageBox>

#include "notifier/message.hpp"

class Notifier
{
public:
    virtual void Notify(const Message& message) const = 0;
    virtual void Notify(Severity severity, const QString& title, const QString& message, const QString& details = "") const = 0;
};

#endif
