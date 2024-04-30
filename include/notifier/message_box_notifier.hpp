#ifndef MESSAGE_BOX_NOTIFIER_H
#define MESSAGE_BOX_NOTIFIER_H

#include "notifier.hpp"

class MessageBoxNotifier : public Notifier
{
public:
    void Notify(const Message& message) const override;
    void Notify(Severity severity, const QString& title, const QString& message, const QString& details) const override;

private:
    const QString bugReportPrompt = "<br><br>Looks like this issue could be a bug. Kindly report it <a href='https://github.com/Thurinum/simple-media-encoder/issues/new'>here</a>.";
};

#endif
