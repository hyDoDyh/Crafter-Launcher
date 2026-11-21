#include "microsoftauth.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QSslSocket>
#include <QTimer>
#include <QUrlQuery>

namespace {

QString clientIdString()
{
    // 默认使用 Crafter 自己的应用；也允许通过环境变量或
    // Crafter.ini 的 [Auth/Microsoft] ClientId 换成已获授权的应用。
    const QByteArray fromEnv = qgetenv("CRAFTER_MS_CLIENT_ID");
    if (!fromEnv.isEmpty())
        return QString::fromUtf8(fromEnv).trimmed();

    const QString fromSettings = QSettings()
                                     .value(QStringLiteral("Auth/Microsoft/ClientId"))
                                     .toString()
                                     .trimmed();
    if (!fromSettings.isEmpty())
        return fromSettings;

    return QStringLiteral("415ea03b-1e6d-4a79-8112-453391a4e00a");
}

QString scopeString()
{
    return QStringLiteral("XboxLive.signin offline_access");
}

QUrl deviceCodeUrl()
{
    return QUrl(QStringLiteral("https://login.microsoftonline.com/consumers/oauth2/v2.0/devicecode"));
}

QUrl tokenUrl()
{
    return QUrl(QStringLiteral("https://login.microsoftonline.com/consumers/oauth2/v2.0/token"));
}

QUrl xboxLiveUrl()
{
    return QUrl(QStringLiteral("https://user.auth.xboxlive.com/user/authenticate"));
}

QUrl xstsUrl()
{
    return QUrl(QStringLiteral("https://xsts.auth.xboxlive.com/xsts/authorize"));
}

QUrl minecraftLoginUrl()
{
    return QUrl(QStringLiteral("https://api.minecraftservices.com/authentication/login_with_xbox"));
}

QUrl minecraftProfileUrl()
{
    return QUrl(QStringLiteral("https://api.minecraftservices.com/minecraft/profile"));
}

QString refreshTokenKey()
{
    return QStringLiteral("Auth/Microsoft/RefreshToken");
}

QString accountNameKey()
{
    return QStringLiteral("Auth/Microsoft/LastName");
}

} // namespace

MicrosoftAuth::MicrosoftAuth(QObject *parent)
    : QObject(parent)
    , m_net(new QNetworkAccessManager(this))
    , m_pollTimer(new QTimer(this))
{
    m_pollTimer->setSingleShot(true);
    connect(m_pollTimer, &QTimer::timeout, this, [this]() { pollToken(); });
}

MicrosoftAuth::~MicrosoftAuth()
{
    cancel();
}

QString MicrosoftAuth::clientId()
{
    return clientIdString();
}

QString MicrosoftAuth::savedRefreshToken()
{
    return QSettings().value(refreshTokenKey()).toString();
}

QString MicrosoftAuth::savedAccountName()
{
    return QSettings().value(accountNameKey()).toString();
}

void MicrosoftAuth::forgetSavedAccount()
{
    QSettings settings;
    settings.remove(refreshTokenKey());
    settings.remove(accountNameKey());
    settings.sync();
}

bool MicrosoftAuth::isRunning() const
{
    return m_stage != Stage::Idle;
}

void MicrosoftAuth::login()
{
    const QString refreshToken = savedRefreshToken();
    if (!refreshToken.isEmpty()) {
        setStatus(QStringLiteral("正在使用上次的微软账号登录…"));
        loginWithRefreshToken(refreshToken);
    } else {
        startDeviceCodeFlow();
    }
}

