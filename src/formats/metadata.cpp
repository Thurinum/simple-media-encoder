#include "formats/metadata.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

std::variant<Metadata, Metadata::Error>
Metadata::Builder::fromJson(QByteArray data)
{
    QJsonDocument document = QJsonDocument::fromJson(data);

    if (document.isNull()) {
        return Error {
            QObject::tr(
                "Could not retrieve media metadata. Is the file corrupted?"),
            QObject::tr("Found metadata: %1").arg(data)
        };
    }

    QJsonObject root    = document.object();
    QJsonArray  streams = root.value("streams").toArray();

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
        return Error { QObject::tr("Media metadata is incomplete."),
                       QObject::tr("Found metadata: %1").arg(data) };
    }

    Metadata metadata;
    error = { QObject::tr("Media metadata is incomplete."), "" };

    std::pair<double, double> aspectRatio = getAspectRatio();

    metadata = Metadata {
        .width            = value(video, "width", true).toDouble(),
        .height           = value(video, "height", true).toDouble(),
        .sizeKbps         = value(format, "size", true).toDouble() * 0.001,
        .audioBitrateKbps = value(audio, "bit_rate").toDouble() * 0.001,
        .durationSeconds  = value(format, "duration", true).toDouble(),
        .aspectRatioX     = aspectRatio.first,
        .aspectRatioY     = aspectRatio.second,
        .frameRate        = getFrameRate(),
        .videoCodec       = value(video, "codec_name", true).toString(),
        .audioCodec       = value(audio, "codec_name", true).toString(),
        .container        = "" // TODO: Find a reliable way to query format type
    };

    if (!error.details.isEmpty()) {
        error.details += QObject::tr("Raw metadata: %1").arg(data);
        return error;
    }

    return metadata;
}

double Metadata::Builder::getFrameRate()
{
    QVariant frameRateData = value(video, "r_frame_rate");

    if (frameRateData.isNull()) {
        QVariant nbrFramesData = value(video, "nb_frames");
        QVariant durationData  = value(format, "duration");

        if (nbrFramesData.isNull() || durationData.isNull()) {
            NotFound("frame rate");
            return -1;
        }

        double nbrFrames = nbrFramesData.toDouble();
        double duration  = durationData.toDouble();

        return nbrFrames / duration;
    }

    QStringList frameRateRatio = frameRateData.toString().split("/");

    if (frameRateRatio.size() != 2) {
        NotFound("frame rate");
        return -1;
    }

    return frameRateRatio.first().toDouble() / frameRateRatio.last().toDouble();
}

std::pair<double, double> Metadata::Builder::getAspectRatio()
{
    QVariant aspectRatioData = value(video, "display_aspect_ratio");

    if (aspectRatioData.isNull()) {
        double width  = value(video, "width").toDouble();
        double height = value(video, "height").toDouble();
        double ratio  = width / height;

        return { ratio, 1 };
    }

    QStringList aspectRatio = aspectRatioData.toString().split(":");

    if (aspectRatio.size() != 2) {
        NotFound("aspect ratio");
        return {};
    }

    return { aspectRatio.first().toDouble(), aspectRatio.last().toDouble() };
}
