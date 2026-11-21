#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "playerinfo.h"

class MicrosoftAuth;
class QLabel;
class QPushButton;
class QProgressBar;
class QCloseEvent;

/**
 * 微软正版登录对话框：展示设备代码、自动打开浏览器并等待登录完成。
 */
class LoginDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog() override;

    PlayerInfo player() const { return m_player; }

public slots:
    void reject() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onStatusChanged(const QString &status);
    void onDeviceCode(const QString &userCode,
                      const QString &verificationUri,
                      const QString &message);
    void onLoginSucceeded(const PlayerInfo &player);
    void onLoginFailed(const QString &error);
    void onCopyAndOpen();
    void onRetry();

private:
    MicrosoftAuth *m_auth = nullptr;
    PlayerInfo m_player;

    QLabel *m_statusLabel = nullptr;
    QLabel *m_codeLabel = nullptr;
    QLabel *m_linkLabel = nullptr;
    QPushButton *m_actionButton = nullptr;
    QPushButton *m_retryButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QProgressBar *m_progress = nullptr;

    QString m_verificationUri;
    bool m_pendingRefresh = false;
};

#endif // LOGINDIALOG_H
