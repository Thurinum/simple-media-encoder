#ifndef FORMAT_SUPPORT_H
#define FORMAT_SUPPORT_H

#include "formats/codec.hpp"
#include "formats/container.hpp"

struct FormatSupport {
    const QList<Codec> videoCodecs;
    const QList<Codec> audioCodecs;
    const QList<Container> containers;
};

#endif
