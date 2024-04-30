#ifndef CONTAINER_H
#define CONTAINER_H

#include <QList>

struct Codec;

struct Container {
    const QString name;
    const QList<Codec> supportedCodecs;
};

#endif
