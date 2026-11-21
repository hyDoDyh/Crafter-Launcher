#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "launcherpage.h"
#include "ui_launcherpage.h"
#include "logger.h"
#include "downloadpage.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void on_LauncherButton_clicked();


    void on_DownloadButton_clicked();


    void on_SetMainFolderButton_clicked();

private:
    Ui::MainWindow *ui;
    LauncherPage *LauncherPageInstance;
    DownloadPage *DownloadPageInstance;
    Logger* logger;
    Download *downloader = Download::getInstance();
    void ExitAnim();
    void SetGameDir();
};
#endif // MAINWINDOW_H
