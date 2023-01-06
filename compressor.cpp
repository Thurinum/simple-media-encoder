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

void Compressor::compress(const QUrl& inputUrl,
				  const QDir& outputDir,
				  const QString& fileSuffix,
				  const Codec& videoCodec,
				  const Codec& audioCodec,
				  const Container& container,
				  double sizeKbps,
				  double audioQualityPercent,
				  int outputWidth,
				  int outputHeight,
				  double minVideoBitrateKbps,
				  double minAudioBitrateKbps,
				  double maxAudioBitrateKbps,
				  double overshootCorrectionPercent)
{
	if (!container.supportedCodecs.contains(videoCodec.library)) {
		emit compressionFailed(
			tr("Selected container %1 does not support selected video codec: %2.")
				.arg(container.name.toUpper(), videoCodec.name));
		return;
	}

	if (!container.supportedCodecs.contains(audioCodec.library)) {
		emit compressionFailed(
			tr("Selected container %1 does not support selected audio codec: %2.")
				.arg(container.name.toUpper(), audioCodec.name));
		return;
	}

	if (outputWidth < 0) {
		emit compressionFailed(tr("Output width must be greater than 0, but was %1")
						     .arg(QString::number(outputWidth)));
		return;
	}
	if (outputHeight < 0) {
		emit compressionFailed(tr("Output height must be greater than 0, but was %1")
						     .arg(QString::number(outputHeight)));
		return;
	}

	ffprobe->startCommand(QString("ffprobe%1 -v error -select_streams v:0 -show_entries "
						"stream=width,height,display_aspect_ratio,duration -of "
						"default=noprint_wrappers=1:nokey=1 \"%2\"")
					    .arg(IS_WINDOWS ? ".exe" : "", inputUrl.toLocalFile()));
	ffprobe->waitForFinished();

	// TODO: Refactor to use key value pairs
	QStringList metadata = QString(ffprobe->readAll()).split(QRegularExpression("[\n\r]+"));

	if (metadata.count() != 5) {
		emit compressionFailed(tr("Could not retrieve media metadata. Is the file corrupted?"),
					     tr("Input file: %1\nFound metadata: %2")
						     .arg(inputUrl.toLocalFile(), metadata.join(", ")));
		return;
	}

	bool couldParseDuration = false;
	QString durationOutput = metadata[3];
	double durationSeconds = durationOutput.toDouble(&couldParseDuration);

	if (!couldParseDuration) {
		emit compressionFailed(tr("Failed to parse media duration. Is the file corrupted?"),
					     tr("Invalid media duration '%1'.").arg(durationOutput));
		return;
	}

	// width and height scaling
	QString widthParam;

	if (outputWidth == AUTO_SIZE)
		widthParam = outputHeight == AUTO_SIZE ? "iw" : "-1";
	else
		widthParam = QString::number(outputWidth);

	QString heightParam = outputHeight == AUTO_SIZE
					    ? "-2"
					    : QString::number(outputHeight + outputHeight % 2);

	// pixel ratio
	double pixelRatio = 1;

	QStringList aspectRatio = metadata[2].split(":");
	int aspectRatioW = aspectRatio.first().toInt();
	int aspectRatioH = aspectRatio.last().toInt();
	int inputWidth = metadata[0].toInt();
	int inputHeight = metadata[1].toInt();

	double inputPixelCount = inputWidth * inputHeight;

	if (outputWidth == AUTO_SIZE) {
		outputWidth = outputHeight * aspectRatioW / aspectRatioH;
	} else {
		outputHeight = outputWidth * aspectRatioW / aspectRatioH;
	}

	double outputPixelCount = outputWidth * outputHeight;

	// TODO: Add option to enable bitrate compensation even when upscaling (will result in bigger files)
	if (outputPixelCount > 0 && outputPixelCount < inputPixelCount)
		pixelRatio = outputPixelCount / inputPixelCount;

	// calculate bitrate
	double bitrateKbps = sizeKbps / durationSeconds * (1.0 - overshootCorrectionPercent);
	double audioBitrateKbps = qMax(minAudioBitrateKbps, audioQualityPercent * maxAudioBitrateKbps);
	double videoBitrateKpbs = qMax(minVideoBitrateKbps,
						 pixelRatio * (bitrateKbps - audioBitrateKbps));

	if (audioBitrateKbps < audioCodec.minBitrateKbps) {
		emit compressionFailed(tr("Selected audio codec %1 requires a minimum bitrate of %2 "
						  "kbps, but requested was %3 kbps.")
						     .arg(audioCodec.name,
							    QString::number(audioCodec.minBitrateKbps),
							    QString::number(audioBitrateKbps)));
		return;
	}

	emit compressionStarted(videoBitrateKpbs, audioBitrateKbps);

	QStringList fileName = inputUrl.fileName().split(".");
	QString outputPath = outputDir.filePath(fileName.first() + "_" + fileSuffix + "."
							    + container.name);
	QString command
		= QString(R"(ffmpeg%1 -i "%2" -c:v %3 -c:a %4 %5 -b:a %6k -vf scale=%7:%8%9 "%10" -y)")
			  .arg(IS_WINDOWS ? ".exe" : "",
				 inputUrl.toLocalFile(),
				 videoCodec.library,
				 audioCodec.library,
				 sizeKbps > AUTO_SIZE
					 ? QString("-b:v %1").arg(QString::number(videoBitrateKpbs))
					 : "",
				 QString::number(audioBitrateKbps),
				 widthParam,
				 heightParam,
				 outputWidth != AUTO_SIZE && outputHeight != AUTO_SIZE ? ",setsar=1/1" : "",
				 outputPath);

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

	*processFinishedConnection = connect(ffmpeg, &QProcess::finished, [=, this](int exitCode) {
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
			emit compressionFailed("Could not open the compressed media.",
						     media.errorString());
			output.clear();
			media.close();
			return;
		}

		const double BYTES_TO_KB = 125.0; // or smt like that anyway lol

		disconnect(*processUpdateConnection);
		disconnect(*processFinishedConnection);
		emit compressionSucceeded(sizeKbps, media.size() / BYTES_TO_KB, &media);
		output.clear();
		media.close();
	});

	ffmpeg->startCommand(command);
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
