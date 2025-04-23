#include "beepwarning.h"
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <QDebug>
#include <sys/ioctl.h>

BeepWarning::BeepWarning()
{
    this->beepInit();
    this->beepMute();
}

void BeepWarning::beepInit()
{
   this->bFd = open("/dev/100ask_beep0",O_RDWR);
   if (bFd < 0)
   {
        perror("Failed to open beep dev");
        return ;
   }
}

void BeepWarning::beepRing()
{
    int ret = 0;
    int flag = 1;
    ret = write(this->bFd,&flag,1);
    if(ret<0)
    {
        perror("Failed to beep ring");
        close(this->bFd);
        return ;
    }
}

void BeepWarning::beepMute()
{
    int ret = 0;
    int flag = 0;
    ret = write(this->bFd,&flag,1);
    if(ret<0)
    {
        perror("Failed to beep mute");
        close(this->bFd);
        return ;
    }
}
