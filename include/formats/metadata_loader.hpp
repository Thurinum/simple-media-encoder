#ifndef METADATA_LOADER_H
#define METADATA_LOADER_H

#include <QByteArray>
#include <QCoreApplication>
#include <QEventLoop>
#include <QProcess>
#include <variant>

#include "formats/metadata.hpp"
#include "notifier/message.hpp"
#include "utils/platform_info.hpp"

typedef std::variant<Metadata, Message> MetadataResult;

class MetadataLoader : public QObject
{
    Q_OBJECT
public:
    MetadataLoader(const PlatformInfo& platformInfo);

    void loadAsync(const QString& path);

signals:
    void loadAsyncComplete(MetadataResult result);

private:
    void handleResult();
    MetadataResult parse(QByteArray data);

    double getFrameRate(QList<QString>& errors);
    std::pair<double, double> getAspectRatio(QList<QString>& errors);

    inline QVariant value(QList<QString>& errors, QJsonObject& source, const QString& key, bool required = false)
    {
        if (source.isEmpty())
            return {};

        if (source.contains(key))
            return source.value(key).toVariant();

        if (required)
            NotFound(errors, key);

        return {};
    }

    inline void NotFound(QList<QString>& errors, const QString& key)
    {
        errors.append(QString("Could not find %1 in metadata.").arg(key));
    }

    QProcess ffprobe;
    PlatformInfo platform;

    QJsonObject format;
    QJsonObject video;
    QJsonObject audio;
};

#endif
