#ifndef MESSAGE_BOX_NOTIFIER_H
#define MESSAGE_BOX_NOTIFIER_H

#include "notifier.hpp"

class MessageBoxNotifier : public Notifier
{
public:
    void Notify(const Message& message) const override;
    void Notify(Severity severity, const QString& title, const QString& message, const QString& details) const override;
};

#endif
