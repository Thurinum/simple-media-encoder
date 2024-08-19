#include "encoder_options_builder.hpp"

#include <QFile>

EncoderOptionsBuilder::self& EncoderOptionsBuilder::useMetadata(const Metadata& metadata)
{
    this->inputMetadata = metadata;
    return *this;
}

EncoderOptionsBuilder::self& EncoderOptionsBuilder::inputFrom(const QString& inputPath)
{
    if (inputPath.isEmpty()) {
        errors.append(QObject::tr("Input path is empty."));
        return *this;
    }

    if (!QFile::exists(inputPath)) {
        errors.append(QObject::tr("No file exists at input path '%1'.").arg(inputPath));
        return *this;
    }

    this->inputPath = inputPath;
    return *this;
}

EncoderOptionsBuilder::self& EncoderOptionsBuilder::outputTo(const QString& outputPath)
{
    if (outputPath.isEmpty()) {
        errors.append(QObject::tr("Output path is empty."));
        return *this;
    }

    this->outputPath = outputPath;
    return *this;
}

EncoderOptionsBuilder::self& EncoderOptionsBuilder::withVideoCodec(const Codec& codec)
{
    this->videoCodec = codec;
    return *this;
}

EncoderOptionsBuilder::self& EncoderOptionsBuilder::withAudioCodec(const Codec& codec)
{
    this->audioCodec = codec;
    return *this;
}

EncoderOptionsBuilder::self& EncoderOptionsBuilder::withContainer(const Container& container)
{
    this->container = container;
    return *this;
}

EncoderOptionsBuilder::self& EncoderOptionsBuilder::withTargetOutputSize(double sizeKbps)
{
    if (sizeKbps == 0) { // auto-mode
        return *this;
    }

    if (sizeKbps <= 0) {
        errors.append(QObject::tr("Target output size must be greater than 0."));
        return *this;
    }

    this->sizeKbps = sizeKbps;
    return *this;
}

EncoderOptionsBuilder::self& EncoderOptionsBuilder::withAudioQuality(double audioQualityPercent)
{
    if (audioQualityPercent < 0 || audioQualityPercent > 100) {
        errors.append(QObject::tr("Audio quality must be between 0 and 100."));
        return *this;
    }

    this->audioQualityPercent = audioQualityPercent;
    return *this;
}

EncoderOptionsBuilder::self& EncoderOptionsBuilder::withOutputWidth(int outputWidth)
{
    if (outputWidth == 0) // auto-mode
        return *this;

    if (outputWidth <= 0) {
        errors.append(QObject::tr("Output width must be greater than 0."));
        return *this;
    }

    this->outputWidth = outputWidth;
    return *this;
}

EncoderOptionsBuilder::self& EncoderOptionsBuilder::withOutputHeight(int outputHeight)
{
    if (outputHeight == 0) // auto-mode
        return *this;

    if (outputHeight <= 0) {
        errors.append(QObject::tr("Output height must be greater than 0."));
        return *this;
    }

    this->outputHeight = outputHeight;
    return *this;
}

EncoderOptionsBuilder::self& EncoderOptionsBuilder::withAspectRatio(const QPoint& aspectRatio)
{
    if (aspectRatio.x() == 0 || aspectRatio.y() == 0) // auto-mode
        return *this;

    if (aspectRatio.x() < 0 || aspectRatio.y() < 0) {
        errors.append(QObject::tr("Aspect ratio components must be greater than 0."));
        return *this;
    }

    if (aspectRatio.y() == 0) {
        errors.append(QObject::tr("Aspect ratio denominator must be greater than 0."));
        return *this;
    }

    this->aspectRatio = aspectRatio;
    return *this;
}

EncoderOptionsBuilder::self& EncoderOptionsBuilder::atFps(int fps)
{
    if (fps == 0) // auto-mode
        return *this;

    if (fps <= 0) {
        errors.append(QObject::tr("FPS must be greater than 0."));
        return *this;
    }

    this->fps = fps;
    return *this;
}

EncoderOptionsBuilder::self& EncoderOptionsBuilder::atSpeed(double speed)
{
    if (speed == 0) // auto-mode
        return *this;

    if (speed < 0) {
        errors.append(QObject::tr("Speed must not be lower than 0."));
        return *this;
    }

    this->speed = speed;
    return *this;
}

EncoderOptionsBuilder::self& EncoderOptionsBuilder::withMinVideoBitrate(double bitrateKbps)
{
    if (bitrateKbps <= 0) {
        errors.append(QObject::tr("Minimum video bitrate must be greater than 0."));
        return *this;
    }

    this->minVideoBitrateKbps = bitrateKbps;
    return *this;
}

EncoderOptionsBuilder::self& EncoderOptionsBuilder::withMinAudioBitrate(double bitrateKbps)
{
    if (bitrateKbps <= 0) {
        errors.append(QObject::tr("Minimum audio bitrate must be greater than 0."));
        return *this;
    }

    this->minAudioBitrateKbps = bitrateKbps;
    return *this;
}

EncoderOptionsBuilder::self& EncoderOptionsBuilder::withMaxAudioBitrate(double bitrateKbps)
{
    if (bitrateKbps <= 0) {
        errors.append(QObject::tr("Maximum audio bitrate must be greater than 0."));
        return *this;
    }

    this->maxAudioBitrateKbps = bitrateKbps;
    return *this;
}

EncoderOptionsBuilder::self& EncoderOptionsBuilder::withOvershootCorrection(double overshootCorrectionPercent)
{
    this->overshootCorrectionPercent = overshootCorrectionPercent;
    return *this;
}

EncoderOptionsBuilder::self& EncoderOptionsBuilder::withCustomArguments(const QString& customArguments)
{
    this->customArguments = customArguments;
    return *this;
}

std::variant<EncoderOptions, QList<QString>> EncoderOptionsBuilder::build()
{
    if (!inputMetadata.has_value())
        errors.append(QObject::tr("No input metadata was specified."));

    if (!inputPath.has_value())
        errors.append(QObject::tr("No input path was specified."));

    if (!outputPath.has_value())
        errors.append(QObject::tr("No output path was specified."));

    if (!videoCodec.has_value() && !audioCodec.has_value())
        errors.append(QObject::tr("Neither a video or audio codec was specified."));

    if (!container.has_value())
        errors.append(QObject::tr("No container was specified."));

    if (minAudioBitrateKbps > maxAudioBitrateKbps)
        errors.append(QObject::tr("Minimum audio bitrate must be less than or equal to maximum audio bitrate."));

    if (!errors.isEmpty())
        return errors;

    // TODO: Should we use std::move? I have to read on move semantics lol
    return EncoderOptions {
        .inputMetadata = *inputMetadata,
        .inputPath = *inputPath,
        .outputPath = *outputPath,
        .videoCodec = videoCodec,
        .audioCodec = audioCodec,
        .container = *container,
        .sizeKbps = sizeKbps,
        .audioQualityPercent = audioQualityPercent,
        .outputWidth = outputWidth,
        .outputHeight = outputHeight,
        .aspectRatio = aspectRatio,
        .fps = fps,
        .speed = speed,
        .minVideoBitrateKbps = minVideoBitrateKbps,
        .minAudioBitrateKbps = minAudioBitrateKbps,
        .maxAudioBitrateKbps = maxAudioBitrateKbps,
        .overshootCorrectionPercent = overshootCorrectionPercent,
        .customArguments = customArguments
    };
}
