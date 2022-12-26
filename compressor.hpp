#ifndef COMPRESSOR_HPP
#define COMPRESSOR_HPP

#include <QEventLoop>
#include <QObject>
#include <QProcess>

class Compressor : public QObject
{
	Q_OBJECT

public:
	explicit Compressor(QObject* parent);

	enum VideoCodec { libx265, libvpx_vp9, libx264 };
	enum AudioCodec { libopus, aac, libvorbis, libmp3lame }; // libfdk_aac
	enum Container { mp4, webm, mov, mkv };

	Q_ENUM(VideoCodec)
	Q_ENUM(AudioCodec)
	Q_ENUM(Container)

	void compress(const QUrl& fileUrl,
			  const QString& fileSuffix,
			  VideoCodec videoCodec,
			  AudioCodec audioCodec,
			  Container container,
			  double sizeKbps,
			  double audioQualityPercent,
			  double minVideoBitrateKbps = 64,
			  double minAudioBitrateKbps = 16,
			  double maxAudioBitrateKbps = 256,
			  double overshootCorrectionPercent = 0.02);

signals:
	void compressionSucceeded(double requestedSizeKbps, double actualSizeKbps);
	void compressionProgressUpdate(double progressPercent);
	void compressionFailed(QString errorMessage);

private:
	QEventLoop eventLoop;
	QProcess* ffprobe = new QProcess();
	QProcess* ffmpeg = new QProcess(&eventLoop);
	QMetaObject::Connection* const processUpdateConnection = new QMetaObject::Connection();
	QMetaObject::Connection* const processFinishedConnection = new QMetaObject::Connection();

	double durationSeconds = -1;
};

#endif // COMPRESSOR_HPP
