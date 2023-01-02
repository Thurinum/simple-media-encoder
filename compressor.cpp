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
				  int width,
				  int height,
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

	if (width < 0) {
		emit compressionFailed(
			tr("Output width must be greater than 0, but was %1").arg(QString::number(width)));
		return;
	}
	if (height < 0) {
		emit compressionFailed(tr("Output height must be greater than 0, but was %1")
						     .arg(QString::number(height)));
		return;
	}

	// get media info
	ffprobe->startCommand(QString("ffprobe.exe -v error -select_streams v:0 -show_entries "
						"stream=width,height,duration -of "
						"default=noprint_wrappers=1:nokey=1 \"%1\"")
					    .arg(inputUrl.toLocalFile()));
	ffprobe->waitForFinished();

	// TODO: Refactor to use key value pairs
	QStringList metadata = QString(ffprobe->readAll()).split("\r\n");
	bool couldParseDuration = false;
	QString durationOutput = metadata[2];
	double durationSeconds = durationOutput.toDouble(&couldParseDuration);

	if (!couldParseDuration) {
		emit compressionFailed(tr("Media file is not found, invalid, or corrupted."),
					     durationOutput);
		return;
	}

	// width and height scaling
	QString widthParam;

	if (width == AUTO_SIZE)
		widthParam = height == AUTO_SIZE ? "iw" : "-1";
	else
		widthParam = QString::number(width);

	QString heightParam = height == AUTO_SIZE ? "-2" : QString::number(height + height % 2);

	// pixel ratio
	double pixelRatio = 1;

	if (width != AUTO_SIZE && height != AUTO_SIZE) {
		double inputPixelCount = metadata[0].toInt() * metadata[1].toInt(); // w h
		double outputPixelCount = width * height;

		// TODO: Add option to enable bitrate compensation even when upscaling (will result in bigger files)
		if (outputPixelCount > inputPixelCount)
			pixelRatio = outputPixelCount / inputPixelCount;
	}

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
		= QString(
			  R"(ffmpeg.exe -i "%1" -c:v %2 -c:a %3 -b:v %4k -b:a %5k -vf scale=%6:%7%8 "%9" -y)")
			  .arg(inputUrl.toLocalFile(),
				 videoCodec.library,
				 audioCodec.library,
				 QString::number(videoBitrateKpbs),
				 QString::number(audioBitrateKbps),
				 widthParam,
				 heightParam,
				 width != AUTO_SIZE && height != AUTO_SIZE ? ",setsar=1/1" : "",
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
	ffmpeg->startCommand("ffmpeg.exe -encoders");
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
