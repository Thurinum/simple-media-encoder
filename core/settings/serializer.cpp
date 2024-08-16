#include "serializer.hpp"

#include <QAbstractButton>

void Serializer::serialize(const QComboBox* widget) const {
    serializeBase(widget, widget->currentText());
}

void Serializer::serialize(const QPlainTextEdit* widget) const {
    serializeBase(widget, widget->toPlainText());
}

void Serializer::serialize(const QLineEdit* widget) const {
    serializeBase(widget, widget->text());
}

void Serializer::serialize(const QButtonGroup* widget) const {
    serializeBase(widget, widget->checkedId());
}

void Serializer::deserialize(QComboBox* widget) const {
    widget->setCurrentText(deserializeBase(widget).toString());
}

void Serializer::deserialize(QPlainTextEdit* widget) const {
    widget->setPlainText(deserializeBase(widget).toString());
}

void Serializer::deserialize(QLineEdit* widget) const {
    widget->setText(deserializeBase(widget).toString());
}

void Serializer::deserialize(const QButtonGroup* widget) const {
    QAbstractButton* checkedButton = widget->button(deserializeBase(widget).toInt());

    if (!checkedButton)
        return;

    checkedButton->setChecked(true);
}

QString Serializer::key(const QObject* object)
{
    return QString("LastDesired/%1").arg(object->objectName());
}

void Serializer::serializeBase(const QObject* object, const QVariant& value) const {
    if (!dataSource)
        return;

    dataSource->Set(key(object), value);
}

QVariant Serializer::deserializeBase(const QObject* object) const {
    if (!dataSource)
        return {};

    return dataSource->get(key(object));
}
