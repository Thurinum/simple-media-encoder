#ifndef WARNINGS_HPP
#define WARNINGS_HPP

#include <QHash>
#include <QString>
#include <QWidget>

class Warnings
{
public:
    Warnings(QWidget* widget);

    void Add(const QString& key, const QString& text);
    void Remove(const QString& key);

private:
    void UpdateWidget() const;

    QWidget* m_tooltipWidget;
    QHash<QString, QString> m_warnings;
};

#endif // WARNINGS_HPP
