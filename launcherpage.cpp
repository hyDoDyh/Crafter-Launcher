#include "launcherpage.h"
#include "ui_launcherpage.h"
#include <QVBoxLayout>
#include <QSettings>
#include <QSignalBlocker>
#include <QFontMetrics>
#include <QMessageBox>
#include "minecraft.h"
#include "logindialog.h"
#include "microsoftauth.h"

LauncherPage::LauncherPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LauncherPage)
{
    ui->setupUi(this);
    launcher = Launcher::getInstance();
    ui->SaveConfigButton->hide();
    ui->MicrosoftLoginButton->hide();

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
    if (m_launching)
        return;

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
    if (launchArg.argName.isEmpty())
        launchArg.argName = selectedConfig ? m_currentConfig : QStringLiteral("new Config");

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

    if (!launchArg.player.isOffline) {
        // 正版配置：启动前先用保存的刷新令牌换一个新的 Minecraft 访问令牌
        const QString refreshToken = MicrosoftAuth::savedRefreshToken();
        if (refreshToken.isEmpty()) {
            QMessageBox::warning(this, tr("正版登录"),
                                 tr("该配置使用正版登录，但本机还没有保存的微软账号。\n"
                                    "请先选择“正版”，然后点击“微软正版登录”完成登录。"));
            return;
        }

        m_launching = true;
        m_pendingLaunch = launchArg;
        m_pendingVersion = now;
        ui->LaunchButton->setEnabled(false);
        ui->AccountStateLabel->setText(tr("正在验证正版账号…"));

        if (!m_auth) {
            m_auth = new MicrosoftAuth(this);
            connect(m_auth, &MicrosoftAuth::loginSucceeded,
                    this, &LauncherPage::onAuthSucceeded);
            connect(m_auth, &MicrosoftAuth::loginFailed,
                    this, &LauncherPage::onAuthFailed);
            connect(m_auth, &MicrosoftAuth::statusChanged, this, [this](const QString &status) {
                if (m_launching)
                    ui->AccountStateLabel->setText(status);
            });
        }
        m_auth->loginWithRefreshToken(refreshToken);
        return;
    }

    startLaunch(launchArg);
}

void LauncherPage::startLaunch(launchArguments launchArg)
{
    if (launchArg.player.isOffline && launchArg.player.Name.isEmpty()) {
        PlayerInfo player;
        player.isOffline = true;
        player.Name = "TEST_PLAYER";
        player.UUID = "0";
        player.XUID = "0";
        player.AuthAccessToken = "0";
        player.ClientID = "0";
        launchArg.player = player;
    }

    if (launchArg.player.isOffline)
        logger->info("使用离线账号启动：" + launchArg.player.Name);
    else
        logger->info("使用正版账号启动：" + launchArg.player.Name);

    connectOutput();

    launcher->launch(now, launchArg);
}

void LauncherPage::onAuthSucceeded(const PlayerInfo &player)
{
    if (!m_launching) {
        applyPlayer(player);
        return;
    }

    m_launching = false;
    ui->LaunchButton->setEnabled(true);

    launchArguments launchArg = m_pendingLaunch;
    launchArg.player = player;
    if (launchArg.argName.isEmpty())
        launchArg.argName = m_currentConfig;

    launcher->SaveConfig(m_pendingVersion.isEmpty() ? now : m_pendingVersion, launchArg);

    if (selectedConfig && launchArg.argName == m_currentConfig) {
        arg.player = player;
        syncPlayerEdits(player);
        updateModeUi();
    }

    logger->info(QStringLiteral("正版账号验证成功：") + player.Name);
    updateAccountLabel();
    startLaunch(launchArg);
}

void LauncherPage::onAuthFailed(const QString &error)
{
    m_launching = false;
    ui->LaunchButton->setEnabled(true);
    updateAccountLabel();
    logger->error(QStringLiteral("正版账号验证失败：") + error);
    QMessageBox::warning(this, tr("正版登录"), error);
}

void LauncherPage::clearLog(){
    ui->MCOutput->clear();
}
void LauncherPage::LoadConfig(QString Config){
    arg = launcher->LoadConfig(now,Config);
    if (arg.argName.isEmpty())
        arg.argName = Config;
    m_currentConfig = Config;

    syncPlayerEdits(arg.player);

    QVBoxLayout *layout = ui->ConfigLayout;
    for (int i = 0; i < layout->count(); ++i) {
        QLayoutItem *item = layout->itemAt(i);
        if (QWidget *w = item->widget()) {
            w->show();
        }
    }

    updateModeUi();
    updateAccountLabel();
}

