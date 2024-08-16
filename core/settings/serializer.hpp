#pragma once

#include "settings.hpp"

#include <QButtonGroup>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>

// QSpinBox, QDoubleSpinBox, QSlider, QDial, QProgressBar
template <typename T>
concept ValueWidget = requires(T t) {
    {
        t->value()
    } -> std::convertible_to<QVariant>;
    {
        t->setValue(0)
    } -> std::same_as<void>;
};

// QCheckBox and QGroupBox
template <typename T>
concept Checkable = requires(T t) {
    {
        t->isChecked()
    } -> std::same_as<bool>;
    {
        t->setChecked(true)
    } -> std::same_as<void>;
};

/*!
 * \brief Provides a unified way to serialize and deserialize widgets from/to a data source.
 * \details For some reason, Qt input widgets don't implement a common interface for getting and setting their values.
 * That makes streamlining the process of saving and loading state a very inelegant.
 */
class Serializer
{
public:
    template <ValueWidget W>
    void serialize(W widget) { serializeBase(widget, widget->value()); }
    void serialize(const QComboBox* widget) const;
    template <Checkable W>
    void serialize(W widget) { serializeBase(widget, widget->isChecked()); }
    void serialize(const QPlainTextEdit* widget) const;
    void serialize(const QLineEdit* widget) const;
    void serialize(const QButtonGroup* widget) const;

    template <ValueWidget W>
    void deserialize(W widget) { widget->setValue(deserializeBase(widget).template value<double>()); }
    void deserialize(QComboBox* widget) const;
    template <Checkable W>
    void deserialize(W widget) { widget->setChecked(deserializeBase(widget).toBool()); }
    void deserialize(QPlainTextEdit* widget) const;
    void deserialize(QLineEdit* widget) const;
    void deserialize(const QButtonGroup* widget) const;

private:
    static QString key(const QObject* object);
    void serializeBase(const QObject* object, const QVariant& value) const;
    QVariant deserializeBase(const QObject* object) const;

    std::shared_ptr<Settings> dataSource;
};
