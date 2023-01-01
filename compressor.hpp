#ifndef COMPRESSOR_HPP
#define COMPRESSOR_HPP

#include <QDir>
#include <QEventLoop>
#include <QList>
#include <QObject>
#include <QProcess>

class Compressor : public QObject
{
	Q_OBJECT

public:
	explicit Compressor(QObject* parent);

	struct Format
	{
		QString name;
		QString library;

		Format(QString name, QString library) : name{name}, library{library} {}

		bool operator==(const Format& rhs) const;

		static QString stringFromList(const QList<Format>& list);
	};

	const QList<Format> videoCodecs = {Format("H.264 NVENC", "h264_nvenc"),
						     Format("H.265", "libx265"),
						     Format("VP9", "libvpx-vp9"),
						     Format("H.264", "libx264")};

	const QList<Format> audioCodecs = {Format("OPUS", "libopus"),
						     Format("AAC", "aac"),
						     Format("OGG Vorbis", "libvorbis"),
						     Format("MP3", "libmp3lame")};

	const QList<QString> containers = {"mp4", "webm", "mov", "mkv"};

	void compress(const QUrl& inputUrl,
			  const QDir& outputDir,
			  const QString& fileSuffix,
			  const Format& videoCodec,
			  const Format& audioCodec,
			  const QString& container,
			  double sizeKbps,
			  double audioQualityPercent,
			  double minVideoBitrateKbps = 64,
			  double minAudioBitrateKbps = 16,
			  double maxAudioBitrateKbps = 256,
			  double overshootCorrectionPercent = 0.02);

signals:
	void compressionStarted(double videoBitrateKbps, double audioBitrateKbps);
	void compressionSucceeded(double requestedSizeKbps, double actualSizeKbps);
	void compressionProgressUpdate(double progressPercent);
	void compressionFailed(QString error, QString errorDetails = "");

private:
	QEventLoop eventLoop;
	QProcess* ffprobe = new QProcess();
	QProcess* ffmpeg = new QProcess(&eventLoop);
	QMetaObject::Connection* const processUpdateConnection = new QMetaObject::Connection();
	QMetaObject::Connection* const processFinishedConnection = new QMetaObject::Connection();

	QString parseOutput();

	double durationSeconds = -1;

	QString output = "";
};

#endif // COMPRESSOR_HPP
