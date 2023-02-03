#ifndef METADATABUILDER_HPP
#define METADATABUILDER_HPP

#include <QJsonObject>
#include <QObject>
#include <QPoint>
#include <QString>
#include <variant>

struct Metadata
{
	double  width;
	double  height;
	double  sizeKbps;
	double  audioBitrateKbps;
	double  durationSeconds;
	double  aspectRatioX;
	double  aspectRatioY;
	double  frameRate;
	QString videoCodec;
	QString audioCodec;
	QString container;

	class Builder;
	struct Error;
};

struct Metadata::Error
{
	QString summary;
	QString details;
};

class Metadata::Builder
{
public:
	std::variant<Metadata, Metadata::Error> fromJson(QByteArray data);

private:
	double			  getFrameRate();
	std::pair<double, double> getAspectRatio();

	inline QVariant value(QJsonObject& source, const QString& key, bool required = false)
	{
		if (source.isEmpty())
			return {};

		if (source.contains(key))
			return source.value(key).toVariant();

		if (required)
			NotFound(key);

		return {};
	}
	inline void NotFound(const QString& datum)
	{
		error.details += QObject::tr("Unable to find or compute %1.\n").arg(datum);
	}

	QJsonObject	    format;
	QJsonObject	    video;
	QJsonObject	    audio;
	Metadata::Error error;
};
#endif // METADATABUILDER_HPP
