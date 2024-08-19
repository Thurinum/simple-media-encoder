#pragma once

#include "settings.hpp"

#include <QButtonGroup>
#include <QComboBox>
#include <QPlainTextEdit>

/*!
 * \brief Provides a unified way to serialize and deserialize widgets from/to a data source.
 * \details For some reason, Qt's input widgets don't implement a common interface for getting and setting their values. This class abstracts widget state persistence.
 * \remark Throughout the class, QObject is used instead of QWidget because QButtonGroup inherits the former. It will fail at runtime if a widget's type is not supported.
 */
class Serializer
{
public:
    void serialize(const QObject* widget, const std::shared_ptr<Settings>& dataSource, const QString& dataSourceKey) const;
    void deserialize(QObject* widget, const std::shared_ptr<Settings>& dataSource, const QString& dataSourceKey) const;

    void serializeMany(const QList<QObject*>& widgets, const std::shared_ptr<Settings>& dataSource, const QString& dataSourceKey) const;
    void deserializeMany(const QList<QObject*>& widgets, const std::shared_ptr<Settings>& dataSource, const QString& dataSourceKey) const;

private:
    QString getKey(const QObject* widget, const QString& dataSourceKey) const;
    QVariant getWidgetValue(const QObject* widget) const;
    void setWidgetValue(QObject* widget, const QVariant& value) const;
};
