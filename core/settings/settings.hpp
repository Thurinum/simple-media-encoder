#pragma once

#include <QVariant>

struct Settings {
    virtual ~Settings() = default;

    [[nodiscard]] virtual QVariant get(const QString& key) const = 0;
    virtual void Set(const QString& key, const QVariant& value) = 0;

    [[nodiscard]] virtual QStringList groups() const = 0;
    [[nodiscard]] virtual QStringList keysInGroup(const QString& group) const = 0;
    [[nodiscard]] virtual QString fileName() const = 0;
};
