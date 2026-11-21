#ifndef LOGGER_H
#define LOGGER_H
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutexLocker>
#include <QMutex>
class Logger : public QObject
{
    Q_OBJECT
public:
    static Logger* getInstance(){
        if(Instance != nullptr)
            return Instance;
        return Instance = new Logger("log.txt");
    }
    void info(QString massage);
    void warning(QString massage);
    void error(QString massage);
    void setFilePath(QString path);
private:
    static Logger* Instance;
    QFile file;
    QMutex m_mutex;
    Logger(QString path);
signals:
    void OnLog(QString message);
};

#endif // LOGGER_H
