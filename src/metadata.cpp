#include "metadata.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

std::variant<Metadata, Metadata::Error> Metadata::Builder::fromJson(QByteArray data)
{
	QJsonDocument document = QJsonDocument::fromJson(data);

	if (document.isNull()) {
		return Error{QObject::tr("Could not retrieve media metadata. Is the file corrupted?"),
				 QObject::tr("Found metadata: %1").arg(data)};
	}

	QJsonObject root	  = document.object();
	QJsonArray	streams = root.value("streams").toArray();

	// we only support 1 stream of each type at the moment
	QJsonObject format = root.value("format").toObject();
	QJsonObject video;
	QJsonObject audio;

	for (QJsonValueRef streamRef : streams) {
		QJsonObject stream = streamRef.toObject();
		QJsonValue	type	 = stream.value("codec_type");

		if (type == "video" && video.isEmpty()) {
			video = stream;
		}

		if (type == "audio" && audio.isEmpty()) {
			audio = stream;
		}
	}

	if (video.isEmpty() && audio.isEmpty()) {
		return Error{QObject::tr("Media metadata is incomplete."), QObject::tr("Found metadata: %1").arg(data)};
	}

	Metadata metadata;
	Error	   error{QObject::tr("Media metadata is incomplete."), ""};

	auto value = [&error](QJsonObject& source, const QString& key) -> QVariant {
		if (!source.contains(key)) {
			error.details += QObject::tr("Could not find key '%1'.\n").arg(key);
			return {};
		}

		return source.value(key).toVariant();
	};

	double	duration	= value(format, "duration").toDouble();
	double	nbrFrames	= value(video, "nb_frames").toDouble();
	QStringList aspectRatio = value(video, "display_aspect_ratio").toString().split(':');

	metadata = Metadata{
		.width		= value(video, "width").toDouble(),
		.height		= value(video, "height").toDouble(),
		.sizeKbps		= value(format, "size").toDouble() * 0.001,
		.audioBitrateKbps = value(audio, "bit_rate").toDouble() * 0.001,
		.durationSeconds	= duration,
		.aspectRatioX	= aspectRatio.first().toDouble(),
		.aspectRatioY	= aspectRatio.last().toDouble(),
		.frameRate		= nbrFrames / duration,
		.videoCodec		= value(video, "codec_name").toString(),
		.audioCodec		= value(audio, "codec_name").toString(),
		.container		= "" // TODO: Find a reliable way to query format type
	};

	if (!error.details.isEmpty()) {
		error.details += QObject::tr("Raw metadata: %1").arg(data);
		return error;
	}

	return metadata;
}
