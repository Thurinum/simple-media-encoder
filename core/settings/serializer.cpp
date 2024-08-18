#include "serializer.hpp"

#include <QCheckBox>
#include <QDial>
#include <QGroupBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QSpinBox>

class QCheckBox;
void Serializer::serialize(const QObject* widget, const std::shared_ptr<Settings>& dataSource, const QString& dataSourceKey) const
{
    if (!dataSource)
        return;

    const QString key = getKey(widget, dataSourceKey);

    dataSource->Set(key, getWidgetValue(widget));
}

void Serializer::deserialize(QObject* widget, const std::shared_ptr<Settings>& dataSource, const QString& dataSourceKey) const
{
    if (!dataSource)
        return;

    const QString key = getKey(widget, dataSourceKey);
    const QVariant value = dataSource->get(key);
    setWidgetValue(widget, value);
}

void Serializer::serializeMany(const QList<QObject*>& widgets, const std::shared_ptr<Settings>& dataSource, const QString& dataSourceKey) const
{
    for (const QObject* widget : widgets)
        serialize(widget, dataSource, dataSourceKey);
}

void Serializer::deserializeMany(const QList<QObject*>& widgets, const std::shared_ptr<Settings>& dataSource, const QString& dataSourceKey) const
{
    for (QObject* widget : widgets)
        deserialize(widget, dataSource, dataSourceKey);
}

QString Serializer::getKey(const QObject* widget, const QString& dataSourceKey) const
{
    return QString("%1/%2").arg(dataSourceKey, widget->objectName());
}

QVariant Serializer::getWidgetValue(const QObject* widget) const
{
    if (const auto* comboBox = qobject_cast<const QComboBox*>(widget))
        return comboBox->currentText();

    if (const auto* plainTextEdit = qobject_cast<const QPlainTextEdit*>(widget))
        return plainTextEdit->toPlainText();

    if (const auto* lineEdit = qobject_cast<const QLineEdit*>(widget))
        return lineEdit->text();

    if (const auto* buttonGroup = qobject_cast<const QButtonGroup*>(widget))
        return buttonGroup->checkedId();

    if (const auto* spinBox = qobject_cast<const QSpinBox*>(widget))
        return spinBox->value();

    if (const auto* doubleSpinBox = qobject_cast<const QDoubleSpinBox*>(widget))
        return doubleSpinBox->value();

    if (const auto* slider = qobject_cast<const QSlider*>(widget))
        return slider->value();

    if (const auto* dial = qobject_cast<const QDial*>(widget))
        return dial->value();

    if (const auto* progressBar = qobject_cast<const QProgressBar*>(widget))
        return progressBar->value();

    if (const auto* checkbox = qobject_cast<const QCheckBox*>(widget))
        return checkbox->isChecked();

    if (const auto* groupBox = qobject_cast<const QGroupBox*>(widget))
        return groupBox->isChecked();

    const std::string error = std::format("Widget '{}' has unsupported type '{}' for serialization.", widget->objectName().toStdString(), widget->metaObject()->className());
    throw std::runtime_error(error);
}
void Serializer::setWidgetValue(QObject* widget, const QVariant& value) const
{
    if (auto* comboBox = qobject_cast<QComboBox*>(widget))
    {
        comboBox->setCurrentText(value.toString());
    }
    else if (auto* plainTextEdit = qobject_cast<QPlainTextEdit*>(widget))
    {
        plainTextEdit->setPlainText(value.toString());
    }
    else if (auto* lineEdit = qobject_cast<QLineEdit*>(widget))
    {
        lineEdit->setText(value.toString());
    }
    else if (const auto* buttonGroup = qobject_cast<QButtonGroup*>(widget))
    {
        if (QAbstractButton* checkedButton = buttonGroup->button(value.toInt()))
        {
            checkedButton->setChecked(true);
        }
    }
    else if (auto* spinBox = qobject_cast<QSpinBox*>(widget))
    {
        spinBox->setValue(value.toInt());
    }
    else if (auto* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(widget))
    {
        doubleSpinBox->setValue(value.toDouble());
    }
    else if (auto* slider = qobject_cast<QSlider*>(widget))
    {
        slider->setValue(value.toInt());
    }
    else if (auto* dial = qobject_cast<QDial*>(widget))
    {
        dial->setValue(value.toInt());
    }
    else if (auto* progressBar = qobject_cast<QProgressBar*>(widget))
    {
        progressBar->setValue(value.toInt());
    }
    else if (auto* checkbox = qobject_cast<QCheckBox*>(widget))
    {
        checkbox->setChecked(value.toBool());
    }
    else if (auto* groupBox = qobject_cast<QGroupBox*>(widget))
    {
        groupBox->setChecked(value.toBool());
    }
    else
    {
        const std::string error = std::format("Widget '{}' has unsupported type '{}' for deserialization.", widget->objectName().toStdString(), widget->metaObject()->className());
        throw std::runtime_error(error);
    }
}
