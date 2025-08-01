#ifndef MEDIAENCODER_H
#define MEDIAENCODER_H

#include "core/formats/codec.hpp"
#include "core/formats/container.hpp"
#include "core/formats/metadata.hpp"
#include "encoder_options.hpp"

#include <QDir>
#include <QEventLoop>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QProcess>

struct Message;
using std::optional;

class MediaEncoder final : public QObject
{
    Q_OBJECT

public:
    explicit MediaEncoder();

    struct ComputedOptions
    {
        optional<double> videoBitrateKbps;
        optional<double> audioBitrateKbps;
    };

    void Encode(const EncoderOptions& options);
    QString getAvailableFormats() const;

signals:
    void encodingStarted(double videoBitrateKbps, double audioBitrateKbps);
    void encodingSucceeded(const EncoderOptions& options, const ComputedOptions& computed, QFile& output);
    void encodingProgressUpdate(double progressPercent);
    void encodingFailed(QString error, QString errorDetails = "");

private:
    const bool IS_WINDOWS = QSysInfo::kernelType() == "winnt";

    void StartCompression(const EncoderOptions& options, const ComputedOptions& computedOptions, const Metadata& metadata);
    void UpdateProgress(double mediaDuration);
    void EndCompression(
        const EncoderOptions& options, const ComputedOptions& computed, QString outputPath, QString command, int exitCode
    );

    [[nodiscard]] QString BuildBaseParams(const EncoderOptions& options, const ComputedOptions& computed) const;
    [[nodiscard]] QString BuildVideoFilterParams(const EncoderOptions& options, [[maybe_unused]] const ComputedOptions& computed) const;
    [[nodiscard]] QString BuildAudioFilterParams(const EncoderOptions& options, const ComputedOptions& computed) const;

    void ComputeVideoBitrate(const EncoderOptions& options, ComputedOptions& computed, const Metadata& metadata) const;
    bool computeAudioBitrate(const EncoderOptions& options, ComputedOptions& computed) const;
    double computePixelRatio(const EncoderOptions& options, const Metadata& metadata) const;

    std::variant<QString, Message> extensionForContainer(const Container& container) const;

    QString output = "";
    QString parseOutput() const;

    QEventLoop eventLoop;
    QProcess* ffmpeg = new QProcess(&eventLoop);

    QMetaObject::Connection processUpdateConnection;
    QMetaObject::Connection processFinishedConnection;
};

#endif // MEDIAENCODER_H
