#include "minecraft.h"

Minecraft::Minecraft(QString Name,QString Type,QString Url,MinecraftVersion ver,QWidget *parent,bool launcher)
    : QFrame{parent}
{


    this->Name = new QLabel(this);
    this->Type = new QLabel(this);
    this->Button = new QPushButton(this);

    this->ver = ver;
    this->Name->setText(Name);
    this->Type->setText(Type);
    this->url = Url;
    this->Button->setText(launcher? "启动":"下载");
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addStretch();
    layout->addWidget(this->Name);
    layout->addWidget(this->Type);
    layout->addWidget(this->Button);
    connect(this->Button,&QPushButton::clicked,this ,[=](){
        emit ButtonClicked(this->ver);
    });
}
