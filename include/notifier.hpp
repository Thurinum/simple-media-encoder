#ifndef MESSAGE_H
#define MESSAGE_H

#include <QMessageBox>

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
        , title(std::move(title))
        , message(std::move(message))
        , details(std::move(details))
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

#endif
