#ifndef MEDIAENCODER_H
#define MEDIAENCODER_H

#include "encoder_options.hpp"
#include "core/formats/codec.hpp"
#include "core/formats/container.hpp"
#include "core/formats/metadata.hpp"
#include "core/formats/metadata_loader.hpp"

#include <QDir>
#include <QEventLoop>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QProcess>

struct Message;
using std::optional;

class MediaEncoder : public QObject {
    Q_OBJECT

public:
    explicit MediaEncoder(QObject* parent);
    ~MediaEncoder() override;

    struct ComputedOptions {
        optional<double> videoBitrateKbps;
        optional<double> audioBitrateKbps;
    };

    void Encode(const EncoderOptions& options);
    QString getAvailableFormats();

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

    void ComputeVideoBitrate(const EncoderOptions& options, ComputedOptions& computed, const Metadata& metadata);
    bool computeAudioBitrate(const EncoderOptions& options, ComputedOptions& computed);
    double computePixelRatio(const EncoderOptions& options, const Metadata& metadata);

    std::variant<QString, Message> extensionForContainer(const Container& container);

    QString output = "";
    QString parseOutput();

    QEventLoop eventLoop;
    QProcess* ffprobe = new QProcess();
    QProcess* ffmpeg = new QProcess(&eventLoop);

    QMetaObject::Connection* const processUpdateConnection = new QMetaObject::Connection();
    QMetaObject::Connection* const processFinishedConnection = new QMetaObject::Connection();
};

#endif // MEDIAENCODER_H
