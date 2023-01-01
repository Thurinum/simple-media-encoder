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
	connect(ffmpeg, &QProcess::errorOccurred, [=](QProcess::ProcessError error) {
		emit compressionFailed("Process " + QVariant::fromValue(error).toString());
	});
}

void Compressor::compress(const QUrl& inputUrl,
				  const QDir& outputDir,
				  const QString& fileSuffix,
				  const Format& videoCodec,
				  const Format& audioCodec,
				  const QString& container,
				  double sizeKbps,
				  double audioQualityPercent,
				  double minVideoBitrateKbps,
				  double minAudioBitrateKbps,
				  double maxAudioBitrateKbps,
				  double overshootCorrectionPercent)
{
	if (!videoCodecs.contains(videoCodec)) {
		emit compressionFailed("Invalid video codec " + videoCodec.name,
					     "Available codecs: " + Format::stringFromList(videoCodecs));
		return;
	}
	if (!audioCodecs.contains(audioCodec)) {
		emit compressionFailed("Invalid audio codec " + audioCodec.name,
					     "Available codecs: " + Format::stringFromList(audioCodecs));
		return;
	}
	if (!containers.contains(container)) {
		emit compressionFailed("Invalid container " + container,
					     "Available codecs: " + containers.join(", "));
		return;
	}

	ffprobe->startCommand(QString("ffprobe.exe -v error -show_entries format=duration -of "
						"default=noprint_wrappers=1:nokey=1 \"%1\"")
					    .arg(inputUrl.toLocalFile()));
	ffprobe->waitForFinished();

	bool couldParseDuration = false;
	QString durationOutput = ffprobe->readAll();
	double durationSeconds = durationOutput.toDouble(&couldParseDuration);

	if (!couldParseDuration) {
		emit compressionFailed("Media file is not found, invalid, or corrupted.",
					     durationOutput);
		return;
	}

	double bitrateKbps = sizeKbps / durationSeconds * (1.0 - overshootCorrectionPercent);
	double audioBitrateKbps = qMax(minAudioBitrateKbps, audioQualityPercent * maxAudioBitrateKbps);
	double videoBitrateKpbs = qMax(minVideoBitrateKbps, bitrateKbps - audioBitrateKbps);

	emit compressionStarted(videoBitrateKpbs, audioBitrateKbps);

	QStringList fileName = inputUrl.fileName().split(".");
	QString outputPath = outputDir.filePath(fileName.first() + "_" + fileSuffix + "." + container);

	QString command = QString("ffmpeg.exe -i \"%1\" -c:v %2 -c:a %3 -b:v %4k -b:a %5k \"%6\" -y")
					.arg(inputUrl.toLocalFile(),
					     videoCodec.library,
					     audioCodec.library,
					     QString::number(videoBitrateKpbs),
					     QString::number(audioBitrateKbps),
					     outputPath);

	*processUpdateConnection = connect(ffmpeg, &QProcess::readyRead, [=]() {
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

	*processFinishedConnection = connect(ffmpeg, &QProcess::finished, [=](int exitCode) {
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
			return;
		}

		disconnect(*processUpdateConnection);
		disconnect(*processFinishedConnection);
		emit compressionSucceeded(sizeKbps, media.size() / 125.0);
		output.clear();
	});

	ffmpeg->startCommand(command);
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

bool Compressor::Format::operator==(const Format& rhs) const
{
	return this->name == rhs.name && this->library == rhs.library;
}

QString Compressor::Format::stringFromList(const QList<Format>& list)
{
	QString str;

	for (const Format& format : list)
		str += format.name % ", ";

	str.chop(2);

	return str;
}
