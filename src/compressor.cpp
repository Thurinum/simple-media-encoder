#include "compressor.hpp"

#include <QEventLoop>
#include <QFile>
#include <QRegularExpression>
#include <QStringBuilder>
#include <QTime>
#include <QUrl>
#include <QVariant>

Compressor::Compressor(QObject* parent) : QObject{parent}

{
	ffmpeg->setProcessChannelMode(QProcess::MergedChannels);
	connect(ffmpeg, &QProcess::errorOccurred, [this](QProcess::ProcessError error) {
		emit compressionFailed(tr("Process %1").arg(QVariant::fromValue(error).toString()));
	});
}

bool Compressor::computeAudioBitrate(const Options& options, ComputedOptions& computed)
{
	double audioBitrateKbps = qMax(options.minAudioBitrateKbps,
						 options.audioQualityPercent.value_or(1)
							 * options.maxAudioBitrateKbps);
	if (audioBitrateKbps < options.audioCodec->minBitrateKbps) {
		emit compressionFailed(tr("Selected audio codec %1 requires a minimum bitrate of %2 "
						  "kbps, but requested was %3 kbps.")
						     .arg(options.audioCodec->name,
							    QString::number(options.audioCodec->minBitrateKbps),
							    QString::number(audioBitrateKbps)));
		return false;
	}

	computed.audioBitrateKbps = audioBitrateKbps;
	return true;
}

double Compressor::computePixelRatio(const Options& options, const QStringList& metadata)
{
	double pixelRatio = 1;

	QStringList aspectRatio = metadata[2].split(":");
	int aspectRatioW = aspectRatio.first().toInt();
	int aspectRatioH = aspectRatio.last().toInt();
	int inputWidth = metadata[0].toInt();
	int inputHeight = metadata[1].toInt();
	int outputWidth;
	int outputHeight;

	long inputPixelCount = inputWidth * inputHeight;

	if (options.outputWidth.has_value()) {
		outputHeight = *options.outputHeight;
		outputWidth = outputHeight * aspectRatioW / aspectRatioH;
	} else {
		outputWidth = *options.outputWidth;
		outputHeight = outputWidth * aspectRatioW / aspectRatioH;
	}

	double outputPixelCount = outputWidth * outputHeight;

	// TODO: Add option to enable bitrate compensation even when upscaling (will result in bigger files)
	if (outputPixelCount > 0 && outputPixelCount < inputPixelCount)
		pixelRatio = outputPixelCount / inputPixelCount;

	return pixelRatio;
}

void Compressor::computeVideoBitrate(const Options& options,
						 ComputedOptions& computed,
						 const QStringList& metadata)
{
	double audioBitrateKbps = computed.audioBitrateKbps.value_or(0);

	double pixelRatio = computePixelRatio(options, metadata);
	double bitrateKbps = *options.sizeKbps / m_durationSeconds
				   * (1.0 - options.overshootCorrectionPercent);

	computed.videoBitrateKbps = qMax(options.minVideoBitrateKbps,
						   pixelRatio * (bitrateKbps - audioBitrateKbps));
}

void Compressor::compress(const Options& options)
{
	if (!areValidOptions(options))
		return;

	QStringList metadata = mediaMetadata(options.inputPath);
	if (metadata.isEmpty())
		return;

	m_durationSeconds = mediaDurationSeconds(metadata);
	if (m_durationSeconds == -1)
		return;

	ComputedOptions computed;

	if (options.audioCodec.has_value()) {
		if (!computeAudioBitrate(options, computed))
			return;
	}

	if (options.videoCodec.has_value()) {
		computeVideoBitrate(options, computed, metadata);
	}

	StartCompression(options, computed);
}

QString Compressor::availableFormats()
{
	ffmpeg->startCommand(QString("ffmpeg%1 -encoders").arg(IS_WINDOWS ? ".exe" : ""));
	ffmpeg->waitForFinished();
	return ffmpeg->readAllStandardOutput();
}

