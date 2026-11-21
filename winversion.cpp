#include "winversion.h"

WinVersion::WinVersion() {
    os = QOperatingSystemVersion::current();
    majorVersion = os.majorVersion();
    minorVersion = os.minorVersion();
    microVersion = os.microVersion();

}
WinVersion::WinVersion(int major,int minor,int micro){
    majorVersion = major;
    minorVersion = minor;
    microVersion = micro;
}
WinVersion::WinVersion(QString str){
    QStringList list = str.split(".");
    majorVersion = list[0].toInt();
    minorVersion = list[1].toInt();
    microVersion = list[2].toInt();
}
