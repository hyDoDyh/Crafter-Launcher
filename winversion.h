#ifndef WINVERSION_H
#define WINVERSION_H
#include <QOperatingSystemVersion>
class WinVersion
{
public:
    WinVersion();
    WinVersion(int major,int minor,int micro);
    WinVersion(QString str);
    QOperatingSystemVersion os = QOperatingSystemVersion::current();
    int majorVersion;
    int minorVersion;
    int microVersion;

    friend bool operator<(const WinVersion& a,const WinVersion& b){
        if(a.majorVersion != b.majorVersion)
            return a.majorVersion < b.majorVersion;
        if(a.minorVersion != b.minorVersion)
            return a.minorVersion < b.minorVersion;
        return a.microVersion < b.microVersion;
    }
    friend bool operator<=(const WinVersion& a,const WinVersion& b){
        if(a.majorVersion != b.majorVersion)
            return a.majorVersion < b.majorVersion;
        if(a.minorVersion != b.minorVersion)
            return a.minorVersion < b.minorVersion;
        return a.microVersion <= b.microVersion;
    }
    friend bool operator>(const WinVersion& a,const WinVersion& b){
        if(a.majorVersion != b.majorVersion)
            return a.majorVersion > b.majorVersion;
        if(a.minorVersion != b.minorVersion)
            return a.minorVersion > b.minorVersion;
        return a.microVersion > b.microVersion;
    }
    friend bool operator>=(const WinVersion& a,const WinVersion& b){
        if(a.majorVersion != b.majorVersion)
            return a.majorVersion > b.majorVersion;
        if(a.minorVersion != b.minorVersion)
            return a.minorVersion > b.minorVersion;
        return a.microVersion >= b.microVersion;
    }
};

#endif // WINVERSION_H