QString Compressor::parseOutput()
{
	QStringList split = m_output.split("Press [q] to stop, [?] for help");
	if (split.length() == 1)
		split = m_output.split("[0][0][0][0]");

	return split.last()
		.replace(
			QRegularExpression(
				R"((\[.*\]|(?:Conversion failed!)|(?:v\d\.\d.*)|(?: (?:\s)+)|(?:- (?:\s)+(?1))))"),
			"")
		.trimmed();
}

bool Compressor::areValidOptions(const Options& options)
{
	QString error;

	if (!options.videoCodec.has_value() && !options.audioCodec.has_value())
		error = tr("Neither a video nor an audio codec was selected.");

	if (!options.videoCodec.has_value() && !options.container.has_value())
		error = tr("A container was selected but audio-only encoding was specified.");

	if (options.videoCodec.has_value()
	    && !options.container->supportedCodecs.contains(options.videoCodec->library)) {
		error = tr("Selected container %1 does not support selected video codec: %2.")
				  .arg(options.container->name.toUpper(), options.videoCodec->name);
	}
	if (options.audioCodec.has_value()
	    && !options.container->supportedCodecs.contains(options.audioCodec->library)) {
		error = tr("Selected container %1 does not support selected audio codec: %2.")
				  .arg(options.container->name.toUpper(), options.audioCodec->name);
	}
	if (options.outputWidth.has_value() && options.outputWidth < 0) {
		error = tr("Output width must be greater than 0, but was %1")
				  .arg(QString::number(*options.outputWidth));
	}
	if (options.outputHeight.has_value() && options.outputHeight < 0) {
		error = tr("Output height must be greater than 0, but was %1")
				  .arg(QString::number(*options.outputHeight));
	}

	if (options.aspectRatio.has_value() && options.aspectRatio->x() <= 0) {
		error = tr("Invalid horizontal aspect %1").arg(QString::number(options.aspectRatio->x()));
	}
	if (options.aspectRatio.has_value() && options.aspectRatio->y() <= 0) {
		error = tr("Invalid horizontal aspect %1").arg(QString::number(options.aspectRatio->x()));
	}

	if (options.customArguments.has_value()
	    && QRegularExpression("[^ -/a-z0-9]").match(*options.customArguments).hasMatch()) {
		error = tr("Custom parameters contain invalid characters.");
	}

	if (!error.isEmpty()) {
		emit compressionFailed(error, tr("rip bozo"));
		return false;
	}

	return true;
}

QStringList Compressor::mediaMetadata(const QString& path)
{
	ffprobe->startCommand(QString("ffprobe%1 -v error -select_streams v:0 -show_entries "
						"stream=width,height,display_aspect_ratio,duration -of "
						"default=noprint_wrappers=1:nokey=1 \"%2\"")
					    .arg(IS_WINDOWS ? ".exe" : "", path));
	ffprobe->waitForFinished();

	// TODO: Refactor to use key value pairs
	QStringList metadata = QString(ffprobe->readAll()).split(QRegularExpression("[\n\r]+"));

	if (metadata.count() != 5) {
		emit compressionFailed(tr("Could not retrieve media metadata. Is the file corrupted?"),
					     tr("Input file: %1\nFound metadata: %2")
						     .arg(path, metadata.join(", ")));
		return QStringList();
	}

	return metadata;
}

double Compressor::mediaDurationSeconds(const QStringList& metadata)
{
	bool couldParse = false;
	QString input = metadata[3];
	double durationSeconds = input.toDouble(&couldParse);

	if (!couldParse) {
		emit compressionFailed(tr("Failed to parse media duration. Is the file corrupted?"),
					     tr("Invalid media duration '%1'.").arg(input));
		return -1;
	}

	return durationSeconds;
}

