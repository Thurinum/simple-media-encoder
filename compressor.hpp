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

	struct Codec
	{
		QString name;
		QString library;
		double minBitrateKbps = 0;

		bool operator==(const Codec& rhs) const;

		static QString stringFromList(const QList<Codec>& list);
	};

	struct Container
	{
		QString name;
		QStringList supportedCodecs;
	};

	void compress(const QUrl& inputUrl,
			  const QDir& outputDir,
			  const QString& fileSuffix,
			  const Codec& videoCodec,
			  const Codec& audioCodec,
			  const Container& container,
			  double sizeKbps,
			  double audioQualityPercent,
			  double minVideoBitrateKbps = 64,
			  double minAudioBitrateKbps = 16,
			  double maxAudioBitrateKbps = 256,
			  double overshootCorrectionPercent = 0.02);

	QString availableFormats();

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

	QString output = "";
	double durationSeconds = -1;

	QString parseOutput();
};

#endif // COMPRESSOR_HPP
