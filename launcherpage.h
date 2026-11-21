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

private:
    void connectOutput();
    void disConnectOutput();
    void clearLog();
    void LoadConfig(QString COnfig);
    void pushLog(QString log);
    void applyPlayer(const PlayerInfo &player);
    void updateAccountLabel();
    Launcher *launcher;
    Ui::LauncherPage *ui;
    bool selected = false;
    bool selectedConfig = false;
    launchArguments arg;
    QString now;
    Download* downloader = Download::getInstance();
    Logger *logger = Logger::getInstance();

    PlayerInfo m_signedInPlayer;
    bool m_hasSignedInPlayer = false;
};

#endif // LAUNCHERPAGE_H
