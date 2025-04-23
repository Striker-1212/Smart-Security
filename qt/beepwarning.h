#ifndef BEEPWARNING_H
#define BEEPWARNING_H

class BeepWarning
{
private:
    int bFd;
public:
    BeepWarning();
    void beepInit();
    void beepRing();
    void beepMute();

};

#endif // BEEPWARNING_H
