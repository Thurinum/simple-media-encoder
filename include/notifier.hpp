#ifndef MESSAGE_H
#define MESSAGE_H

#include <QMessageBox>

// TODO: Move to service

enum Severity {
    Info = QMessageBox::Information,
    Warning = QMessageBox::Warning,
    Error = QMessageBox::Critical,
    Critical // should terminate program
};

//!
//! \brief Represents a message to be displayed to the user.
//!
struct Message {
public:
    Message(Severity severity, QString title, QString message, QString details = "")
        : severity(severity)
        , title(title)
        , message(message)
        , details(details)
    {
    }

    const Severity severity;
    const QString title;
    const QString message;
    const QString details;
};

class Notifier
{
public:
    virtual void Notify(const Message& message) const = 0;
    virtual void Notify(Severity severity, const QString& title, const QString& message, const QString& details = "") const = 0;
};

class MessageBoxNotifier : public Notifier
{
public:
    void Notify(const Message& message) const override;
    void Notify(Severity severity, const QString& title, const QString& message, const QString& details) const override;
};

#endif
