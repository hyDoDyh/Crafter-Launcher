#include "launcherpage.h"
#include "ui_launcherpage.h"
#include <QVBoxLayout>
#include <QSettings>
#include "minecraft.h"
#include "logindialog.h"
LauncherPage::LauncherPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LauncherPage)
{
    ui->setupUi(this);
    launcher = Launcher::getInstance();
    ui->SaveConfigButton->hide();
    QVBoxLayout *layout = ui->ConfigLayout;
    for (int i = 0; i < layout->count(); ++i) {
        QLayoutItem *item = layout->itemAt(i);
        if (QWidget *w = item->widget()) {
            w->hide();
        }
    }
    updateAccountLabel();
}

LauncherPage::~LauncherPage()
{
    delete ui;
}

void LauncherPage::on_RefreshButton_clicked()
{
    QList<QString> versions = launcher->getVersions();
    QWidget *contentWidget = new QWidget;

    QVBoxLayout *layout = new QVBoxLayout(contentWidget);


    ui->scrollArea->setWidget(contentWidget);
    for(QString ver : versions){
        QPushButton *btn = new QPushButton(ver);
        layout->addWidget(btn);
        connect(btn,&QPushButton::clicked,this,[=](){
            QPushButton *btn = qobject_cast<QPushButton*>(sender());
            ui->NameLabel->setText("版本:"+btn->text());
            // ui->TypeLabel->setText("类型:"+ver.getType());
            selected = true;
            now = btn->text();
            on_RefreshLaunchArgsListBtn_clicked();

        });
    }


}



void LauncherPage::on_LaunchButton_clicked()
{
    if(!selected){
        logger->warning("你还没有选择一个版本!!!");
        return;
    }

    launchArguments launchArg;
    if (selectedConfig) {
        // 使用用户选中的启动配置
        launchArg = arg;
    } else {
        // 没有选中配置时，优先读取该版本的默认配置，读不到再用内置默认值
        launchArg = launcher->LoadConfig(now, QStringLiteral("new Config"));
        if (launchArg.Path.isEmpty()) {
            launchArg = launchArguments::Defult(now);
            launchArg.argName = QStringLiteral("new Config");
        }
    }

    if (launchArg.Path.isEmpty())
        launchArg.Path = downloader->getMainFolder(NULL) + "/versions/" + now + "/" + now + ".json";
    if (launchArg.version.isEmpty())
        launchArg.version = now;
    if (launchArg.gameDir.isEmpty())
        launchArg.gameDir = downloader->getMainFolder(NULL) + "/versions/" + now;
    if (launchArg.libPath.isEmpty())
        launchArg.libPath = downloader->getMainFolder(NULL) + "/libraries";
    if (launchArg.assetIndex.isEmpty())
        launchArg.assetIndex = "0"; // TODO

    if (!selectedConfig) {
        if (m_hasSignedInPlayer) {
            // 本次运行中登录的正版账号优先，令牌最新
            launchArg.player = m_signedInPlayer;
        } else if (launchArg.player.Name.isEmpty() || launchArg.player.AuthAccessToken.isEmpty()) {
            PlayerInfo player;
            player.isOffline = true;
            player.Name = "TEST_PLAYER";
            player.UUID = "0";
            player.XUID = "0";
            player.AuthAccessToken = "0";
            player.ClientID = "0";
            launchArg.player = player;
        }
    }

    if (launchArg.player.isOffline)
        logger->info("使用离线账号启动：" + launchArg.player.Name);
    else
        logger->info("使用正版账号启动：" + launchArg.player.Name);

    connectOutput();

    launcher->launch(now,launchArg);
}
void LauncherPage::clearLog(){
    ui->MCOutput->clear();
}
void LauncherPage::LoadConfig(QString Config){
    arg = launcher->LoadConfig(now,Config);
    ui->PlayerNameEdit->setText(arg.player.Name);
    ui->PlayerUUIDEdit->setText(arg.player.UUID);
    ui->PlayerXUIDEdit->setText(arg.player.XUID);
    ui->PlayerAuthTokenEdit->setText(arg.player.AuthAccessToken);
    ui->PlayerClientIDEdit->setText(arg.player.ClientID);
    ui->SaveConfigButton->show();
    QVBoxLayout *layout = ui->ConfigLayout;
    for (int i = 0; i < layout->count(); ++i) {
        QLayoutItem *item = layout->itemAt(i);
        if (QWidget *w = item->widget()) {
            w->show();
        }
    }

}

