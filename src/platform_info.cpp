#include "platform_info.hpp"

#include <QOffscreenSurface>
#include <QOpenGLFunctions>
#include <QProcess>
#include <QString>

PlatformInfo::PlatformInfo() { m_isNvidia = DetectNvidia(); }

bool PlatformInfo::DetectNvidia()
{
    QOpenGLContext context;
    context.create();

    QOffscreenSurface surface;
    surface.setFormat(context.format());
    surface.create();
    context.makeCurrent(&surface);

    QOpenGLFunctions functions;
    functions.initializeOpenGLFunctions();

    const GLubyte* vendor = functions.glGetString(GL_VENDOR);
    QString vendorString
        = QString::fromUtf8(reinterpret_cast<const char*>(vendor));

    return vendorString.contains("NVIDIA", Qt::CaseInsensitive);
}