void MicrosoftAuth::startDeviceCodeFlow()
{
    reset();

    if (!QSslSocket::supportsSsl()) {
        emit loginFailed(QStringLiteral(
            "当前 Qt 运行环境缺少 OpenSSL，无法进行 HTTPS 登录。\n"
            "请把 libssl-3-x64.dll 和 libcrypto-3-x64.dll 放到程序目录后重试。"));
        return;
    }

    m_stage = Stage::DeviceCode;
    setStatus(QStringLiteral("正在向微软申请设备登录代码…"));

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("client_id"), clientId());
    query.addQueryItem(QStringLiteral("scope"), scopeString());

    QNetworkReply *reply = m_net->post(formRequest(deviceCodeUrl()),
                                       query.toString(QUrl::FullyEncoded).toUtf8());
    beginReply(reply);
}

void MicrosoftAuth::loginWithRefreshToken(const QString &refreshToken)
{
    reset();

    if (refreshToken.isEmpty()) {
        emit loginFailed(QStringLiteral("没有可用的刷新令牌，请重新登录。"));
        return;
    }
    if (!QSslSocket::supportsSsl()) {
        emit loginFailed(QStringLiteral(
            "当前 Qt 运行环境缺少 OpenSSL，无法进行 HTTPS 登录。\n"
            "请把 libssl-3-x64.dll 和 libcrypto-3-x64.dll 放到程序目录后重试。"));
        return;
    }

    m_stage = Stage::RefreshToken;

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("client_id"), clientId());
    query.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("refresh_token"));
    query.addQueryItem(QStringLiteral("refresh_token"), refreshToken);
    query.addQueryItem(QStringLiteral("scope"), scopeString());

    QNetworkReply *reply = m_net->post(formRequest(tokenUrl()),
                                       query.toString(QUrl::FullyEncoded).toUtf8());
    beginReply(reply);
}

void MicrosoftAuth::cancel()
{
    reset();
}

void MicrosoftAuth::reset()
{
    m_stage = Stage::Idle;
    if (m_pollTimer)
        m_pollTimer->stop();
    if (m_reply) {
        QNetworkReply *reply = m_reply;
        m_reply = nullptr;
        reply->abort();
        reply->deleteLater();
    }
}

void MicrosoftAuth::pollToken()
{
    if (m_stage != Stage::PollToken || m_reply)
        return;

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("client_id"), clientId());
    query.addQueryItem(QStringLiteral("grant_type"),
                       QStringLiteral("urn:ietf:params:oauth:grant-type:device_code"));
    query.addQueryItem(QStringLiteral("device_code"), m_deviceCode);

    QNetworkReply *reply = m_net->post(formRequest(tokenUrl()),
                                       query.toString(QUrl::FullyEncoded).toUtf8());
    beginReply(reply);
}

void MicrosoftAuth::requestXboxLive()
{
    m_stage = Stage::XboxLive;
    setStatus(QStringLiteral("微软账号登录成功，正在验证 Xbox Live 账户…"));

    QJsonObject properties;
    properties.insert(QStringLiteral("AuthMethod"), QStringLiteral("RPS"));
    properties.insert(QStringLiteral("SiteName"), QStringLiteral("user.auth.xboxlive.com"));
    properties.insert(QStringLiteral("RpsTicket"), QStringLiteral("d=") + m_msAccessToken);

    QJsonObject body;
    body.insert(QStringLiteral("Properties"), properties);
    body.insert(QStringLiteral("RelyingParty"), QStringLiteral("http://auth.xboxlive.com"));
    body.insert(QStringLiteral("TokenType"), QStringLiteral("JWT"));

    QNetworkRequest request = jsonRequest(xboxLiveUrl());
    request.setRawHeader("x-xbl-contract-version", "1");
    QNetworkReply *reply = m_net->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    beginReply(reply);
}

