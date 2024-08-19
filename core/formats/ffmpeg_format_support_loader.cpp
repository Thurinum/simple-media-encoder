#include "ffmpeg_format_support_loader.hpp"
#include "codec.hpp"
#include "core/notifier/notifier.hpp"

#include <QProcess>
#include <QRegularExpression>
#include <QThread>

FFmpegFormatSupportLoader::FFmpegFormatSupportLoader()
    : codecsProcess(new QProcess(this))
    , containersProcess(new QProcess(this)) { }

void FFmpegFormatSupportLoader::QuerySupportedFormatsAsync()
{
    if (cachedFormats != nullptr) {
        emit queryCompleted(cachedFormats);
        return;
    }

    // TODO: Make commands cross-platform
    connect(codecsProcess, &QProcess::finished, this, &FFmpegFormatSupportLoader::onCodecsQueried);
    codecsProcess->startCommand(R"(ffmpeg.exe -encoders -hide_banner)");

    connect(containersProcess, &QProcess::finished, this, &FFmpegFormatSupportLoader::onContainersQueried);
    containersProcess->startCommand(R"(ffmpeg.exe -muxers -hide_banner)");
}

void FFmpegFormatSupportLoader::onCodecsQueried()
{
    codecs = parseCodecs();
    codecsQueried = true;

    CheckQueryComplete();
}

void FFmpegFormatSupportLoader::onContainersQueried()
{
    containers = parseContainers();
    containersQueried = true;

    CheckQueryComplete();
}

QPair<QList<Codec>, QList<Codec>> FFmpegFormatSupportLoader::parseCodecs()
{
    if (!EnsureValidResult(codecsProcess))
        return {};

    QList<Codec> videoCodecs;
    QList<Codec> audioCodecs;

    SkipLines(10);

    while (codecsProcess->canReadLine()) {
        QString line = codecsProcess->readLine().trimmed();
        bool isAudio;

        // TODO: Audio boolean member might be unnecessary
        if (line[0] == 'V')
            isAudio = false;
        else if (line[0] == 'A')
            isAudio = true;
        else
            continue;

        static QRegularExpression delimiter("\\s");
        QString libraryName = line.section(delimiter, 1, 2).trimmed();
        QString displayName = line.section(delimiter, 2).trimmed();

        Codec codec { displayName, libraryName, isAudio };

        if (isAudio)
            audioCodecs.append(codec);
        else
            videoCodecs.append(codec);
    }

    return { videoCodecs, audioCodecs };
}

QList<Container> FFmpegFormatSupportLoader::parseContainers()
{
    if (!EnsureValidResult(containersProcess))
        return {};

    QList<Container> containers;

    while (containersProcess->canReadLine()) {
        QString line = containersProcess->readLine().trimmed();

        if (line[0] != 'E')
            continue;

        static QRegularExpression delimiter("\\s");
        QString libraryName = line.section(delimiter, 1, 2).trimmed();
        QString displayName = line.section(delimiter, 2).trimmed();

        containers.append({ displayName, libraryName });
    }

    return containers;
}

bool FFmpegFormatSupportLoader::EnsureValidResult(QProcess* process)
{
    if (process->exitStatus() == QProcess::CrashExit || process->exitCode() != 0) {
        emit queryCompleted(Message(
            Severity::Critical,
            QObject::tr("Failed to query supported formats"),
            QObject::tr("Asking FFmpeg for available encoders failed because: '%1'.").arg(process->errorString()),
            process->readAllStandardError()
        ));

        return false;
    }

    return true;
}

void FFmpegFormatSupportLoader::SkipLines(size_t count)
{
    for (size_t i = 0; i < count; i++) {
        codecsProcess->readLine();
    }
}

void FFmpegFormatSupportLoader::CheckQueryComplete()
{
    if (codecsQueried && containersQueried) {
        cachedFormats = QSharedPointer<FormatSupport>::create(codecs.first, codecs.second, containers);
        emit queryCompleted(cachedFormats);
    }
}
