#include "logger.h"
#include <QObject>
Logger* Logger::Instance = nullptr;
Logger::Logger(QString Path) :file(Path) {
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

