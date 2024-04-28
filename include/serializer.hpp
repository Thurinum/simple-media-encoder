#ifndef INPUT_WIDGET_SERIALIZER_H
#define INPUT_WIDGET_SERIALIZER_H

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

class Serializer
{
public:
    Serializer(QSharedPointer<Settings> dataSource)
        : dataSource(dataSource) {};

    template <ValueWidget W>
    void serialize(W widget) { serializeBase(widget, widget->value()); }
    void serialize(QComboBox* widget);
    template <Checkable W>
    void serialize(W widget) { serializeBase(widget, widget->isChecked()); }
    void serialize(QPlainTextEdit* widget);
    void serialize(QLineEdit* widget);
    void serialize(QButtonGroup* widget);

    template <ValueWidget W>
    void deserialize(W widget) { widget->setValue(deserializeBase(widget).template value<double>()); }
    void deserialize(QComboBox* widget);
    template <Checkable W>
    void deserialize(W widget) { widget->setChecked(deserializeBase(widget).toBool()); }
    void deserialize(QPlainTextEdit* widget);
    void deserialize(QLineEdit* widget);
    void deserialize(QButtonGroup* widget);

private:
    QString key(QObject* object);
    void serializeBase(QObject* object, const QVariant& value);
    QVariant deserializeBase(QObject* object);

    QSharedPointer<Settings> dataSource;
};

#endif
