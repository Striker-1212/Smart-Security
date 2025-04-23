#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QMainWindow>
#include <QTimer>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>
#include <unistd.h>
#include <QDebug>
#include "login.h"
#include "register.h"
#include "HumanDetect.h"
#include "FireMonitor.h"
#include "v4l2api.h"
#include "beepwarning.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    HumanDetect detectThread;
    Login *login;
    Register *regist;
    FireMonitor firethread;
    V4l2Api vapi;//实例化对象后先调用构造函数
    BeepWarning beep;


signals:
    void autoExit();

private slots:
    void loginSlot();
    void loginfailedSlot();
    void loginsuccessSlot();
    void logincloseSlot();
    void fireSlot(bool);
    void on_openBt_clicked();
    void recvImage(QImage image);
    void on_closeBt_clicked();
    void on_label_4_windowTitleChanged(const QString &title);
    void on_label_4_linkActivated(const QString &link);
};
#endif // MAINWINDOW_H
