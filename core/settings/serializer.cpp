#include "serializer.hpp"

#include <QAbstractButton>

void Serializer::serialize(QComboBox* widget)
{
    serializeBase(widget, widget->currentText());
}

void Serializer::serialize(QPlainTextEdit* widget)
{
    serializeBase(widget, widget->toPlainText());
}

void Serializer::serialize(QLineEdit* widget)
{
    serializeBase(widget, widget->text());
}

void Serializer::serialize(QButtonGroup* widget)
{
    serializeBase(widget, widget->checkedId());
}

void Serializer::deserialize(QComboBox* widget)
{
    widget->setCurrentText(deserializeBase(widget).toString());
}

void Serializer::deserialize(QPlainTextEdit* widget)
{
    widget->setPlainText(deserializeBase(widget).toString());
}

void Serializer::deserialize(QLineEdit* widget)
{
    widget->setText(deserializeBase(widget).toString());
}

void Serializer::deserialize(QButtonGroup* widget)
{
    QAbstractButton* checkedButton = widget->button(deserializeBase(widget).toInt());

    if (!checkedButton)
        return;

    checkedButton->setChecked(true);
}

QString Serializer::key(QObject* object)
{
    return QString("LastDesired/%1").arg(object->objectName());
}

void Serializer::serializeBase(QObject* object, const QVariant& value)
{
    if (!dataSource)
        return;

    dataSource->Set(key(object), value);
}

QVariant Serializer::deserializeBase(QObject* object)
{
    if (!dataSource)
        return {};

    return dataSource->get(key(object));
}
