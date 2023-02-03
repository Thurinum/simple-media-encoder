#ifndef COMPRESSOR_HPP
#define COMPRESSOR_HPP

#include "metadata.hpp"

#include <QDir>
#include <QEventLoop>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QProcess>

using std::optional;

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

	struct Preset
	{
		QString name;
		Codec videoCodec;
		Codec audioCodec;
		Container container;
	};

	struct Options
	{
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

	struct ComputedOptions
	{
		optional<double> videoBitrateKbps;
		optional<double> audioBitrateKbps;
	};

	void Compress(const Options& options);
	std::variant<Metadata, Metadata::Error> getMetadata(const QString& path);
	QString getAvailableFormats();

signals:
	void compressionStarted(double videoBitrateKbps, double audioBitrateKbps);
	void compressionSucceeded(const Options& options,
					  const ComputedOptions& computed,
					  QFile& output);
	void compressionProgressUpdate(double progressPercent);
	void compressionFailed(QString error, QString errorDetails = "");

private:
	const bool IS_WINDOWS = QSysInfo::kernelType() == "winnt";

	void StartCompression(const Options& options, const ComputedOptions& computedOptions, const Metadata& metadata);
	void UpdateProgress(double mediaDuration);
	void EndCompression(
		const Options& options, const ComputedOptions& computed, QString outputPath, QString command, int exitCode);

	bool areValidOptions(const Options& options);
	void ComputeVideoBitrate(const Options& options, ComputedOptions& computed, const Metadata& metadata);
	bool computeAudioBitrate(const Options& options, ComputedOptions& computed);
	double computePixelRatio(const Options& options, const Metadata& metadata);

	QString output = "";
	QString parseOutput();

	QEventLoop eventLoop;
	QProcess* ffprobe = new QProcess();
	QProcess* ffmpeg = new QProcess(&eventLoop);

	QMetaObject::Connection* const processUpdateConnection = new QMetaObject::Connection();
	QMetaObject::Connection* const processFinishedConnection = new QMetaObject::Connection();
};

Q_DECLARE_METATYPE(Compressor::Codec);
Q_DECLARE_METATYPE(Compressor::Container);
Q_DECLARE_METATYPE(Compressor::Preset);

#endif // COMPRESSOR_HPP
