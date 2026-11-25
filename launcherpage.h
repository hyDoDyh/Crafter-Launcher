#ifndef LAUNCHERPAGE_H
#define LAUNCHERPAGE_H

#include <QWidget>
#include "logger.h"
#include "minecraftversion.h"
#include "download.h"
#include "launcher.h"
#include "playerinfo.h"

namespace Ui {
class LauncherPage;
}

class MicrosoftAuth;

class LauncherPage : public QWidget
{
    Q_OBJECT

public:
    explicit LauncherPage(QWidget *parent = nullptr);
    ~LauncherPage();

private slots:
    void on_RefreshButton_clicked();

    void on_LaunchButton_clicked();

    void on_RefreshLaunchArgsListBtn_clicked();

    void on_CreateConfigButton_clicked();

    void on_SaveConfigButton_clicked();

    void on_MicrosoftLoginButton_clicked();

    void on_OfflineRadio_toggled(bool checked);

    void on_OnlineRadio_toggled(bool checked);

    void onAuthSucceeded(const PlayerInfo &player);

    void onAuthFailed(const QString &error);

private:
    void connectOutput();
    void disConnectOutput();
    void clearLog();
    void LoadConfig(QString Config);
    void pushLog(QString log);
    void applyPlayer(const PlayerInfo &player);
    void syncPlayerEdits(const PlayerInfo &player);
    void updateAccountLabel();
    void updateModeUi();
    void startLaunch(launchArguments launchArg);

    Launcher *launcher;
    Ui::LauncherPage *ui;
    bool selected = false;
    bool selectedConfig = false;
    launchArguments arg;
    QString now;
    QString m_currentConfig;
    Download* downloader = Download::getInstance();
    Logger *logger = Logger::getInstance();

    MicrosoftAuth *m_auth = nullptr;
    bool m_settingMode = false;
    bool m_launching = false;
    launchArguments m_pendingLaunch;
    QString m_pendingVersion;
};

#endif // LAUNCHERPAGE_H
