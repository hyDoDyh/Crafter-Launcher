#ifndef MINECRAFT_H
#define MINECRAFT_H

#include "minecraftversion.h"

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QFrame>
#include <QHBoxLayout>
class Minecraft : public QFrame
{
    Q_OBJECT
public:
    explicit Minecraft(QString Name,QString Type,QString Url,MinecraftVersion ver,QWidget *parent = nullptr,bool launcher = false);
private:
    QLabel* Name;
    QLabel* Type;
    QPushButton *Button;
    QString(url);
    MinecraftVersion ver;
signals:
    void ButtonClicked(MinecraftVersion ver);

};

#endif // MINECRAFT_H
