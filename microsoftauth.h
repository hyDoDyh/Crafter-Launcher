#ifndef MICROSOFTAUTH_H
#define MICROSOFTAUTH_H

#include <QObject>
#include <QJsonObject>
#include "playerinfo.h"

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;
class QUrl;
class QNetworkRequest;

/**
 * 微软正版登录（设备代码流程）
 *
 * 认证链：
 *   Microsoft OAuth2 设备代码 -> Xbox Live -> XSTS -> Minecraft Services -> 玩家档案
 *
 * 登录成功后通过 loginSucceeded(PlayerInfo) 返回可直接用于启动游戏的账号信息。
 * 刷新令牌保存在 QSettings 的 Auth/Microsoft/RefreshToken 中，下次登录可静默刷新。
 */
class MicrosoftAuth : public QObject
{
    Q_OBJECT
public:
    explicit MicrosoftAuth(QObject *parent = nullptr);
    ~MicrosoftAuth() override;

    static QString clientId();
    static QString savedRefreshToken();
    static QString savedAccountName();
    static void forgetSavedAccount();

    /// 有保存的刷新令牌则静默登录，否则开始设备代码流程。
    void login();
    /// 无条件开始设备代码流程。
    void startDeviceCodeFlow();
    /// 使用刷新令牌登录。
    void loginWithRefreshToken(const QString &refreshToken);
    /// 中止当前登录（不会发出 loginFailed）。
    void cancel();

    bool isRunning() const;

signals:
    void statusChanged(const QString &status);
    void deviceCodeReady(const QString &userCode,
                         const QString &verificationUri,
                         const QString &message);
    void loginSucceeded(const PlayerInfo &player);
    void loginFailed(const QString &error);

private:
    enum class Stage {
        Idle,
        DeviceCode,
        PollToken,
        RefreshToken,
        XboxLive,
        Xsts,
        Minecraft,
        Profile
    };

    void pollToken();
    void requestXboxLive();
    void requestXsts();
    void requestMinecraftLogin();
    void requestProfile();

    void beginReply(QNetworkReply *reply);
    void handleReply(QNetworkReply *reply);
    void handleDeviceCode(const QJsonObject &obj);
    void handleToken(const QJsonObject &obj, Stage stage);
    void handleXboxLive(const QJsonObject &obj);
    void handleXsts(const QJsonObject &obj);
    void handleMinecraftLogin(const QJsonObject &obj);
    void handleProfile(const QJsonObject &obj);

    void fail(const QString &error);
    void setStatus(const QString &status);
    void reset();

    static QNetworkRequest jsonRequest(const QUrl &url);
    static QNetworkRequest formRequest(const QUrl &url);
    static QString errorDescription(const QJsonObject &obj);
    static QString formatUuid(const QString &rawUuid);

    QNetworkAccessManager *m_net = nullptr;
    QNetworkReply *m_reply = nullptr;
    QTimer *m_pollTimer = nullptr;

    Stage m_stage = Stage::Idle;
    QString m_deviceCode;
    QString m_msAccessToken;
    QString m_refreshToken;
    QString m_xblToken;
    QString m_xstsToken;
    QString m_uhs;
    QString m_xuid;
    QString m_mcAccessToken;
    int m_intervalSec = 5;
};

#endif // MICROSOFTAUTH_H
