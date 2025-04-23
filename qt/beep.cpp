#include "beep.h"

#include "Beep.h"
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

#define GPIO_SYSFS "/sys/class/gpio"
#define GPIO_PIN 118 // beep连接的GPIO引脚号

Beep::Beep()
{
    this->Pin_Init();
}

int Beep::PinExport(int gpio_pin)
{
    char path[64];
        int fd;

        snprintf(path, sizeof(path), "%s/export", GPIO_SYSFS);
        fd = open(path, O_WRONLY);
        if (fd < 0) {
            perror("Failed to open export file");
            return -1;
        }

        if (dprintf(fd, "%d", gpio_pin) < 0) {
            perror("Failed to export GPIO");
            close(fd);
            return -1;
        }

        close(fd);
        return 0;
}

int Beep::PinDirection(int gpio_pin, const char *direction)
{
    char path[64];
    int fd;

    snprintf(path, sizeof(path), "%s/gpio%d/direction", GPIO_SYSFS, gpio_pin);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open direction file");
        return -1;
    }

    if (dprintf(fd, "%s", direction) < 0) {
        perror("Failed to set GPIO direction");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int Beep::PinRead(int gpio_pin)
{
    char path[64];
    int fd;
    char value[2];

    snprintf(path, sizeof(path), "%s/gpio%d/value", GPIO_SYSFS, gpio_pin);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open value file");
        return -1;
    }

    if (read(fd, value, sizeof(value)) < 0) {
        perror("Failed to read GPIO value");
        close(fd);
        return -1;
    }

    close(fd);
    return atoi(value);
}

void Beep::Pin_Init()
{
    PinExport(GPIO_PIN);
    PinDirection(GPIO_PIN,"out");
}

/* 程序意外终止时执行echo 119 > /sys/class/gpio/unexport，取消导出引脚 */
void handle_signal(int sig) {
    // 执行你希望在终止前执行的代码
    system("echo 118 > /sys/class/gpio/unexport");

    // 你也可以在这里做一些其他的清理工作

    printf("unexport pin 119 for flame sensor\n");
    exit(0); // 正常退出程序
}

void Beep::run()
{

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    while (1) {
        int value = PinRead(GPIO_PIN);
        if (value < 0) {
            fprintf(stderr, "Failed to read GPIO pin %d value\n", GPIO_PIN);
            return ;
        }

        if (value == 1) {//无火焰时，DO引脚输出高电平
           emit sigfire(false);
           printf("No Flame detected!\n");
        } else {//有火焰时AO输出低电平
            emit sigfire(true);
            printf("flame detected.\n");
        }
        sleep(1); // 每秒读取一次
    }
}

