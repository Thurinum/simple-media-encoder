#ifndef FFMPEG_FORMAT_SUPPORT_LOADER_H
#define FFMPEG_FORMAT_SUPPORT_LOADER_H

#include "format_support.hpp"
#include "format_support_loader.hpp"

#include <QEventLoop>
#include <QList>
#include <QProcess>
#include <QSharedPointer>

class FFmpegFormatSupportLoader : public FormatSupportLoader
{
    Q_OBJECT

public:
    FFmpegFormatSupportLoader();

    void QuerySupportedFormatsAsync() override;

private slots:
    void onCodecsQueried();
    void onContainersQueried();

private:
    QPair<QList<Codec>, QList<Codec>> parseCodecs();
    QList<Container> parseContainers();
    bool EnsureValidResult(QProcess* process);
    void SkipLines(size_t count);
    void CheckQueryComplete();

    QProcess* codecsProcess;
    QProcess* containersProcess;
    bool codecsQueried = false;
    bool containersQueried = false;
    QMetaObject::Connection connection;
    QSharedPointer<FormatSupport> cachedFormats;

    QPair<QList<Codec>, QList<Codec>> codecs;
    QList<Container> containers;
};

#endif
