#include "compressor.hpp"
#include "qdebug.h"

#include <QFile>
#include <QUrl>

void Compressor::compress(const QUrl& fileUrl,
				  const QString& fileSuffix,
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
		emit compressionFailed("An error occured while probing the media file length:\n\n\t"
					     + durationOutput);
		return;
	}

	double bitrateKbps = sizeKbps / durationSeconds * (1.0 - overshootCorrectionPercent);
	double audioBitrateKbps = qMax(minAudioBitrateKbps, audioQualityPercent * maxAudioBitrateKbps);
	double videoBitrateKpbs = qMax(minVideoBitrateKbps, bitrateKbps - audioBitrateKbps);

	QStringList fileName = fileUrl.fileName().split(".");
	QString outputPath = fileName.first() + "_" + fileSuffix + "." + fileName.last();

	// ffmpeg.exe -i "C:\Users\Thurinum\Videos\bellec.mp4" -c:v libaom-av1 -c:a libopus -b:v 1000k -b:a 128k test.mp4 -y

	QString command
		= QString("ffmpeg.exe -i %1 -c:v libaom-av1 -c:a libopus -b:v %2k -b:a %3k %4 -y")
			  .arg(fileUrl.toLocalFile(),
				 QString::number(videoBitrateKpbs),
				 QString::number(audioBitrateKbps),
				 outputPath);
	ffmpeg->startCommand(command);

	auto a = connect(ffmpeg, &QProcess::finished, [=]() {
		QFile media(outputPath);

		if (!media.open(QIODevice::ReadOnly)) {
			emit compressionFailed("Could not open the resulting compressed file:\n\n\t"
						     + media.errorString());
			return;
		}

		emit compressionSucceeded(sizeKbps, media.size() / 125.0);
	});
}
