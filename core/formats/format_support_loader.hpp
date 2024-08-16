#ifndef FORMAT_SUPPORT_LOADER_H
#define FORMAT_SUPPORT_LOADER_H

#include <QObject>

#include "format_support.hpp"
#include "core/notifier/notifier.hpp"

class FormatSupportLoader : public QObject
{
    Q_OBJECT

public:
    virtual void QuerySupportedFormatsAsync() = 0;

signals:
    void queryCompleted(std::variant<QSharedPointer<FormatSupport>, Message> maybeFormats);
};

#endif
