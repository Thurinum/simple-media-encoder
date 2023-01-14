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

	struct Options
	{
		const QUrl& inputUrl;
		const QDir& outputDir;
		const QString& fileSuffix;
		const Codec& videoCodec;
		const Codec& audioCodec;
		const Container& container;
		double sizeKbps;
		double audioQualityPercent;
		int outputWidth;
		int outputHeight;
		double minVideoBitrateKbps = 64;
		double minAudioBitrateKbps = 16;
		double maxAudioBitrateKbps = 256;
		double overshootCorrectionPercent = 0.02;
	};

	static const int AUTO_SIZE = 0;

	void compress(Options options);
	QString availableFormats();

signals:
	void compressionStarted(double videoBitrateKbps, double audioBitrateKbps);
	void compressionSucceeded(double requestedSizeKbps, double actualSizeKbps, QFile* outputFile);
	void compressionProgressUpdate(double progressPercent);
	void compressionFailed(QString error, QString errorDetails = "");

private:
	struct ComputedOptions
	{
		double videoBitrateKbps;
		double audioBitrateKbps;
		QString scaledWidthParameter;
		QString scaledHeightParameter;
	};

	const bool IS_WINDOWS = QSysInfo::kernelType() == "winnt";

	QEventLoop eventLoop;
	QProcess* ffprobe = new QProcess();
	QProcess* ffmpeg = new QProcess(&eventLoop);

	QMetaObject::Connection* const processUpdateConnection = new QMetaObject::Connection();
	QMetaObject::Connection* const processFinishedConnection = new QMetaObject::Connection();

	QString m_output = "";
	double m_durationSeconds = -1;
	QString parseOutput();

	bool areValidOptions(const Options& options);
	QStringList mediaMetadata(const QString& path);
	double mediaDurationSeconds(const QStringList& metadata);
	void StartCompression(const Options& options, const ComputedOptions& computedOptions);
	void UpdateProgress();
	void EndCompression(const Options& options, QString outputPath, QString command, int exitCode);
};

#endif // COMPRESSOR_HPP
