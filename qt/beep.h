#ifndef BEEP_H
#define BEEP_H

#include <QThread>

class Beep : public QThread
{
    Q_OBJECT
public:
    Beep();
    virtual void run();//虚函数
    void Pin_Init();
    int PinRead(int gpio_pin);
private:
    int PinExport(int gpio_pin);
    int PinDirection(int gpio_pin, const char *direction);
signals:
    void sigBeep(bool);//发送起火的信号给槽函数

};

#endif // BEEP_H
