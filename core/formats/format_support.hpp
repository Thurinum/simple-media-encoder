#ifndef FORMAT_SUPPORT_H
#define FORMAT_SUPPORT_H

#include "codec.hpp"
#include "container.hpp"

struct FormatSupport {
    const QList<Codec> videoCodecs;
    const QList<Codec> audioCodecs;
    const QList<Container> containers;
};

#endif
