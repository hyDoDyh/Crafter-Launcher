#ifndef LIBRARY_H
#define LIBRARY_H
#include <QString>
class Library
{
public:
    Library(QString p,QString u,QString n) : path(p) , url(u), name(n){

    }
    const QString getPath(){return path;}
    const QString getUrl(){return url;}
    const QString getName(){return name;}
private:
    QString path;
    QString url;
    QString name;
};

#endif // LIBRARY_H