void MicrosoftAuth::requestXsts()
{
    m_stage = Stage::Xsts;
    setStatus(QStringLiteral("正在向 Xbox 安全服务申请令牌…"));

    QJsonArray userTokens;
    userTokens.append(m_xblToken);

    QJsonObject properties;
    properties.insert(QStringLiteral("SandboxId"), QStringLiteral("RETAIL"));
    properties.insert(QStringLiteral("UserTokens"), userTokens);

    QJsonObject body;
    body.insert(QStringLiteral("Properties"), properties);
    body.insert(QStringLiteral("RelyingParty"),
                QStringLiteral("rp://api.minecraftservices.com/"));
    body.insert(QStringLiteral("TokenType"), QStringLiteral("JWT"));

    QNetworkRequest request = jsonRequest(xstsUrl());
    request.setRawHeader("x-xbl-contract-version", "1");
    QNetworkReply *reply = m_net->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    beginReply(reply);
}

void MicrosoftAuth::requestMinecraftLogin()
{
    m_stage = Stage::Minecraft;
    setStatus(QStringLiteral("正在登录 Minecraft 服务…"));

    QJsonObject body;
    body.insert(QStringLiteral("identityToken"),
                QStringLiteral("XBL3.0 x=") + m_uhs + QStringLiteral(";") + m_xstsToken);

    QNetworkReply *reply = m_net->post(jsonRequest(minecraftLoginUrl()),
                                       QJsonDocument(body).toJson(QJsonDocument::Compact));
    beginReply(reply);
}

void MicrosoftAuth::requestProfile()
{
    m_stage = Stage::Profile;
    setStatus(QStringLiteral("正在获取 Minecraft 玩家档案…"));

    QNetworkRequest request = jsonRequest(minecraftProfileUrl());
    request.setRawHeader("Authorization", "Bearer " + m_mcAccessToken.toUtf8());
    beginReply(m_net->get(request));
}

void MicrosoftAuth::beginReply(QNetworkReply *reply)
{
    m_reply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleReply(reply); });
}

void MicrosoftAuth::handleReply(QNetworkReply *reply)
{
    if (reply != m_reply) {
        reply->deleteLater();
        return;
    }
    m_reply = nullptr;
    reply->deleteLater();

    const Stage stage = m_stage;
    if (stage == Stage::Idle)
        return;

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray data = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    const QJsonObject obj = doc.object();

    if (!doc.isObject()) {
        if (reply->error() == QNetworkReply::OperationCanceledError)
            return;
        QString detail = reply->errorString();
        if (status > 0)
            detail += QStringLiteral("（HTTP %1）").arg(status);
        fail(QStringLiteral("网络请求失败：") + detail);
        return;
    }

    switch (stage) {
    case Stage::DeviceCode:
        handleDeviceCode(obj);
        break;
    case Stage::PollToken:
    case Stage::RefreshToken:
        handleToken(obj, stage);
        break;
    case Stage::XboxLive:
        handleXboxLive(obj);
        break;
    case Stage::Xsts:
        handleXsts(obj);
        break;
    case Stage::Minecraft:
        handleMinecraftLogin(obj);
        break;
    case Stage::Profile:
        handleProfile(obj);
        break;
    case Stage::Idle:
        break;
    }
}

void MicrosoftAuth::handleDeviceCode(const QJsonObject &obj)
{
    if (!obj.value(QStringLiteral("error")).toString().isEmpty()) {
        fail(QStringLiteral("获取登录代码失败：") + errorDescription(obj));
        return;
    }

    m_deviceCode = obj.value(QStringLiteral("device_code")).toString();
    const QString userCode = obj.value(QStringLiteral("user_code")).toString();
    QString verificationUri = obj.value(QStringLiteral("verification_uri")).toString();
    if (verificationUri.isEmpty())
        verificationUri = QStringLiteral("https://microsoft.com/link");

    if (m_deviceCode.isEmpty() || userCode.isEmpty()) {
        fail(QStringLiteral("微软返回的设备代码数据不完整，请重试。"));
        return;
    }

    m_intervalSec = qMax(1, obj.value(QStringLiteral("interval")).toInt(5));
    m_stage = Stage::PollToken;
    m_pollTimer->start(m_intervalSec * 1000);

    emit deviceCodeReady(userCode, verificationUri,
                         obj.value(QStringLiteral("message")).toString());
    setStatus(QStringLiteral("等待你在浏览器中完成登录…"));
}

