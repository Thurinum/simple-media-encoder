#ifndef CODEC_H
#define CODEC_H

#include <QMetaType>
#include <QString>

struct Codec {
    QString displayName;
    QString libraryName;
    bool isAudioCodec;
};

Q_DECLARE_METATYPE(Codec)

#endif
