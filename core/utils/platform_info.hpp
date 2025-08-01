#ifndef PLATFORM_INFO_HPP
#define PLATFORM_INFO_HPP

#include <QString>
#include <QSysInfo>

class PlatformInfo
{
public:
    PlatformInfo();

    bool isWindows() const { return m_isWindows; };
    bool isNvidia() const { return m_isNvidia; };

private:
    bool DetectNvidia() const;

    const bool m_isWindows = QSysInfo::kernelType() == "winnt";
    bool m_isNvidia;
};

#endif