void MicrosoftAuth::handleToken(const QJsonObject &obj, Stage stage)
{
    const QString accessToken = obj.value(QStringLiteral("access_token")).toString();
    if (!accessToken.isEmpty()) {
        m_msAccessToken = accessToken;

        const QString refreshToken = obj.value(QStringLiteral("refresh_token")).toString();
        if (!refreshToken.isEmpty()) {
            m_refreshToken = refreshToken;
            QSettings().setValue(refreshTokenKey(), refreshToken);
        }

        requestXboxLive();
        return;
    }

    const QString error = obj.value(QStringLiteral("error")).toString();

    if (stage == Stage::RefreshToken) {
        // 刷新失败（过期/被撤销），清掉旧令牌，下次走完整设备代码流程。
        if (error == QStringLiteral("invalid_grant")
            || error == QStringLiteral("interaction_required")) {
            forgetSavedAccount();
            fail(QStringLiteral("上次的微软登录已失效，需要重新登录。"));
        } else {
            fail(QStringLiteral("微软账号刷新失败：") + errorDescription(obj));
        }
        return;
    }

    // 设备代码轮询
    if (error == QStringLiteral("authorization_pending")) {
        m_pollTimer->start(m_intervalSec * 1000);
        return;
    }
    if (error == QStringLiteral("slow_down")) {
        m_intervalSec += 5;
        m_pollTimer->start(m_intervalSec * 1000);
        return;
    }
    if (error == QStringLiteral("expired_token")) {
        fail(QStringLiteral("登录代码已过期，请重新获取。"));
        return;
    }
    if (error == QStringLiteral("authorization_declined")) {
        fail(QStringLiteral("你在浏览器中拒绝了本次登录。"));
        return;
    }
    if (error == QStringLiteral("bad_verification_code")) {
        fail(QStringLiteral("登录代码无效，请重新获取。"));
        return;
    }

    fail(QStringLiteral("微软账号登录失败：") + errorDescription(obj));
}

void MicrosoftAuth::handleXboxLive(const QJsonObject &obj)
{
    const QString token = obj.value(QStringLiteral("Token")).toString();
    if (token.isEmpty()) {
        fail(QStringLiteral("Xbox Live 验证失败：") + errorDescription(obj));
        return;
    }

    m_xblToken = token;
    requestXsts();
}

void MicrosoftAuth::handleXsts(const QJsonObject &obj)
{
    const QString token = obj.value(QStringLiteral("Token")).toString();
    if (token.isEmpty()) {
        const qint64 xerr = obj.value(QStringLiteral("XErr")).toVariant().toLongLong();
        QString message;
        switch (xerr) {
        case 2148916233LL:
            message = QStringLiteral(
                "该微软账号还没有 Xbox 账户。请先访问 https://www.xbox.com 登录一次并创建档案，然后重试。");
            break;
        case 2148916235LL:
            message = QStringLiteral("Xbox Live 在你所在的地区不可用。");
            break;
        case 2148916236LL:
        case 2148916237LL:
            message = QStringLiteral("该账号需要先在 Xbox 网站完成成年人身份验证。");
            break;
        case 2148916238LL:
            message = QStringLiteral("这是未成年账号，需要先加入 Microsoft 家庭组后才能登录。");
            break;
        case 2148916227LL:
            message = QStringLiteral("该账号已被 Xbox Live 封禁。");
            break;
        default:
            message = QStringLiteral("Xbox 安全令牌验证失败：") + errorDescription(obj);
            break;
        }
        fail(message);
        return;
    }

    const QJsonArray xui = obj.value(QStringLiteral("DisplayClaims")).toObject()
                               .value(QStringLiteral("xui")).toArray();
    if (!xui.isEmpty()) {
        const QJsonObject claim = xui.at(0).toObject();
        m_uhs = claim.value(QStringLiteral("uhs")).toString();
        m_xuid = claim.value(QStringLiteral("xid")).toString();
    }

    if (m_uhs.isEmpty()) {
        fail(QStringLiteral("Xbox 返回的账户信息不完整，请重试。"));
        return;
    }

    m_xstsToken = token;
    requestMinecraftLogin();
}

