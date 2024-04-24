#include "notifier.hpp"

#include <QApplication>

void MessageBoxNotifier::Notify(const Message& message) const
{
    QFont font;
    font.setBold(true);

    QMessageBox dialog;
    dialog.setWindowTitle(QApplication::applicationName());
    dialog.setIcon(message.severity == Severity::Critical ? QMessageBox::Critical
                                                          : (QMessageBox::Icon)message.severity);
    dialog.setText("<font size=5><b>" + message.title + ".</b></font>");
    dialog.setInformativeText(message.message);
    dialog.setDetailedText(message.details);
    dialog.setStandardButtons(QMessageBox::Ok);
    dialog.exec();

    // wait until event loop has begun before attempting to exit
    if (message.severity == Severity::Critical) {
        QMetaObject::invokeMethod(QApplication::instance(), "exit", Qt::QueuedConnection);
        QApplication::exit();
    }
}

void MessageBoxNotifier::Notify(Severity severity, const QString& title, const QString& message, const QString& details) const
{
    QFont font;
    font.setBold(true);

    QMessageBox dialog;
    dialog.setWindowTitle(QApplication::applicationName());
    dialog.setIcon(severity == Severity::Critical ? QMessageBox::Critical
                                                  : (QMessageBox::Icon)severity);
    dialog.setText("<font size=5><b>" + title + ".</b></font>");
    dialog.setInformativeText(message);
    dialog.setDetailedText(details);
    dialog.setStandardButtons(QMessageBox::Ok);
    dialog.exec();

    // wait until event loop has begun before attempting to exit
    if (severity == Severity::Critical) {
        QMetaObject::invokeMethod(QApplication::instance(), "exit", Qt::QueuedConnection);
        QApplication::exit();
    }
}
