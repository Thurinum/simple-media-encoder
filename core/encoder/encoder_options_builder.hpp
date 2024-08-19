#ifndef ENCODER_OPTIONS_BUILDER_H
#define ENCODER_OPTIONS_BUILDER_H

#include "encoder_options.hpp"
#include "core/formats/codec.hpp"
#include "core/formats/container.hpp"
#include "core/formats/metadata.hpp"

using std::optional;

class EncoderOptionsBuilder
{
    typedef EncoderOptionsBuilder self;

public:
    self& useMetadata(const Metadata& metadata);
    self& inputFrom(const QString& inputPath);
    self& outputTo(const QString& outputPath);
    self& withVideoCodec(const Codec& codec);
    self& withAudioCodec(const Codec& codec);
    self& withContainer(const Container& container);
    self& withTargetOutputSize(double sizeKbps);
    self& withAudioQuality(double audioQualityPercent);
    self& withOutputWidth(int outputWidth);
    self& withOutputHeight(int outputHeight);
    self& withAspectRatio(const QPoint& aspectRatio);
    self& atFps(int fps);
    self& atSpeed(double speed);
    self& withMinVideoBitrate(double bitrateKbps);
    self& withMinAudioBitrate(double bitrateKbps);
    self& withMaxAudioBitrate(double bitrateKbps);
    self& withOvershootCorrection(double overshootCorrectionPercent);
    self& withCustomArguments(const QString& customArguments);

    std::variant<EncoderOptions, QList<QString>> build();

private:
    optional<Metadata> inputMetadata;
    optional<QString> inputPath;
    optional<QString> outputPath;
    optional<Codec> videoCodec;
    optional<Codec> audioCodec;
    optional<Container> container;
    optional<double> sizeKbps;
    optional<double> audioQualityPercent;
    optional<int> outputWidth;
    optional<int> outputHeight;
    optional<QPoint> aspectRatio;
    optional<int> fps;
    optional<double> speed;
    double minVideoBitrateKbps = 64;
    double minAudioBitrateKbps = 16;
    double maxAudioBitrateKbps = 256;
    double overshootCorrectionPercent = 0.02;
    optional<QString> customArguments;

    QList<QString> errors;
};

#endif
