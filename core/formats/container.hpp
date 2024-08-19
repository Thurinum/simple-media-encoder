#ifndef CONTAINER_H
#define CONTAINER_H

#include <QList>
#include <QMetaType>

struct Codec;

struct Container {
    QString displayName;
    QString formatName;
};

Q_DECLARE_METATYPE(Container)

#endif
