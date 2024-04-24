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
//! \brief A simple immutable message that invokes tr() upon construction.
//!
struct Message {
public:
    Message(Severity severity, const char* title, const char* message, const char* details = "")
        : severity(severity)
        , title(QObject::tr(title))
        , message(QObject::tr(message))
        , details(QObject::tr(details))
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
    void Notify(const Message& message) const override;
    void Notify(Severity severity, const QString& title, const QString& message, const QString& details) const override;
};

#endif
