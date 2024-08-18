#pragma once

#include "settings.hpp"

#include <QPointer>

class QSettings;
class QString;

class IniSettings final : public Settings
{
public:
    explicit IniSettings(const QString& fileName, const QString& defaultFileName = "");

    [[nodiscard]] QVariant get(const QString& key) const override;
    void Set(const QString& key, const QVariant& value) override;

    [[nodiscard]] QStringList groups() const override;
    [[nodiscard]] QStringList keysInGroup(const QString& group) const override;
    [[nodiscard]] QString fileName() const override;

private:
    QPointer<QSettings> settings;
    QPointer<QSettings> defaultSettings;
};