#include "FireMonitor.h"
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <QDebug>
#include <sys/ioctl.h>
#include <signal.h>

//#define GPIO_SYSFS "/sys/class/gpio"
//#define GPIO_PIN 119 // 火焰传感器连接的GPIO引脚号
//原理:无火焰时，DO引脚输出高电平,有火焰时AO输出低电平
FireMonitor::FireMonitor()
{
    this->Pin_Init();
}

void FireMonitor::Pin_Init()
{
    this->fFd = open("/dev/100ask_gpio_flame",O_RDONLY);
    if (fFd < 0)
    {
         perror("Failed to open fire sensor");
         return ;
    }
}

int FireMonitor::PinRead()
{
    int value;
    if (read(this->fFd, &value, 4) < 0) {
        perror("Failed to read value");
        close(this->fFd);
        return -1;
    }
    return (value & 0x1);
}

void FireMonitor::run()
{
    int value = -1;
    emit sigfire(false);
    while (1) {
        value = PinRead();
        if (value == 0) {
           emit sigfire(false);
        } else if(value == 1){
            emit sigfire(true);
            printf("flame detected.\n");
        }else {
            fprintf(stderr, "Failed to read fire sensor\n");
            return ;
        }

    }
}
