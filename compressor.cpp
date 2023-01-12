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

void Compressor::compress(Options options)
{
	if (!validateOptions(options))
		return;

	QStringList metadata = mediaMetadata(options.inputUrl.toLocalFile());
	if (metadata.isEmpty())
		return;

	double durationSeconds = mediaDurationSeconds(metadata);
	if (durationSeconds == -1)
		return;

	// FIXME: This whole section makes no sense. Why are we calculating width parameter
	// but then also recalculating output width conditionally? Both parts should be linked
	// width and height scaling
	QString widthParam;

	if (options.outputWidth == AUTO_SIZE)
		widthParam = options.outputHeight == AUTO_SIZE ? "iw" : "-1";
	else
		widthParam = QString::number(options.outputWidth);

	QString heightParam = options.outputHeight == AUTO_SIZE
					    ? "-2"
					    : QString::number(options.outputHeight + options.outputHeight % 2);

	// pixel ratio
	double pixelRatio = 1;

	QStringList aspectRatio = metadata[2].split(":");
	int aspectRatioW = aspectRatio.first().toInt();
	int aspectRatioH = aspectRatio.last().toInt();
	int inputWidth = metadata[0].toInt();
	int inputHeight = metadata[1].toInt();

	double inputPixelCount = inputWidth * inputHeight;

	if (options.outputWidth == AUTO_SIZE) {
		options.outputWidth = options.outputHeight * aspectRatioW / aspectRatioH;
	} else {
		options.outputHeight = options.outputWidth * aspectRatioW / aspectRatioH;
	}

	double outputPixelCount = options.outputWidth * options.outputHeight;

	// TODO: Add option to enable bitrate compensation even when upscaling (will result in bigger files)
	if (outputPixelCount > 0 && outputPixelCount < inputPixelCount)
		pixelRatio = outputPixelCount / inputPixelCount;

	// calculate bitrate
	double bitrateKbps = options.sizeKbps / durationSeconds
				   * (1.0 - options.overshootCorrectionPercent);
	double audioBitrateKbps = qMax(options.minAudioBitrateKbps,
						 options.audioQualityPercent * options.maxAudioBitrateKbps);
	double videoBitrateKpbs = qMax(options.minVideoBitrateKbps,
						 pixelRatio * (bitrateKbps - audioBitrateKbps));

	if (audioBitrateKbps < options.audioCodec.minBitrateKbps) {
		emit compressionFailed(tr("Selected audio codec %1 requires a minimum bitrate of %2 "
						  "kbps, but requested was %3 kbps.")
						     .arg(options.audioCodec.name,
							    QString::number(options.audioCodec.minBitrateKbps),
							    QString::number(audioBitrateKbps)));
		return;
	}

	// FIXME: For some reason, can't go inside PerformCompression
	*processUpdateConnection = connect(ffmpeg, &QProcess::readyRead, [=, this]() {
		QString line = QString(ffmpeg->readAll());
		QRegularExpression regex("time=([0-9][0-9]:[0-9][0-9]:[0-9][0-9].[0-9][0-9])");
		QRegularExpressionMatch match = regex.match(line);

		output += line;

		if (!match.hasMatch())
			return;

		QTime timestamp = QTime::fromString(match.captured(1));
		int currentDuration = timestamp.second() + timestamp.minute() * 60
					    + timestamp.hour() * 3600;
		int progressPercent = currentDuration * 100 / durationSeconds;

		emit compressionProgressUpdate(progressPercent);
	});

	PerformCompression(options, {videoBitrateKpbs, audioBitrateKbps, widthParam, heightParam});
}

QString Compressor::availableFormats()
{
	ffmpeg->startCommand(QString("ffmpeg%1 -encoders").arg(IS_WINDOWS ? ".exe" : ""));
	ffmpeg->waitForFinished();
	return ffmpeg->readAllStandardOutput();
}

QString Compressor::parseOutput()
{
	QStringList split = output.split("Press [q] to stop, [?] for help");
	if (split.length() == 1)
		split = output.split("[0][0][0][0]");

	return split.last()
		.replace(
			QRegularExpression(
				R"((\[.*\]|(?:Conversion failed!)|(?:v\d\.\d.*)|(?: (?:\s)+)|(?:- (?:\s)+(?1))))"),
			"")
		.trimmed();
}

bool Compressor::validateOptions(const Options& options)
{
	QString error;

	if (!options.container.supportedCodecs.contains(options.videoCodec.library)) {
		error = tr("Selected options.container %1 does not support selected video codec: %2.")
				  .arg(options.container.name.toUpper(), options.videoCodec.name);
	}
	if (!options.container.supportedCodecs.contains(options.audioCodec.library)) {
		error = tr("Selected options.container %1 does not support selected audio codec: %2.")
				  .arg(options.container.name.toUpper(), options.audioCodec.name);
	}
	if (options.outputWidth < 0) {
		error = tr("Output width must be greater than 0, but was %1")
				  .arg(QString::number(options.outputWidth));
	}
	if (options.outputHeight < 0) {
		error = tr("Output height must be greater than 0, but was %1")
				  .arg(QString::number(options.outputHeight));
	}

	return error.isEmpty();
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

void Compressor::PerformCompression(const Options& options, const ComputedOptions& computedOptions)
{
	emit compressionStarted(computedOptions.videoBitrateKbps, computedOptions.audioBitrateKbps);

	QStringList fileName = options.inputUrl.fileName().split(".");
	QString outputPath = options.outputDir.filePath(fileName.first() + "_" + options.fileSuffix
									+ "." + options.container.name);
	QString command =
		QString(R"(ffmpeg%1 -i "%2" -c:v %3 -c:a %4 %5 -b:a %6k -vf scale=%7:%8%9 "%10" -y)")
			.arg(IS_WINDOWS ? ".exe" : "",
			     options.inputUrl.toLocalFile(),
			     options.videoCodec.library,
			     options.audioCodec.library,
			     options.sizeKbps > AUTO_SIZE ? QString("-b:v %1").arg(
				     QString::number(computedOptions.videoBitrateKbps))
								    : "",
			     QString::number(computedOptions.audioBitrateKbps),
			     computedOptions.scaledWidthParameter,
			     computedOptions.scaledHeightParameter,
			     options.outputWidth != AUTO_SIZE && options.outputHeight != AUTO_SIZE
				     ? ",setsar=1/1"
				     : "",
			     outputPath);

	*processFinishedConnection = connect(ffmpeg, &QProcess::finished, [=, this](int exitCode) {
		EndCompression(options, outputPath, command, exitCode);
	});

	ffmpeg->startCommand(command);
}

void Compressor::EndCompression(const Options& options,
					  QString outputPath,
					  QString command,
					  int exitCode)
{
	if (exitCode != 0) {
		disconnect(*processUpdateConnection);
		disconnect(*processFinishedConnection);

		emit compressionFailed(parseOutput(), command + "\n\n" + output);
		output.clear();
		return;
	}

	QFile media(outputPath);
	if (!media.open(QIODevice::ReadOnly)) {
		disconnect(*processUpdateConnection);
		disconnect(*processFinishedConnection);
		emit compressionFailed("Could not open the compressed media.", media.errorString());
		output.clear();
		media.close();
		return;
	}

	const double BYTES_TO_KB = 125.0; // or smt like that anyway lol

	disconnect(*processUpdateConnection);
	disconnect(*processFinishedConnection);
	emit compressionSucceeded(options.sizeKbps, media.size() / BYTES_TO_KB, &media);
	output.clear();
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