void LauncherPage::updateModeUi()
{
    const bool offline = arg.player.isOffline;

    {
        const QSignalBlocker blockOffline(ui->OfflineRadio);
        const QSignalBlocker blockOnline(ui->OnlineRadio);
        ui->OfflineRadio->setChecked(offline);
        ui->OnlineRadio->setChecked(!offline);
    }

    const QList<QWidget *> offlineWidgets = {
        ui->PlayerLabel,
        ui->PlayerNameLabel, ui->PlayerNameEdit,
        ui->PlayerUUIDLabel, ui->PlayerUUIDEdit,
        ui->PlayerXUIDLabel, ui->PlayerXUIDEdit,
        ui->PlayerAuthAccessTokenLabel, ui->PlayerAuthTokenEdit,
        ui->PlayerClientIDLabel, ui->PlayerClientIDEdit,
    };
    for (QWidget *w : offlineWidgets)
        w->setVisible(offline);

    ui->OnlineAccountLabel->setVisible(!offline);
    ui->MicrosoftLoginButton->setVisible(!offline && selectedConfig);
    ui->SaveConfigButton->setVisible(offline && selectedConfig);

    if (!offline) {
        const QString name = arg.player.Name.isEmpty() ? tr("未登录") : arg.player.Name;
        QString text = tr("正版账号：%1").arg(name);
        text += tr("\n（由正版登录自动填写，不可修改）");
        ui->OnlineAccountLabel->setText(text);
        ui->OnlineAccountLabel->setToolTip(arg.player.UUID.isEmpty()
                                               ? QString()
                                               : tr("UUID：%1").arg(arg.player.UUID));
        ui->OnlineAccountLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        const QFontMetrics fm(ui->OnlineAccountLabel->font());
        ui->OnlineAccountLabel->setMinimumHeight(fm.lineSpacing() * 3 + 8);
    } else {
        ui->OnlineAccountLabel->setMinimumHeight(0);
    }
}

void LauncherPage::updateAccountLabel()
{
    if (!selectedConfig) {
        ui->AccountStateLabel->setText(tr("当前配置：未选择"));
    } else if (arg.player.isOffline) {
        ui->AccountStateLabel->setText(tr("当前配置：离线登录"));
    } else if (arg.player.Name.isEmpty()) {
        ui->AccountStateLabel->setText(tr("当前配置：正版登录（未登录）"));
    } else {
        ui->AccountStateLabel->setText(tr("当前配置：正版登录（%1）").arg(arg.player.Name));
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
    arg = launchArguments();
    m_currentConfig.clear();

    QVBoxLayout *configLayout = ui->ConfigLayout;
    for (int i = 0; i < configLayout->count(); ++i) {
        QLayoutItem *item = configLayout->itemAt(i);
        if (QWidget *w = item->widget())
            w->hide();
    }
    ui->SaveConfigButton->hide();
    ui->MicrosoftLoginButton->hide();
    updateAccountLabel();

    ui->LaunchArgsList->setWidget(contentWidget);
    for(launchArguments arg : Args){
        QPushButton *btn = new QPushButton(arg.argName);
        layout->addWidget(btn);
        connect(btn,&QPushButton::clicked,this,[=](){
            QPushButton *btn = qobject_cast<QPushButton*>(sender());
            selectedConfig = true;
            LoadConfig(btn->text());
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
    if (!selectedConfig || !arg.player.isOffline)
        return;

    arg.player.UUID = ui->PlayerUUIDEdit->text();
    arg.player.XUID = ui->PlayerXUIDEdit->text();
    arg.player.AuthAccessToken = ui->PlayerAuthTokenEdit->text();
    arg.player.Name = ui->PlayerNameEdit->text();
    arg.player.ClientID = ui->PlayerClientIDEdit->text();
    arg.player.isOffline = true;
    launcher->SaveConfig(now,arg);
    updateAccountLabel();
}

void LauncherPage::on_MicrosoftLoginButton_clicked()
{
    if (!selectedConfig) {
        QMessageBox::information(this, tr("正版登录"),
                                 tr("请先在左侧选择一个启动配置，再选择“正版”登录方式。"));
        return;
    }
    LoginDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
        applyPlayer(dialog.player());
}

void LauncherPage::on_OfflineRadio_toggled(bool checked)
{
    if (!checked || m_settingMode)
        return;
    if (!selectedConfig) {
        updateModeUi();
        return;
    }

    arg.player.isOffline = true;
    launcher->SaveConfig(now, arg);
    updateModeUi();
    updateAccountLabel();
}

void LauncherPage::on_OnlineRadio_toggled(bool checked)
{
    if (!checked || m_settingMode)
        return;
    if (!selectedConfig) {
        updateModeUi();
        return;
    }
    if (!arg.player.isOffline) {
        updateModeUi();
        return;
    }

    LoginDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        applyPlayer(dialog.player());
    } else {
        // 用户取消登录：保持离线模式
        updateModeUi();
        updateAccountLabel();
    }
}

void LauncherPage::syncPlayerEdits(const PlayerInfo &player)
{
    const QList<QLineEdit *> edits = {
        ui->PlayerNameEdit, ui->PlayerUUIDEdit, ui->PlayerXUIDEdit,
        ui->PlayerAuthTokenEdit, ui->PlayerClientIDEdit,
    };
    const QStringList values = {
        player.Name, player.UUID, player.XUID,
        player.AuthAccessToken, player.ClientID,
    };
    for (int i = 0; i < edits.size() && i < values.size(); ++i) {
        edits[i]->setText(values[i]);
        edits[i]->setCursorPosition(0);
    }
}

void LauncherPage::applyPlayer(const PlayerInfo &player)
{
    arg.player = player;
    arg.player.isOffline = false;
    if (arg.argName.isEmpty())
        arg.argName = m_currentConfig;

    // 同步一份到离线输入框，切换回离线时仍可看到并修改
    syncPlayerEdits(player);

    if (selectedConfig) {
        launcher->SaveConfig(now, arg);
    }

    updateModeUi();
    updateAccountLabel();
    logger->info(QStringLiteral("正版登录成功：") + player.Name + "（UUID " + player.UUID + "）");
}
