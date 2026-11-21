QT       += core gui
QT       += core gui network concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    button.cpp \
    download.cpp \
    downloadpage.cpp \
    downloadthread.cpp \
    launcharguments.cpp \
    launcher.cpp \
    launcherpage.cpp \
    library.cpp \
    logindialog.cpp \
    logger.cpp \
    main.cpp \
    mainwindow.cpp \
    microsoftauth.cpp \
    minecraft.cpp \
    minecraftversion.cpp \
    minectaftmodel.cpp \
    playerinfo.cpp \
    thememanger.cpp \
    winversion.cpp

HEADERS += \
    button.h \
    download.h \
    downloadpage.h \
    downloadthread.h \
    launcharguments.h \
    launcher.h \
    launcherpage.h \
    library.h \
    logindialog.h \
    logger.h \
    mainwindow.h \
    microsoftauth.h \
    minecraft.h \
    minecraftversion.h \
    minectaftmodel.h \
    playerinfo.h \
    thememanger.h \
    winversion.h

FORMS += \
    downloadpage.ui \
    launcherpage.ui \
    mainwindow.ui

TRANSLATIONS += \
    Crafter_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
# 将目标路径 $$OUT_PWD 替换为 $$DESTDIR
# 1. 根据当前的构建模式，决定目标文件夹路径
CONFIG(debug, debug|release) {
    TARGET_DIR = $$OUT_PWD/debug
} else {
    TARGET_DIR = $$OUT_PWD/release
}

# 2. 将目标路径替换为刚才计算出的 TARGET_DIR
QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$PWD/third_party/quazip/bin/libquazip1-qt6.dll) $$shell_path($$TARGET_DIR) && \
                   $$QMAKE_COPY $$shell_path($$PWD/theme.json) $$shell_path($$TARGET_DIR)
RESOURCES += \
    Crafter.qrc

# 1. 添加头文件路径
# 指向包含 quazip.h, JlCompress.h 等文件的目录
INCLUDEPATH += $$PWD/third_party/quazip/include
# 如果 zlib 的头文件单独存放，也需要添加
INCLUDEPATH += $$PWD/third_party/zlib/include

LIBS += -L$$PWD/third_party/quazip/lib -lquazip1-qt6

# 3. (可选) 根据编译模式定义宏
# 如果是静态链接 QuaZip，通常需要定义 QUAZIP_STATIC
# DEFINES += QUAZIP_STATIC
