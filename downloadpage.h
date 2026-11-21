#ifndef DOWNLOADPAGE_H
#define DOWNLOADPAGE_H

#include <QWidget>
#include "download.h"
namespace Ui {
class DownloadPage;
}

class DownloadPage : public QWidget
{
    Q_OBJECT

public:
    explicit DownloadPage(QWidget *parent = nullptr);
    ~DownloadPage();
private slots:
    void on_RefreshButton_clicked();

    void on_DownloadButton_clicked();

private:
    Ui::DownloadPage *ui;
    Download* downloader = Download::getInstance();
    Logger *logger = Logger::getInstance();
    MinecraftVersion now;
    bool selected = false;
};

#endif // DOWNLOADPAGE_H