void MicrosoftAuth::handleMinecraftLogin(const QJsonObject &obj)
{
    const QString token = obj.value(QStringLiteral("access_token")).toString();
    if (token.isEmpty()) {
        const QString detail = errorDescription(obj);
        if (detail.contains(QStringLiteral("app registration"), Qt::CaseInsensitive)
            || detail.contains(QStringLiteral("AppRegInfo"), Qt::CaseInsensitive)) {
            fail(QStringLiteral(
                     "Minecraft 服务拒绝了当前应用（Client ID: %1）。\n"
                     "微软账号、Xbox Live、XSTS 认证均已通过，但该 Client ID 还没有获得 "
                     "Minecraft 服务的授权。\n"
                     "请用这个 Client ID 到 https://aka.ms/AppRegInfo 提交申请，审批通过后即可登录。")
                     .arg(clientId()));
            return;
        }
        fail(QStringLiteral("Minecraft 登录失败：") + detail);
        return;
    }

    m_mcAccessToken = token;
    requestProfile();
}

void MicrosoftAuth::handleProfile(const QJsonObject &obj)
{
    const QString name = obj.value(QStringLiteral("name")).toString();
    const QString id = obj.value(QStringLiteral("id")).toString();

    if (name.isEmpty() || id.isEmpty()) {
        fail(QStringLiteral(
            "该微软账号没有可用的 Minecraft: Java Edition 档案。\n"
            "请确认已购买游戏并创建过角色名。\n\n") + errorDescription(obj));
        return;
    }

    PlayerInfo player;
    player.Name = name;
    player.UUID = formatUuid(id);
    player.XUID = m_xuid;
    player.AuthAccessToken = m_mcAccessToken;
    player.ClientID = clientId();
    player.isOffline = false;

    QSettings().setValue(accountNameKey(), name);

    m_stage = Stage::Idle;
    setStatus(QStringLiteral("登录成功，欢迎你，") + name + QStringLiteral("！"));
    emit loginSucceeded(player);
}

void MicrosoftAuth::fail(const QString &error)
{
    if (m_stage == Stage::Idle)
        return;

    reset();
    emit loginFailed(error);
}

void MicrosoftAuth::setStatus(const QString &status)
{
    emit statusChanged(status);
}

QNetworkRequest MicrosoftAuth::jsonRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("CrafterLauncher/1.0.0"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    return request;
}

QNetworkRequest MicrosoftAuth::formRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded"));
    request.setRawHeader("Accept", "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("CrafterLauncher/1.0.0"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    return request;
}

QString MicrosoftAuth::errorDescription(const QJsonObject &obj)
{
    QString detail = obj.value(QStringLiteral("error_description")).toString();
    if (detail.isEmpty())
        detail = obj.value(QStringLiteral("errorMessage")).toString();
    if (detail.isEmpty())
        detail = obj.value(QStringLiteral("error")).toString();
    if (detail.isEmpty())
        detail = obj.value(QStringLiteral("Message")).toString();
    if (detail.isEmpty())
        detail = QStringLiteral("未知错误");
    return detail;
}

QString MicrosoftAuth::formatUuid(const QString &rawUuid)
{
    if (rawUuid.size() != 32)
        return rawUuid;

    return rawUuid.mid(0, 8) + QLatin1Char('-')
           + rawUuid.mid(8, 4) + QLatin1Char('-')
           + rawUuid.mid(12, 4) + QLatin1Char('-')
           + rawUuid.mid(16, 4) + QLatin1Char('-')
           + rawUuid.mid(20);
}
