#include "logindialog.h"
#include "microsoftauth.h"

#include <QClipboard>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , m_auth(new MicrosoftAuth(this))
{
    setWindowTitle(tr("微软正版登录"));
    setModal(true);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    resize(500, 340);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(10);

    auto *title = new QLabel(tr("登录 Minecraft 正版账号"), this);
    title->setStyleSheet(QStringLiteral("font-size:20px; font-weight:bold;"));
    layout->addWidget(title);

    auto *hint = new QLabel(
        tr("登录使用微软官方设备代码流程：浏览器会自动打开，"
           "把下面的代码粘贴到页面中并完成登录即可。"),
        this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    m_codeLabel = new QLabel(QStringLiteral("------"), this);
    m_codeLabel->setAlignment(Qt::AlignCenter);
    m_codeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_codeLabel->setStyleSheet(QStringLiteral(
        "font-size:30px; font-weight:bold; letter-spacing:6px;"
        "color:#0a7d4f; padding:10px;"));
    layout->addWidget(m_codeLabel);

    m_linkLabel = new QLabel(this);
    m_linkLabel->setAlignment(Qt::AlignCenter);
    m_linkLabel->setTextFormat(Qt::RichText);
    m_linkLabel->setOpenExternalLinks(true);
    m_linkLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    layout->addWidget(m_linkLabel);

    m_statusLabel = new QLabel(tr("正在准备登录…"), this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QStringLiteral("color:#555;"));
    layout->addWidget(m_statusLabel);

    m_progress = new QProgressBar(this);
    m_progress->setRange(0, 0);
    m_progress->setTextVisible(false);
    m_progress->setMaximumHeight(8);
    layout->addWidget(m_progress);

    layout->addStretch(1);

    auto *buttons = new QHBoxLayout;
    m_actionButton = new QPushButton(tr("复制代码并打开浏览器"), this);
    m_actionButton->setEnabled(false);
    m_retryButton = new QPushButton(tr("重试"), this);
    m_retryButton->hide();
    m_cancelButton = new QPushButton(tr("取消"), this);
    buttons->addWidget(m_actionButton);
    buttons->addWidget(m_retryButton);
    buttons->addStretch(1);
    buttons->addWidget(m_cancelButton);
    layout->addLayout(buttons);

    connect(m_actionButton, &QPushButton::clicked, this, &LoginDialog::onCopyAndOpen);
    connect(m_retryButton, &QPushButton::clicked, this, &LoginDialog::onRetry);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    connect(m_auth, &MicrosoftAuth::statusChanged, this, &LoginDialog::onStatusChanged);
    connect(m_auth, &MicrosoftAuth::deviceCodeReady, this, &LoginDialog::onDeviceCode);
    connect(m_auth, &MicrosoftAuth::loginSucceeded, this, &LoginDialog::onLoginSucceeded);
    connect(m_auth, &MicrosoftAuth::loginFailed, this, &LoginDialog::onLoginFailed);

    const QString lastName = MicrosoftAuth::savedAccountName();
    if (!lastName.isEmpty())
        m_statusLabel->setText(tr("上次登录的账号：%1").arg(lastName));

    // 等对话框显示后再发起网络请求，让用户先看到界面。
    QTimer::singleShot(0, this, [this]() {
        if (!MicrosoftAuth::savedRefreshToken().isEmpty()) {
            m_pendingRefresh = true;
            m_auth->login();
        } else {
            m_auth->startDeviceCodeFlow();
        }
    });
}

LoginDialog::~LoginDialog()
{
    m_auth->cancel();
}

void LoginDialog::reject()
{
    m_auth->cancel();
    QDialog::reject();
}

void LoginDialog::closeEvent(QCloseEvent *event)
{
    m_auth->cancel();
    QDialog::closeEvent(event);
}

void LoginDialog::onStatusChanged(const QString &status)
{
    m_statusLabel->setText(status);
}

void LoginDialog::onDeviceCode(const QString &userCode,
                               const QString &verificationUri,
                               const QString &message)
{
    m_pendingRefresh = false;
    m_verificationUri = verificationUri;

    m_codeLabel->setText(userCode);
    m_linkLabel->setText(QStringLiteral("<a href=\"%1\">%1</a>")
                             .arg(verificationUri.toHtmlEscaped()));
    m_actionButton->setEnabled(true);

    QGuiApplication::clipboard()->setText(userCode);
    m_statusLabel->setText(tr("登录代码已复制到剪贴板，正在打开浏览器…"));
    if (!message.isEmpty())
        m_statusLabel->setToolTip(message);

    QDesktopServices::openUrl(QUrl(verificationUri));
}

void LoginDialog::onCopyAndOpen()
{
    const QString code = m_codeLabel->text();
    if (!code.isEmpty() && code != QStringLiteral("------"))
        QGuiApplication::clipboard()->setText(code);

    QString uri = m_verificationUri;
    if (uri.isEmpty())
        uri = QStringLiteral("https://microsoft.com/link");
    QDesktopServices::openUrl(QUrl(uri));

    m_statusLabel->setText(tr("代码已复制。请在浏览器中粘贴代码并完成登录，此窗口会自动关闭。"));
}

void LoginDialog::onLoginSucceeded(const PlayerInfo &player)
{
    m_player = player;
    accept();
}

void LoginDialog::onLoginFailed(const QString &error)
{
    // 静默刷新失败时自动改走完整的设备代码流程。
    if (m_pendingRefresh) {
        m_pendingRefresh = false;
        m_auth->startDeviceCodeFlow();
        return;
    }

    m_progress->hide();
    m_codeLabel->setText(QStringLiteral("------"));
    if (error.contains(QStringLiteral("AppRegInfo"), Qt::CaseInsensitive)
        || error.contains(QStringLiteral("Client ID"), Qt::CaseInsensitive)) {
        // 应用未获 Minecraft 授权时，给一个可以直接打开的申请入口。
        m_linkLabel->setText(QStringLiteral(
            "<a href=\"https://aka.ms/AppRegInfo\">打开 Minecraft 应用授权申请页面</a>"));
    } else {
        m_linkLabel->clear();
    }
    m_actionButton->hide();
    m_retryButton->show();
    m_cancelButton->setText(tr("关闭"));
    m_statusLabel->setText(tr("登录失败：") + error);
}

void LoginDialog::onRetry()
{
    m_retryButton->hide();
    m_actionButton->show();
    m_actionButton->setEnabled(false);
    m_progress->show();
    m_cancelButton->setText(tr("取消"));
    m_codeLabel->setText(QStringLiteral("------"));
    m_linkLabel->clear();
    m_statusLabel->setText(tr("正在重新获取登录代码…"));

    m_auth->startDeviceCodeFlow();
}