void Compressor::StartCompression(const Options& options, const ComputedOptions& computed)
{
	emit compressionStarted(computed.videoBitrateKbps.value_or(0),
					computed.audioBitrateKbps.value_or(0));

	QString videoCodecParam = options.videoCodec.has_value()
						  ? "-c:v " + options.videoCodec->library
						  : "-vn";
	QString audioCodecParam = options.audioCodec.has_value()
						  ? "-c:a " + options.audioCodec->library
						  : "-an";
	QString videoBitrateParam = options.sizeKbps.has_value()
						    ? "-b:v " + QString::number(*computed.videoBitrateKbps)
								+ "k"
						    : "";
	QString audioBitrateParam = computed.audioBitrateKbps.has_value()
						    ? "-b:a " + QString::number(*computed.audioBitrateKbps)
								+ "k"
						    : "";
	QString aspectRatioParam;
	QString scaleParam;

	if (options.outputWidth.has_value() && options.outputHeight.has_value()) {
		scaleParam = QString("scale=%1:%2")
					 .arg(QString::number(*options.outputWidth),
						QString::number(*options.outputHeight));
		aspectRatioParam = "setsar=1/1";
	} else if (options.outputWidth.has_value()) {
		scaleParam = QString("-vf scale=%1:-2").arg(QString::number(*options.outputWidth));
	} else if (options.outputHeight.has_value()) {
		scaleParam = QString("-vf scale=-1:%1").arg(QString::number(*options.outputHeight));
	}

	if (options.aspectRatio.has_value()) {
		aspectRatioParam = QString("setsar=%1/%2")
						 .arg(QString::number(options.aspectRatio->y()),
							QString::number(options.aspectRatio->x()));
	}

	QStringList videoFilters = QStringList{scaleParam, aspectRatioParam};
	videoFilters.removeAll({});
	QString videoFiltersParam = "-vf " + videoFilters.join(',');

	QString fileExtension = options.videoCodec.has_value() ? options.container->name
										 : options.audioCodec->name;
	QString outputPath = options.outputPath + "." + fileExtension;

	QString command = QString(R"(ffmpeg%1 -i "%2" %3 %4 %5 %6 %7 %8 "%9" -y)")
					.arg(IS_WINDOWS ? ".exe" : "",
					     options.inputPath,
					     videoCodecParam,
					     audioCodecParam,
					     videoBitrateParam,
					     audioBitrateParam,
					     videoFiltersParam,
					     *options.customArguments,
					     outputPath);

	*processUpdateConnection = connect(ffmpeg, &QProcess::readyRead, [this]() {
		UpdateProgress();
	});

	*processFinishedConnection = connect(ffmpeg, &QProcess::finished, [=, this](int exitCode) {
		EndCompression(options, outputPath, command, exitCode);
	});

	ffmpeg->startCommand(command);
}

void Compressor::UpdateProgress()
{
	QString line = QString(ffmpeg->readAll());
	QRegularExpression regex("time=([0-9][0-9]:[0-9][0-9]:[0-9][0-9].[0-9][0-9])");
	QRegularExpressionMatch match = regex.match(line);

	m_output += line;

	if (!match.hasMatch())
		return;

	QTime timestamp = QTime::fromString(match.captured(1));
	int currentDuration = timestamp.second() + timestamp.minute() * 60 + timestamp.hour() * 3600;
	int progressPercent = currentDuration * 100 / m_durationSeconds;

	emit compressionProgressUpdate(progressPercent);
}

void Compressor::EndCompression(const Options& options,
					  QString outputPath,
					  QString command,
					  int exitCode)
{
	if (exitCode != 0) {
		disconnect(*processUpdateConnection);
		disconnect(*processFinishedConnection);

		emit compressionFailed(parseOutput(), command + "\n\n" + m_output);
		m_output.clear();
		return;
	}

	QFile media(outputPath);
	if (!media.open(QIODevice::ReadOnly)) {
		disconnect(*processUpdateConnection);
		disconnect(*processFinishedConnection);
		emit compressionFailed("Could not open the compressed media.", media.errorString());
		m_output.clear();
		media.close();
		return;
	}

	const double BYTES_TO_KB = 125.0; // or smt like that anyway lol

	disconnect(*processUpdateConnection);
	disconnect(*processFinishedConnection);
	emit compressionSucceeded(*options.sizeKbps, media.size() / BYTES_TO_KB, &media);
	m_output.clear();
	media.close();
}

bool Compressor::Codec::operator==(const Codec& rhs) const
{
	return this->name == rhs.name && this->library == rhs.library;
}

QString Compressor::Codec::stringFromList(const QList<Codec>& list)
{
	QString str;

	for (const Codec& format : list)
		str += format.name % ", ";

	str.chop(2);

	return str;
}
