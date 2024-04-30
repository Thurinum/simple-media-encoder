#ifndef CODEC_H
#define CODEC_H

#include <QString>

struct Codec {
    const QString displayName;
    const QString libraryName;
    const bool isAudioCodec;
};

#endif
