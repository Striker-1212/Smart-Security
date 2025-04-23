#ifndef READPE15THREAD_H
#define READPE15THREAD_H

#include <QThread>
class HumanDetect : public QThread
{
     Q_OBJECT
public:
    HumanDetect();
    virtual void run();
    void Pin_Init();
    int PinRead();
private:
    int hFd;

signals:
    void loginExit();
    void sighuman();
    void autoExit();
};

#endif // READPE15THREAD_H

