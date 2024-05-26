#ifndef ENCODER_OPTIONS_H
#define ENCODER_OPTIONS_H

#include <QString>
#include <optional>

#include "formats/codec.hpp"
#include "formats/container.hpp"
#include "formats/metadata.hpp"

using std::optional;

struct EncoderOptions {
    const optional<const Metadata> inputMetadata;
    const QString inputPath;
    const QString outputPath;
    const optional<const Codec> videoCodec;
    const optional<const Codec> audioCodec;
    const Container container;
    const optional<const double> sizeKbps;
    const optional<const double> audioQualityPercent;
    const optional<const int> outputWidth;
    const optional<const int> outputHeight;
    const optional<const QPoint> aspectRatio;
    const optional<const int> fps;
    const optional<const double> speed;
    const double minVideoBitrateKbps = 64;
    const double minAudioBitrateKbps = 16;
    const double maxAudioBitrateKbps = 256;
    const double overshootCorrectionPercent = 0.02;
    const optional<const QString> customArguments;
};

#endif
