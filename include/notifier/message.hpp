#ifndef MESSAGE_H
#define MESSAGE_H

#include <QMessageBox>
#include <QString>

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
    Message(Severity severity, QString title, QString message, QString details = "", bool isLikelyBug = false)
        : severity(severity)
        , title(std::move(title))
        , message(std::move(message))
        , details(std::move(details))
        , isLikelyBug(isLikelyBug)
    {
    }

    const Severity severity;
    const QString title;
    const QString message;
    const QString details;
    const bool isLikelyBug;
};

#endif
