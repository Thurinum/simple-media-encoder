#ifndef METADATA_HPP
#define METADATA_HPP

#include <QJsonObject>
#include <QObject>
#include <QPoint>
#include <QString>

struct Metadata {
    double width;
    double height;
    double sizeKbps;
    double audioBitrateKbps;
    double durationSeconds;
    double aspectRatioX;
    double aspectRatioY;
    double frameRate;
    QString videoCodec;
    QString audioCodec;
    QString container;
};
#endif // METADATA_HPP
