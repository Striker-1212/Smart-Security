#ifndef FireMonitor_H
#define FireMonitor_H
#include <QThread>

class FireMonitor : public QThread
{
    Q_OBJECT
public:
    FireMonitor();
    virtual void run();//虚函数
    void Pin_Init();
    int PinRead();
private:
    int fFd;
signals:
    void sigfire(bool);//发送起火的信号给槽函数

};

#endif // FireMonitor_H
