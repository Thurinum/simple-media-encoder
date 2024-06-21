#include "formats/metadata_loader.hpp"
#include "formats/metadata.hpp"
#include "notifier/message.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QtConcurrent/QtConcurrent>

MetadataResult MetadataLoader::parse(QByteArray data)
{
    QJsonDocument document = QJsonDocument::fromJson(data);

    if (document.isNull()) {
        return Message(
            Severity::Error,
            QObject::tr(
                "Could not retrieve media metadata. Is the file corrupted?"
            ),
            QObject::tr("Found metadata: %1").arg(data)
        );
    }

    QJsonObject root = document.object();
    QJsonArray streams = root.value("streams").toArray();

    // we only support 1 stream of each type at the moment
    format = root.value("format").toObject();

    for (QJsonValueRef streamRef : streams) {
        QJsonObject stream = streamRef.toObject();
        QJsonValue  type   = stream.value("codec_type");

        if (type == "video" && video.isEmpty()) {
            video = stream;
        }

        if (type == "audio" && audio.isEmpty()) {
            audio = stream;
        }
    }

    if (format.isEmpty() || (video.isEmpty() && audio.isEmpty())) {
        return Message(
            Severity::Error,
            QObject::tr("Media metadata is incomplete."),
            QObject::tr("Found metadata: %1").arg(data)
        );
    }

    Metadata metadata;
    QList<QString> errors;
    std::pair<double, double> aspectRatio = getAspectRatio(errors);

    metadata = Metadata {
        .width = value(errors, video, "width", true).toDouble(),
        .height = value(errors, video, "height", true).toDouble(),
        .sizeKbps = value(errors, format, "size", true).toDouble() * 0.001,
        .audioBitrateKbps = value(errors, audio, "bit_rate").toDouble() * 0.001,
        .durationSeconds = value(errors, format, "duration", true).toDouble(),
        .aspectRatioX = aspectRatio.first,
        .aspectRatioY = aspectRatio.second,
        .frameRate = getFrameRate(errors),
        .videoCodec = value(errors, video, "codec_name", true).toString(),
        .audioCodec = value(errors, audio, "codec_name", true).toString(),
        .container = "" // TODO: Find a reliable way to query format type
    };

    if (!errors.isEmpty()) {
        return Message(
            Severity::Error,
            "Missing metadata fields",
            "The following metadata fields could not be found: " + errors.join("\n"),
            QObject::tr("Raw metadata: %1").arg(data)
        );
    }

    return metadata;
}

void MetadataLoader::handleResult()
{
    if (ffprobe.exitCode() != 0) {
        emit Message(
            Severity::Error,
            tr("Could not retrieve media metadata."),
            tr("FFprobe failed: %1").arg(ffprobe.errorString())
        );

        return;
    }

    QByteArray data = ffprobe.readAll();
    MetadataResult result = parse(data);
    emit loadAsyncComplete(result);
}

MetadataLoader::MetadataLoader(const PlatformInfo& platformInfo)
    : platform(platformInfo) { }

void MetadataLoader::loadAsync(const QString& path)
{
    if (ffprobe.state() == QProcess::Running)
        return;

    ffprobe.start(
        QString(R"(ffprobe%1 -v error -print_format json -show_format -show_streams "%2")")
            .arg(platform.isWindows() ? ".exe" : "", path)
    );

    if (!ffprobe.waitForStarted()) {
        emit Message(
            Severity::Error,
            tr("Could not retrieve media metadata."),
            tr("FFprobe failed: %1").arg(ffprobe.errorString())
        );

        return;
    }

    connect(&ffprobe, &QProcess::finished, this, &MetadataLoader::handleResult, Qt::UniqueConnection);
}

double MetadataLoader::getFrameRate(QList<QString>& errors)
{
    QVariant frameRateData = value(errors, video, "r_frame_rate");

    if (frameRateData.isNull()) {
        QVariant nbrFramesData = value(errors, video, "nb_frames");
        QVariant durationData = value(errors, format, "duration");

        if (nbrFramesData.isNull() || durationData.isNull()) {
            NotFound(errors, "frame rate");
            return -1;
        }

        double nbrFrames = nbrFramesData.toDouble();
        double duration  = durationData.toDouble();

        return nbrFrames / duration;
    }

    QStringList frameRateRatio = frameRateData.toString().split("/");

    if (frameRateRatio.size() != 2) {
        NotFound(errors, "frame rate");
        return -1;
    }

    return frameRateRatio.first().toDouble() / frameRateRatio.last().toDouble();
}

std::pair<double, double> MetadataLoader::getAspectRatio(QList<QString>& errors)
{
    QVariant aspectRatioData = value(errors, video, "display_aspect_ratio");

    if (aspectRatioData.isNull()) {
        double width = value(errors, video, "width").toDouble();
        double height = value(errors, video, "height").toDouble();
        double ratio  = width / height;

        return { ratio, 1 };
    }

    QStringList aspectRatio = aspectRatioData.toString().split(":");

    if (aspectRatio.size() != 2) {
        NotFound(errors, "aspect ratio");
        return {};
    }

    return { aspectRatio.first().toDouble(), aspectRatio.last().toDouble() };
}
