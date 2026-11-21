#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "launcherpage.h"
#include "thememanger.h"
#include <QPropertyAnimation>
#include <QPushButton>
#include <QDebug>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QObject>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    logger = Logger::getInstance();
    logger->info("进程已启动");
    ui->setupUi(this);
    connect(logger,&Logger::OnLog,this,[=](QString message){
        ui->statusbar->showMessage(message);
        qDebug() << message;
    });
    //ui->centralwidget->setStyleSheet("background-color: #1111cc;");
    //setWindowFlags(Qt::FramelessWindowHint);
    setStyleSheet(R"(
            Minecraft {
                border: 1px solid #444;
                border-radius: 10px;
                background-color: #222;
            }
        )");
    setFixedSize(this->size());
    LauncherPageInstance = new LauncherPage(this);
    DownloadPageInstance = new DownloadPage(this);
    ui->stackedWidget->addWidget(LauncherPageInstance);
    ui->stackedWidget->addWidget(DownloadPageInstance);
    //加载UI
    QString content;
    QFile file("theme.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        content = in.readAll();
        file.close();
    }
    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject obj = json.object();
    int zoom = obj["zoom"].toInt();
    QJsonArray arr = obj["items"].toArray();
    ui->stackedWidget->move(0,600);
    for(const QJsonValue &item : arr){
        QString name = item["name"].toString();
        QString content = item["content"].toString();
        int posX = item["position"]["x"].toInt() * zoom;
        int posY = item["position"]["y"].toInt() * zoom;
        int startPosX = item["start"]["x"].toInt() * zoom;
        int startPosY = item["start"]["y"].toInt() * zoom;
        int endPosX = item["end"]["x"].toInt() * zoom;
        int endPosY = item["end"]["y"].toInt() * zoom;
        int sizeX = item["size"]["x"].toInt() * zoom;
        int sizeY = item["size"]["y"].toInt() * zoom;
        QString styleSheet = item["styleSheet"].toString();
        QString motion = item["motion"].toString();
        QWidget *obj = this->findChild<QWidget*>(name);
        if(obj){

            obj->setStyleSheet(styleSheet);

            QPushButton *btn = qobject_cast<QPushButton*>(obj);
            if(btn){
            btn->setText(content);
            }

            //动画
            QPropertyAnimation *anim =
                new QPropertyAnimation(obj, "geometry");
            anim->setDuration(600);
            anim->setStartValue(QRect(startPosX, startPosY, sizeX, sizeY));
            anim->setEndValue(QRect(posX, posY, sizeX, sizeY));
            anim->setEasingCurve(ThemeManger::fromString(motion));
            anim->start();


            obj->setProperty("motion",ThemeManger::fromString(motion));
            obj->setProperty("startPos",QPoint(startPosX,startPosY));
            obj->setProperty("endPos",QPoint(endPosX,endPosY));
            // QWidget 自带可写的 "pos" 属性，不能用它保存自定义位置，
            // 否则动画结束后读到的会是控件当前坐标。
            obj->move(posX, posY);
            obj->setProperty("homePos",QPoint(posX,posY));
            obj->setProperty("sizeX",sizeX);
            obj->setProperty("sizeY",sizeY);
            logger->info("主题:已加载按钮"+name);
        }else{
            logger->error("主题:未找到名为"+name+"的按钮");
        }
    }

    // “返回主页面”按钮属于主窗口，堆叠在 centralwidget 之上，不随页面滚动/动画。
    m_homeButton = new QPushButton(QStringLiteral("返回主页面"), this);
    m_homeButton->setObjectName("HomeButton");
    m_homeButton->setGeometry(10, 10, 120, 36);
    m_homeButton->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #ffffff; color: #222222;"
        " border: 1px solid #d0d0d0; border-radius: 6px; padding: 4px 8px; }"
        "QPushButton:hover { background-color: #f2f2f2; }"
        "QPushButton:pressed { background-color: #e6e6e6; }"));
    m_homeButton->hide();
    m_homeButton->raise();
    connect(m_homeButton, &QPushButton::clicked, this, &MainWindow::ShowHome);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::ExitAnim(){
    //主菜单退出动画
    if (m_homeButton) {
        m_homeButton->show();
        m_homeButton->raise();
    }
    const QList<QWidget*> widgets = this->centralWidget()->findChildren<QWidget*>( Qt::FindDirectChildrenOnly);
    for(QWidget *object : widgets){
        QPushButton *btn = qobject_cast<QPushButton*>(object);
        QStackedWidget *wid = qobject_cast<QStackedWidget*>(object);
        if(btn || wid){
            QEasingCurve::Type type = static_cast<QEasingCurve::Type>(object->property("motion").toInt());
            QPoint endPoint = object->property("endPos").toPoint();
            QPoint pos = object->property("homePos").toPoint();
            QRect currentGeometry = object->geometry();
            int endX = endPoint.x();
            int endY = endPoint.y();
            int sizeX = object->property("sizeX").toInt();
            int sizeY = object->property("sizeY").toInt();
            //动画


            qDebug()<<(object->objectName())<<",from"<<pos<<",to"<<endPoint;

            QPropertyAnimation *anim = new QPropertyAnimation(object, "geometry");
            anim->setDuration(600);
            anim->setStartValue(currentGeometry);
            anim->setEndValue(QRect(endX,endY,sizeX,sizeY));
            anim->setEasingCurve(type);
            anim->start();
        }
    }
}

void MainWindow::ShowHome()
{
    logger->info(QStringLiteral("返回主页面：开始动画"));
    if (m_homeButton)
        m_homeButton->hide();
    // 从启动页/下载页返回主菜单：把页面移回下方，主菜单按钮飞回原位。
    const QList<QWidget*> widgets = this->centralWidget()->findChildren<QWidget*>(Qt::FindDirectChildrenOnly);
    for (QWidget *object : widgets) {
        QPushButton *btn = qobject_cast<QPushButton*>(object);
        QStackedWidget *wid = qobject_cast<QStackedWidget*>(object);
        if (!btn && !wid)
            continue;

        QEasingCurve::Type type = static_cast<QEasingCurve::Type>(object->property("motion").toInt());
        const QPoint pos = object->property("homePos").toPoint();
        const int sizeX = object->property("sizeX").toInt();
        const int sizeY = object->property("sizeY").toInt();

        QPropertyAnimation *anim = new QPropertyAnimation(object, "geometry");
        anim->setDuration(600);
        anim->setStartValue(object->geometry());
        anim->setEndValue(QRect(pos.x(), pos.y(), sizeX, sizeY));
        anim->setEasingCurve(type);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void MainWindow::on_LauncherButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    qDebug() << "进入LauncherPage!!!";
    this->ExitAnim();

}
void MainWindow::on_DownloadButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);

    this->ExitAnim();
}





void MainWindow::on_SetMainFolderButton_clicked()
{
    downloader->setMainFolder(this);
}

