#include "mainwindow.h"
#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QSettings>
int main(int argc, char *argv[])
{

    QCoreApplication::setOrganizationName("Craftable Studio");
    QCoreApplication::setApplicationName("Crafter");

    QApplication a(argc, argv);
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Crafter_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }


    QString iniPath = QCoreApplication::applicationDirPath();

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, iniPath);
    MainWindow w;
    w.show();
    return a.exec();
}