void LauncherPage::pushLog(QString log){
    QString all = ui->MCOutput->toPlainText() + '\n' + log;
    ui->MCOutput->setPlainText(all.right(200));
}
void LauncherPage::disConnectOutput(){
    disconnect(launcher, nullptr, this, nullptr);
}
void LauncherPage::connectOutput(){
    connect(launcher,&Launcher::onStart,this,[=](QString log){
        pushLog("[启动器]:开始启动");
    });
    connect(launcher,&Launcher::onInfo,this,[=](QString log){
        pushLog(log);
    });
    connect(launcher,&Launcher::onError,this,[=](QString log){
        pushLog(log);
    });
    connect(launcher,&Launcher::onFail,this,[=](QString log){
        pushLog("[启动器]:启动Java失败,错误:"+log);
    });
    connect(launcher,&Launcher::onExit,this,[=](int exitCode){
        pushLog("[启动器]:游戏退出,退出码"+QString::number(exitCode));
        disConnectOutput();
    });
}


void LauncherPage::on_RefreshLaunchArgsListBtn_clicked()
{
    if(!selected){
        logger->error("请先选择一个版本");
        return;
    }
    QList<launchArguments> Args = launcher->getAllConfiguration(now);
    QWidget *contentWidget = new QWidget;

    QVBoxLayout *layout = new QVBoxLayout(contentWidget);
    selectedConfig = false;

    ui->LaunchArgsList->setWidget(contentWidget);
    for(launchArguments arg : Args){
        QPushButton *btn = new QPushButton(arg.argName);
        layout->addWidget(btn);
        connect(btn,&QPushButton::clicked,this,[=](){
            QPushButton *btn = qobject_cast<QPushButton*>(sender());
            LoadConfig(btn->text());
            selectedConfig = true;
        });
    }

}


void LauncherPage::on_CreateConfigButton_clicked()
{
    if(!selected){
        logger->error("请先选择一个版本");
        return;
    }

    auto cfgs = QStringLiteral("Launcher/Configuration/") + now + "/ConfigurationList";
    QSettings settings;
    settings.setValue(cfgs,QStringList("new Config"));
    on_RefreshLaunchArgsListBtn_clicked();

}


void LauncherPage::on_SaveConfigButton_clicked()
{
    arg.player.UUID = ui->PlayerUUIDEdit->text();
    arg.player.XUID = ui->PlayerXUIDEdit->text();
    arg.player.AuthAccessToken = ui->PlayerAuthTokenEdit->text();
    arg.player.Name = ui->PlayerNameEdit->text();
    arg.player.ClientID = ui->PlayerClientIDEdit->text();
    launcher->SaveConfig(now,arg);
}

void LauncherPage::on_MicrosoftLoginButton_clicked()
{
    LoginDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
        applyPlayer(dialog.player());
}

void LauncherPage::applyPlayer(const PlayerInfo &player)
{
    m_signedInPlayer = player;
    m_hasSignedInPlayer = !player.isOffline;

    ui->PlayerNameEdit->setText(player.Name);
    ui->PlayerUUIDEdit->setText(player.UUID);
    ui->PlayerXUIDEdit->setText(player.XUID);
    ui->PlayerAuthTokenEdit->setText(player.AuthAccessToken);
    ui->PlayerClientIDEdit->setText(player.ClientID);

    // 有选中的版本时，把账号写进配置，下次启动可以直接用
    if (selected) {
        launchArguments cfg;
        if (selectedConfig) {
            cfg = arg;
        } else {
            cfg = launcher->LoadConfig(now, QStringLiteral("new Config"));
            if (cfg.Path.isEmpty()) {
                launcher->CreateDefultConfiguration(now, QStringLiteral("new Config"));
                cfg = launcher->LoadConfig(now, QStringLiteral("new Config"));
            }
        }
        cfg.player = player;
        launcher->SaveConfig(now, cfg);
        if (selectedConfig)
            arg.player = player;
    }

    updateAccountLabel();
    logger->info(QStringLiteral("正版登录成功：") + player.Name + "（UUID " + player.UUID + "）");
}

void LauncherPage::updateAccountLabel()
{
    if (m_hasSignedInPlayer)
        ui->AccountStateLabel->setText(tr("当前账号：%1（正版）").arg(m_signedInPlayer.Name));
    else
        ui->AccountStateLabel->setText(tr("当前账号：未登录（离线）"));
}

