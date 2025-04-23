#include "HumanDetect.h"

#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <QDebug>
//#define GPIO_SYSFS "/sys/class/gpio"
//#define GPIO_PIN 117 // light传感器连接的GPIO引脚号
//echo 117 > /sys/class/gpio/export

HumanDetect::HumanDetect()
{
    this->Pin_Init();
}

void HumanDetect::Pin_Init()
{
    this->hFd = open("/dev/100ask_gpio_light",O_RDONLY);
    if (hFd < 0)
    {
         perror("Failed to open light sensor");
         return ;
    }
}

int HumanDetect::PinRead()
{
    int value;
    if (read(this->hFd, &value, 4) < 0) {
        perror("Failed to read value");
        close(this->hFd);
        return -1;
    }
    return (value & 0x1);
}

void HumanDetect::run()
{

    int value = -1;
    while (1) {
        value = PinRead();
        if (value == 1 )
        {//逻辑值,1代表开设备，实际电平是0
            emit sighuman();
            qDebug()<<tr("please login !!");
        }
        else if(value == 0 )
        {
            //emit autoExit();
            //qDebug()<<tr("Exit !!");
            qDebug()<<tr("nobody !!");
        }

    }
}
