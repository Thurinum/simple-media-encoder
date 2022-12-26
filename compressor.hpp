#ifndef COMPRESSOR_HPP
#define COMPRESSOR_HPP

#include <QObject>
#include <QProcess>

class Compressor : public QObject
{
	Q_OBJECT
public:
	explicit Compressor(QObject* parent) : QObject{parent} {};

	void compress(const QUrl& fileUrl,
			  const QString& fileSuffix,
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
	QProcess* ffprobe = new QProcess(this->parent());
	QProcess* ffmpeg = new QProcess(this->parent());
};

#endif // COMPRESSOR_HPP
