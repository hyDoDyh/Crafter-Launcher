#include "downloadpage.h"
#include "ui_downloadpage.h"
#include "minectaftmodel.h"
#include "minecraft.h"


#include <QVBoxLayout>
DownloadPage::DownloadPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DownloadPage)
{
    ui->setupUi(this);
    qDebug() << "";
    ui->scrollArea->setWidgetResizable(true);
}

DownloadPage::~DownloadPage()
{
    delete ui;
}

void DownloadPage::on_RefreshButton_clicked()
{
    connect(downloader, &Download::versionsReady, this, [=](const QList<MinecraftVersion> &versions) {
        QWidget *contentWidget = new QWidget;

        QVBoxLayout *layout = new QVBoxLayout(contentWidget);


        ui->scrollArea->setWidget(contentWidget);
        for(MinecraftVersion ver : versions){
            Minecraft *mc = new Minecraft(ver.getId(),ver.getType(),ver.getUrl(),ver);
            layout->addWidget(mc);
            connect(mc,&Minecraft::ButtonClicked,this,[=](MinecraftVersion ver){
                ui->NameLabel->setText("版本:"+ver.getId());
                ui->TypeLabel->setText("类型:"+ver.getType());
                selected = true;
                now = ver;
            });
        }

    });
    downloader->getAllVersions();
}


void DownloadPage::on_DownloadButton_clicked()
{
    Download *download = Download::getInstance();
    if(!selected)
        logger->warning("你还没有选择一个版本!!!");
    else
        download->InstallMinecraft(now.getUrl(),now.getId());
}

