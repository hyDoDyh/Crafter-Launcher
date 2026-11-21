#include "logger.h"
#include <QObject>
#include <QDir>
Logger* Logger::Instance = nullptr;
Logger::Logger() {
    QDir().mkpath("log");   // 确保目录存在

    QDateTime now = QDateTime::currentDateTime();
    QString name = now.toString("yyyy-MM-dd_HH-mm-ss-zzz") + ".txt";
    file.setFileName("log/" + name);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Logger: cannot open" << file.fileName()
        << ":" << file.errorString();
    }
}
void Logger::info(QString message){

    QMutexLocker locker(&m_mutex);
    emit OnLog(message);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QDateTime now = QDateTime::currentDateTime();
        QTextStream out(&file);
        QString time = now.toString("[yyyy-MM-dd HH:mm:ss][info]:");
        out << time << message << '\n';
        file.close();
    }

}
void Logger::warning(QString message){

    QMutexLocker locker(&m_mutex);
    emit OnLog(message);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QDateTime now = QDateTime::currentDateTime();
        QTextStream out(&file);
        QString time = now.toString("[yyyy-MM-dd HH:mm:ss][info]:");
        out << time << message << '\n';
        file.close();
    }

}
void Logger::error(QString message){

    QMutexLocker locker(&m_mutex);
    emit OnLog(message);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QDateTime now = QDateTime::currentDateTime();
        QTextStream out(&file);
        QString time = now.toString("[yyyy-MM-dd HH:mm:ss][info]:");
        out << time << message << '\n';
        file.close();
    }

}

