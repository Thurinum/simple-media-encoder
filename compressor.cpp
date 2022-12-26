#include "compressor.hpp"

#include <QEventLoop>
#include <QFile>
#include <QRegularExpression>
#include <QTime>
#include <QUrl>
#include <QVariant>

Compressor::Compressor(QObject* parent) : QObject{parent}

{
	ffmpeg->setReadChannel(QProcess::StandardOutput);
	ffmpeg->setProcessChannelMode(QProcess::MergedChannels);

	connect(ffmpeg, &QProcess::errorOccurred, [=](QProcess::ProcessError error) {
		emit compressionFailed("Process " + QVariant::fromValue(error).toString());
	});
}

void Compressor::compress(const QUrl& fileUrl,
				  const QString& fileSuffix,
				  VideoCodec videoCodec,
				  AudioCodec audioCodec,
				  Container container,
				  double sizeKbps,
				  double audioQualityPercent,
				  double minVideoBitrateKbps,
				  double minAudioBitrateKbps,
				  double maxAudioBitrateKbps,
				  double overshootCorrectionPercent)
{
	ffprobe->startCommand("ffprobe.exe -v error -show_entries format=duration -of "
				    "default=noprint_wrappers=1:nokey=1 "
				    + fileUrl.toLocalFile());
	ffprobe->waitForFinished();

	bool couldParseDuration = false;
	QString durationOutput = ffprobe->readAllStandardOutput();
	double durationSeconds = durationOutput.toDouble(&couldParseDuration);

	if (!couldParseDuration) {
		emit compressionFailed("An error occured while probing the media file length.\n\n"
					     + durationOutput);
		return;
	}

	double bitrateKbps = sizeKbps / durationSeconds * (1.0 - overshootCorrectionPercent);
	double audioBitrateKbps = qMax(minAudioBitrateKbps, audioQualityPercent * maxAudioBitrateKbps);
	double videoBitrateKpbs = qMax(minVideoBitrateKbps, bitrateKbps - audioBitrateKbps);

	QStringList fileName = fileUrl.fileName().split(".");
	QString outputPath = fileName.first() + "_" + fileSuffix + "."
				   + QVariant::fromValue(container).toString();

	QString command = QString("ffmpeg.exe -i %1 -c:v %2 -c:a %3 -b:v %4k -b:a %5k %6 -y")
					.arg(fileUrl.toLocalFile(),
					     QVariant::fromValue(videoCodec).toString(),
					     QVariant::fromValue(audioCodec).toString(),
					     QString::number(videoBitrateKpbs),
					     QString::number(audioBitrateKbps),
					     outputPath);

	*processUpdateConnection = connect(ffmpeg, &QProcess::readyReadStandardOutput, [=]() {
		QString output = QString(ffmpeg->readAllStandardOutput());
		QRegularExpression regex("time=([0-9][0-9]:[0-9][0-9]:[0-9][0-9].[0-9][0-9])");
		QRegularExpressionMatch match = regex.match(output);

		if (!match.hasMatch())
			return;

		int progressPercent = QTime::fromString(match.captured(1)).second() * 100
					    / durationSeconds;

		emit compressionProgressUpdate(progressPercent); // TODO
	});

	*processFinishedConnection = connect(ffmpeg, &QProcess::finished, [=](int exitCode) {
		if (exitCode != 0) {
			disconnect(*processUpdateConnection);
			disconnect(*processFinishedConnection);
			emit compressionFailed(
				"Something went wrong during compression:\n\nffmpeg exit code:"
				+ QString::number(exitCode) + "\n\nffmpeg command: " + command
				+ "\n\nffmpeg error log: " + ffmpeg->readAllStandardOutput());
			return;
		}

		QFile media(outputPath);
		if (!media.open(QIODevice::ReadOnly)) {
			disconnect(*processUpdateConnection);
			disconnect(*processFinishedConnection);
			emit compressionFailed("Could not open the resulting compressed file:\n\n\t"
						     + media.errorString());
			return;
		}

		disconnect(*processUpdateConnection);
		disconnect(*processFinishedConnection);
		emit compressionSucceeded(sizeKbps, media.size() / 125.0);
	});

	ffmpeg->startCommand(command);
}
