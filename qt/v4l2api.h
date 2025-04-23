#ifndef V4L2API_H
#define V4L2API_H
#include <iostream>
#include <vector>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <string.h>
#include <exception>
#include <QThread>
#include <QImage>
#include <QObject>


using namespace  std;

//监控显示尺寸
const int WIDTH= 640;
const int HEIGHT=480;

//异常类
class VideoException : public exception
{
public:
    VideoException(string err):errStr(err) {}
    ~VideoException(){};
    const char* what()const  noexcept
    {
        return errStr.c_str();
    }
private:
    string errStr;
};

//显存
struct VideoFrame
{
    char *start; //保存内核空间映射到用户空间的空间首地址
    int length;//空间长度
};

typedef struct camera_format {
unsigned char description[32]; //字符串描述信息
unsigned int pixelformat; //像素格式
} cam_fmt;

//
class V4l2Api :public QThread
{
    Q_OBJECT
public:
    V4l2Api(const char *dname="/dev/video3", int count=4);//预传参/dev/video*,video3虚拟设备
    ~V4l2Api();
    void open();
    void close();
    void grapImage(unsigned char *imageBuffer, int *length);//捕获图像
    bool yuyv_to_rgb888(unsigned char *yuyvdata, unsigned char *rgbdata, int picw=WIDTH, int pich=HEIGHT);//格式转换
    void printf_message();
    //定义run函数
    void run();//在槽函数数中调用start()后就会自动调用run函数,需要在cpp文件中覆盖它
    void jpeg_to_rgb888(unsigned char *jpegData, int size, unsigned char *rgbdata);
private:
    void video_init();//V4l2设备初始化
    void video_mmap();//分配显存
    void enum_formats();
    void print_formats();//打印帧信息(捕获视频数据分辨率，帧率)

private:
    string deviceName;
    int vfd;//保存文件描述符
    int count;//缓冲区个数
    cam_fmt cam_fmts[10];
    //framebuffers 是一个 std::vector 容器，用于存储 VideoFrame 类型的结构体。
    //它是一个动态数组，可以方便地添加、删除和访问 VideoFrame 对象。
    vector<struct VideoFrame> framebuffers;


signals:
    void sendImage(QImage);//通过信号Qimage参数传递给槽函数
};

#endif // V4L2API_H
