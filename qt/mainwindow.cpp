#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <QTextStream>
int Numberoferrors =3;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    login =new Login();
    regist = new Register();

    /****************************** 启动光电开关线程  ****************************/
    connect(&detectThread,SIGNAL(sighuman()),this,SLOT(loginSlot()),Qt::BlockingQueuedConnection);
    detectThread.start();

    connect(&firethread,SIGNAL(sigfire(bool)),this,SLOT(fireSlot(bool)));
    firethread.start();


}

MainWindow::~MainWindow()
{
    delete ui;
}
/********************************  门禁登陆  *****************************************/
void MainWindow::loginSlot()
{
     disconnect(&detectThread,SIGNAL(sighuman()),this,SLOT(loginSlot()));//避免执行登录逻辑时重复调用该槽函数显示界面连接下列信号和槽
     //system("echo 0 > /sys/class/graphics/fb0/blank");//立即点亮屏幕
     //connect(&detectThread,SIGNAL(autoExit()),login,SLOT(pushbutton_exitSlot()),Qt::BlockingQueuedConnection);
     login->show();

     connect(login,SIGNAL(loginsuccess()),this,SLOT(loginsuccessSlot()));
     connect(login,SIGNAL(loginfailed()),this,SLOT(loginfailedSlot()));
     connect(login,SIGNAL(loginclose()),this,SLOT(logincloseSlot()));
}
void MainWindow::loginsuccessSlot()
{
     QMessageBox::information(this, tr("information"),"密码正确，门锁已打开");
    // 重置错误次数
     Numberoferrors =3;
     login->close();
     beep.beepMute();
     regist->show();
     connect(&detectThread,SIGNAL(sighuman()),this,SLOT(loginSlot()));
}
void MainWindow::loginfailedSlot()
{
    QString info;
    Numberoferrors--;
    switch(Numberoferrors)
    {

    case 2:
        info ="密码错误,还有3次机会";
        break;
    case 1:
        info ="密码错误,还有2次机会";
        break;
    case 0:
        info ="密码错误,还有1次机会";
        break;
    default:
        info ="即将报警";
        break;
    }

    QMessageBox::warning(this, tr("warning"),info);
    if(Numberoferrors <0)
    {
        qDebug() << Numberoferrors;
        beep.beepRing();
    }
}
void MainWindow::logincloseSlot()
{
    //disconnect(&detectThread,SIGNAL(autoExit()),login,SLOT(pushbutton_exitSlot()));
    connect(&detectThread,SIGNAL(sighuman()),this,SLOT(loginSlot()));//这次登录退出后再次连接登录信号槽，否则无法再进入登陆界面了
}

void MainWindow::fireSlot(bool fire)
{
    if(fire == true)
    {
        ui->label_fire->setText("是");
        ui->label_fire->setStyleSheet("background-color:Red;");
        beep.beepRing();
    }else if(fire == false && Numberoferrors >= 0){//避免错误次数到达了蜂鸣器还会被火焰传感器逻辑关闭
        ui->label_fire->setText("否");
        ui->label_fire->setStyleSheet("background-color:green;");
        beep.beepMute();
    }
}

void MainWindow::on_openBt_clicked()
{
    connect(&vapi, &V4l2Api::sendImage, this, &MainWindow::recvImage);
    vapi.start();//在槽函数中v4l2api实例化对象vapi调用start()后就会自动调用run函数
}

void MainWindow::on_closeBt_clicked()
{
    disconnect(&vapi, &V4l2Api::sendImage, this, &MainWindow::recvImage);
    ui->label_4->clear();
}
void MainWindow::recvImage(QImage image)//这个参数会通过信号传给这个槽函数将图像显示在label标签
{
    qDebug() << "recving image" ;
    QPixmap mmp = QPixmap::fromImage(image);
    ui->label_4->setPixmap(mmp);

}



