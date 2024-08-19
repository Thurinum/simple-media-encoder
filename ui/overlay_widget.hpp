#pragma once

#include <QGraphicsEffect>
#include <QPainter>
#include <QPropertyAnimation>
#include <QWidget>

/*!
 * \brief A widget that displays a semi-transparent overlay with centered text.
 */
class OverlayWidget final : public QWidget
{
public:
    explicit OverlayWidget(QWidget* parent = nullptr)
        : QWidget(parent)
        , text("")
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);

        opacityEffect = new QGraphicsOpacityEffect(this);
        setGraphicsEffect(opacityEffect);

        fadeAnimation = new QPropertyAnimation(opacityEffect, "opacity", this);
        fadeAnimation->setDuration(300);
    }

    void setText(const QString& text)
    {
        this->text = text;
        update();
    }

    void setBackgroundColor(const QColor& color)
    {
        this->backgroundColor = color;
        update();
    }

    void showWithFade()
    {
        show();
        fadeAnimation->setStartValue(0.0);
        fadeAnimation->setEndValue(1.0);
        disconnect(onFadeOutFinished);
        fadeAnimation->start();
    }

    void hideWithFade()
    {
        fadeAnimation->setStartValue(1.0);
        fadeAnimation->setEndValue(0.0);
        disconnect(onFadeOutFinished);
        onFadeOutFinished = connect(fadeAnimation, &QPropertyAnimation::finished, this, &OverlayWidget::hide);
        fadeAnimation->start();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), backgroundColor);

        QFont font = painter.font();
        font.setPointSize(32);
        painter.setPen(Qt::white);
        painter.setFont(font);

        const QRect textRect = painter.boundingRect(rect(), Qt::AlignCenter, text);
        painter.drawText(textRect, Qt::AlignCenter, text);
    }

private:
    QString text;
    QColor backgroundColor;
    QGraphicsOpacityEffect* opacityEffect;
    QPropertyAnimation* fadeAnimation;

    QMetaObject::Connection onFadeOutFinished;
};