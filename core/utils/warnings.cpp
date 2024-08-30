#include "warnings.hpp"

Warnings::Warnings(QWidget* widget)
    : m_tooltipWidget { widget }
{
}

void Warnings::Add(const QString& key, const QString& text)
{
    m_warnings.insert(key, text);
    UpdateWidget();
}

void Warnings::Remove(const QString& key)
{
    m_warnings.remove(key);
    UpdateWidget();
}

void Warnings::UpdateWidget() const
{
    if (!m_tooltipWidget)
        return;

    const bool hasWarnings = !m_warnings.isEmpty();

    m_tooltipWidget->setVisible(hasWarnings);
    m_tooltipWidget->setToolTip(hasWarnings ? m_warnings.values().join("\n") : "");
}
