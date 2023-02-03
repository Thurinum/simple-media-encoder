#ifndef METADATABUILDER_HPP
#define METADATABUILDER_HPP

#include <QObject>
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

class Metadata::Builder
{
public:
	static std::variant<Metadata, Metadata::Error> fromJson(QByteArray data);

private:
	void parseFps();
};

struct Metadata::Error
{
	QString summary;
	QString details;
};

#endif // METADATABUILDER_HPP
