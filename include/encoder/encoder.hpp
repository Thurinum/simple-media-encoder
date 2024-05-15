#ifndef MEDIAENCODER_H
#define MEDIAENCODER_H

#include "formats/codec.hpp"
#include "formats/container.hpp"
#include "formats/metadata.hpp"

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

    struct Options {
        const QString inputPath;
        const QString outputPath;
        optional<const Codec> videoCodec;
        optional<const Codec> audioCodec;
        optional<const Container> container;
        optional<double> sizeKbps;
        optional<double> audioQualityPercent;
        optional<int> outputWidth;
        optional<int> outputHeight;
        optional<QPoint> aspectRatio;
        optional<int> fps;
        optional<double> speed;
        optional<QString> customArguments;
        optional<const Metadata> inputMetadata;
        double minVideoBitrateKbps = 64;
        double minAudioBitrateKbps = 16;
        double maxAudioBitrateKbps = 256;
        double overshootCorrectionPercent = 0.02;
    };

    struct ComputedOptions {
        optional<double> videoBitrateKbps;
        optional<double> audioBitrateKbps;
    };

    void Encode(const Options& options);
    std::variant<Metadata, Metadata::Error> getMetadata(const QString& path);
    QString getAvailableFormats();

signals:
    void encodingStarted(double videoBitrateKbps, double audioBitrateKbps);
    void encodingSucceeded(const Options& options, const ComputedOptions& computed, QFile& output);
    void encodingProgressUpdate(double progressPercent);
    void encodingFailed(QString error, QString errorDetails = "");
    void metadataComputed();

private:
    const bool IS_WINDOWS = QSysInfo::kernelType() == "winnt";

    void StartCompression(const Options& options, const ComputedOptions& computedOptions, const Metadata& metadata);
    void UpdateProgress(double mediaDuration);
    void EndCompression(
        const Options& options, const ComputedOptions& computed, QString outputPath, QString command, int exitCode);

    bool validateOptions(const Options& options);
    void ComputeVideoBitrate(const Options& options, ComputedOptions& computed, const Metadata& metadata);
    bool computeAudioBitrate(const Options& options, ComputedOptions& computed);
    double computePixelRatio(const Options& options, const Metadata& metadata);

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
