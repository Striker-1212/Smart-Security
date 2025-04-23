#include "v4l2api.h"
#include <QDebug>
#include <QImage>
#define CLAMP(value) ((value) > 255 ? 255 : ((value) < 0 ? 0 : (value)))

//#include <jpeglib.h>

/* 下面的由析构函数调用的函数在点击按钮槽函数调用run()之前就已经运行了 */

//deviceName(dname)：将构造函数的参数 dname 的值直接赋给类的成员变量 deviceName。
//count(count)：将构造函数的参数 count 的值直接赋给类的成员变量 count。
V4l2Api::V4l2Api(const char *dname, int count):deviceName(dname),count(count)//
{
    this->open();
}

V4l2Api::~V4l2Api()
{
    this->close();
}

void V4l2Api::open()
{
    system("modprobe v4l2loopback devices=2 video_nr=2,3 exclusive_caps=0");
    //system("ffmpeg -i /dev/video1 -codec copy -f v4l2 /dev/video2 -codec copy -f v4l2 /dev/video3");
    //system("ffmpeg -f v4l2 -i /dev/video2  -fflags nobuffer -f flv rtmp://127.0.0.1:1935/live/wei");
    video_init();

    video_mmap();


#if 1
    //开始采集
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret  =  ioctl(this->vfd, VIDIOC_STREAMON, &type);
    if(ret < 0)
    {
        perror("start fail");
    }
#endif
}

void V4l2Api::close()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret  =  ioctl(this->vfd, VIDIOC_STREAMOFF, &type);
    if(ret < 0)
    {
        perror("stop fail");
    }
    //释放映射d内存
    for(int i=0; i<this->framebuffers.size(); i++)
    {
        munmap(framebuffers.at(i).start, framebuffers.at(i).length);
    }
}
//VIDIOC_DQBUF,buf出队，并对图像数据进行处理
void V4l2Api::grapImage(unsigned char *imageBuffer, int *length)//都是返回参数
{
    int iRet = -1;
    struct pollfd tFds[1];
    tFds->fd = this->vfd;
    tFds->events = POLLIN;
    iRet = poll(tFds,1,-1);
    if(iRet < 0)
    {
        perror("poll error!\n");
    }
    //select (rfds, wfds, efds, time)

    struct v4l2_buffer readbuf;
    memset(&readbuf, 0, sizeof(struct v4l2_buffer));
    readbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    readbuf.memory = V4L2_MEMORY_MMAP;
    //perror("read");
    if(ioctl(this->vfd, VIDIOC_DQBUF, &readbuf)<0)//取一帧数据
    {
        perror("read image fail");
    }
//    printf("%ld\n", readbuf.length);
    *length = readbuf.length;
    /* 按index将容器中对应的buf内存映射到传入参数 */
    memcpy(imageBuffer,framebuffers[readbuf.index].start, framebuffers[readbuf.index].length);

    //把用完的队列空间放回队列中重复使用
    if(ioctl(vfd, VIDIOC_QBUF, &readbuf)<0)
    {
        perror("destroy fail");
        exit(1);
    }
}

void V4l2Api::video_init()
{
    //1.打开设备
    this->vfd = ::open("/dev/video3", O_RDWR | O_NONBLOCK);//video3虚拟设备,摄像头复用
    if(this->vfd < 0)
    {
        qDebug("open video fail");
        return;
//        VideoException vexp("open fail");//创建异常对象
//        //抛异常
//        throw vexp;
    }
    //2.配置采集属性
    struct v4l2_format vfmt;
    vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; //表示当前操作的缓冲区类型是视频捕获（video capture）。
    vfmt.fmt.pix.width = WIDTH;// 播放视频宽度
    vfmt.fmt.pix.height = HEIGHT;//播放视频高度
    vfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;//（设置视频输出格式，但是要摄像头支持4:2:2）
    //通过ioctl把属性写入设备
    int ret  = ioctl(this->vfd, VIDIOC_S_FMT, &vfmt);
    if(ret < 0)
    {
        return;
//        VideoException vexp("set fail");//创建异常对象
//        throw vexp;
    }
    //通过ioctl从设备获取属性,检查确保属性正确设置
    memset(&vfmt, 0, sizeof(vfmt));
    vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(this->vfd, VIDIOC_G_FMT, &vfmt);
    if(ret < 0)
    {
        return;
//        VideoException vexp("get fail");//创建异常对象
//        throw vexp;
    }
    /* 确保设置的这三个属性值不为空 */
    if(vfmt.fmt.pix.width == WIDTH && vfmt.fmt.pix.height==HEIGHT
            && vfmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
    {

    }else {
        return;
//        VideoException vexp("get fail");//创建异常对象
//        throw vexp;
    }
}

void V4l2Api::video_mmap()
{
    //1申请缓冲区队列
    struct v4l2_requestbuffers reqbuffer;
    reqbuffer.count = this->count;//申请缓冲区队列长度
    reqbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuffer.memory = V4L2_MEMORY_MMAP;
    //从队列中拿到内核空间,申请buffer，APP可以申请很多个buffer，但是驱动程序不一定能申请到
    int ret = ioctl(this->vfd, VIDIOC_REQBUFS, &reqbuffer);
    if(ret < 0)
    {
        return;
//        VideoException vexp("get fail");//创建异常对象
//        throw vexp;;
    }

    //2.映射
    for(int i=0; i<this->count; i++)
    {
        struct VideoFrame frame;//映射地址和内存长度存储在这个结构体里

        struct v4l2_buffer mapbuffer;
        mapbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mapbuffer.index = i;//第几个缓冲区
        mapbuffer.memory = V4L2_MEMORY_MMAP;

        ret  = ioctl(this->vfd, VIDIOC_QUERYBUF, &mapbuffer);
        if(ret < 0)
        {
            return;
    //        VideoException vexp("get fail");//创建异常对象
    //        throw vexp;
        }
        //映射
        frame.length = mapbuffer.length;
        frame.start = (char *)mmap(NULL, mapbuffer.length, PROT_READ|PROT_WRITE, MAP_SHARED, this->vfd, mapbuffer.m.offset);

        //空间放回队列中（内核空间）"空闲链表",
        ret = ioctl(this->vfd, VIDIOC_QBUF, &mapbuffer);

        //把frame添加到容器framebuffers,从容器中取数据
        framebuffers.push_back(frame);
    }
}

//bool V4l2Api::yuyv_to_rgb888(unsigned char * input_ptr, unsigned char * output_ptr, int image_width, int image_height)


bool V4l2Api::yuyv_to_rgb888(unsigned char *yuyvdata, unsigned char *rgbdata, int picw, int pich)
{
    int i, j;
       unsigned char y1,y2,u,v;
       int r1,g1,b1,r2,g2,b2;

       //确保所转的数据或要保存的地址有效
       if(yuyvdata == NULL || rgbdata == NULL)
       {
           return false;
       }

       int tmpw = picw/2;
       for(i=0; i<pich; i++)
       {
           for(j=0; j<tmpw; j++)// 640/2  == 320
           {
               //yuv422
               //R = 1.164*(Y-16) + 1.159*(V-128);
               //G = 1.164*(Y-16) - 0.380*(U-128)+ 0.813*(V-128);
               //B = 1.164*(Y-16) + 2.018*(U-128));

               //下面的四个像素为：[Y0 U0 V0] [Y1 U1 V1] -------------[Y2 U2 V2] [Y3 U3 V3]
               //存放的码流为：    Y0 U0 Y1 V1------------------------Y2 U2 Y3 V3
               //映射出像素点为：  [Y0 U0 V1] [Y1 U0 V1]--------------[Y2 U2 V3] [Y3 U2 V3]

               //获取每个像素yuyv数据   YuYv
               y1 = *(yuyvdata + (i*tmpw+j)*4);                //yuv像素的Y
               u  = *(yuyvdata + (i*tmpw+j)*4+1);              //yuv像素的U
               y2 = *(yuyvdata + (i*tmpw+j)*4+2);
               v  = *(yuyvdata + (i*tmpw+j)*4+3);

               //把yuyv数据转换为rgb数据
               r1 = 1.164*(y1-16) + 2.018*(u-128);
               g1= 1.164*(y1-16) - 0.380*(v-128)- 0.394*(v-128);
               b1 = 1.164*(y1-16) + 1.159*(v-128);
               r2 = 1.164*(y2-16) + 2.018*(u-128);
               g2= 1.164*(y2-16) - 0.380*(v-128)- 0.394*(v-128);
               b2 = 1.164*(y2-16) + 1.159*(v-128);

   //            r1 = 1.164*(y1-16) + 1.159*(v-128);
   //            g1= 1.164*(y1-16) - 0.380*(v-128)+ 0.813*(v-128);
   //            b1 = 1.164*(y1-16) + 2.018*(u-128);
   //            r2 = 1.164*(y2-16) + 1.159*(v-128);
   //            g2= 1.164*(y2-16) - 0.380*(v-128)+ 0.813*(v-128);
   //            b2 = 1.164*(y2-16) + 2.018*(u-128);

               if(r1 > 255) r1=255;
               else if(r1 < 0) r1 = 0;

               if(g1 > 255) g1=255;
               else if(g1 < 0) g1 = 0;

               if(b1 > 255) b1=255;
               else if(b1 < 0) b1 = 0;

               if(r2 > 255) r2=255;
               else if(r2 < 0) r2 = 0;

               if(g2 > 255) g2=255;
               else if(g2 < 0) g2 = 0;

               if(b2 > 255) b2=255;
               else if(b2 < 0) b2 = 0;

               //把rgb值保存于rgb空间 数据为反向
   //            rgbdata[((pich-1-i)*tmpw+j)*6]     = (unsigned char)b1;
   //            rgbdata[((pich-1-i)*tmpw+j)*6 + 1] = (unsigned char)g1;
   //            rgbdata[((pich-1-i)*tmpw+j)*6 + 2] = (unsigned char)r1;
   //            rgbdata[((pich-1-i)*tmpw+j)*6 + 3] = (unsigned char)b2;
   //            rgbdata[((pich-1-i)*tmpw+j)*6 + 4] = (unsigned char)g2;
   //            rgbdata[((pich-1-i)*tmpw+j)*6 + 5] = (unsigned char)r2;

               rgbdata[((i+1)*tmpw+j)*6]     = (unsigned char)b1;
               rgbdata[((i+1)*tmpw+j)*6 + 1] = (unsigned char)g1;
               rgbdata[((i+1)*tmpw+j)*6 + 2] = (unsigned char)r1;
               rgbdata[((i+1)*tmpw+j)*6 + 3] = (unsigned char)b2;
               rgbdata[((i+1)*tmpw+j)*6 + 4] = (unsigned char)g2;
               rgbdata[((i+1)*tmpw+j)*6 + 5] = (unsigned char)r2;
           }
       }
       memcpy(yuyvdata,rgbdata,HEIGHT*WIDTH*3);


   // pr_debug("\tchange to RGB OK \n");
    return true;
}




void V4l2Api::jpeg_to_rgb888(unsigned char *jpegData, int size, unsigned char *rgbdata)
{
    Q_UNUSED(jpegData);
    Q_UNUSED(size);
    Q_UNUSED(rgbdata);
//    //解码jpeg图片
//    //1.定义解码对象struct jpeg_decompress_struct 错误处理对象struct jpeg_error_mgr;
//    struct jpeg_decompress_struct cinfo;
//    struct jpeg_error_mgr err;

//    //2.初始化错误jpeg_std_error(err)，创建初始化解码对象jpeg_create_decompress();
//    cinfo.err = jpeg_std_error(&err);
//    jpeg_create_decompress(&cinfo);

//    //3.加载源数据jpeg_mem_src()
//    jpeg_mem_src(&cinfo, jpegData, size);

//    //4.获取jpeg图片头数据
//    jpeg_read_header(&cinfo, true);

//    //5.开始解码
//    jpeg_start_decompress(&cinfo);

//    //6.分配存储一行像素所需要的空间//---RGB数据
//    //640--cinfo.output_width, 480--cinfo.output_height
//    char *rowFrame=(char*)malloc(cinfo.output_width*3);

//    int pos = 0;
//    //7.一行一行循环读取（一次读取一行，要全部读完）
//    while(cinfo.output_scanline<cinfo.output_height)
//    {
//            //读取一行数据--解码一行
//            jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&rowFrame, 1);

//            //把rgb像素显示在lcd上 mmp
//            memcpy(rgbdata+pos, rowFrame,cinfo.output_width*3);
//            pos += cinfo.output_width*3;
//    }
//    free(rowFrame);
//    //8.解码完成
//    jpeg_finish_decompress(&cinfo);
//    //9.销毁解码对象
//    jpeg_destroy_decompress(&cinfo);
}
void V4l2Api::enum_formats(void){

    struct v4l2_fmtdesc fmtdesc ;
    /* 枚举摄像头所支持的所有像素格式以及描述信息 */
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (0 == ioctl(this->vfd, VIDIOC_ENUM_FMT, &fmtdesc)) {
    // 将枚举出来的格式以及描述信息存放在数组中
        cam_fmts[fmtdesc.index].pixelformat = fmtdesc.pixelformat;
        fmtdesc.index++;
    }
}

void V4l2Api::print_formats(void)
{
    struct v4l2_frmsizeenum frmsize ;
    struct v4l2_frmivalenum frmival ;
    int iRet;
    int i;
    int v4l2_fd = this->vfd;

    frmsize.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frmival.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    for (i = 0; cam_fmts[i].pixelformat; i++) {
        printf("format<0x%x>, \n", cam_fmts[i].pixelformat);

    /* 枚举出摄像头所支持的所有视频采集分辨率 */
    frmsize.index = 0;
    frmsize.pixel_format = cam_fmts[i].pixelformat;
    frmival.pixel_format = cam_fmts[i].pixelformat;
    while (0==(iRet = ioctl(v4l2_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize))) {
        printf("size<%d*%d> ",frmsize.discrete.width,frmsize.discrete.height);
        frmsize.index++;//枚举设备支持的所有分辨率。
        /* 获取摄像头视频采集帧率 */
        frmival.index = 0;
        frmival.width = frmsize.discrete.width;
        frmival.height = frmsize.discrete.height;
        while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival)) {
            cout << (frmival.discrete.denominator / frmival.discrete.numerator) << "<fps>" ;
            frmival.index++;//枚举设备支持的所有帧间隔。
        }
        cout<<endl;
    }

 }
}

void V4l2Api::printf_message(void){
    enum_formats();
    print_formats();
}

void V4l2Api::run()
{
    unsigned char buffer[WIDTH*HEIGHT*4];
    unsigned char rgbbuffer[WIDTH*HEIGHT*4];
    int len;

    //打印分辨率 帧率
    printf_message();
    while(1)
    {
        grapImage(buffer, &len);

        yuyv_to_rgb888((unsigned char *)buffer, (unsigned char *)rgbbuffer);

        //把RGB数据转为QImage
        QImage image((uchar*)rgbbuffer, WIDTH, HEIGHT, QImage::Format_RGB888);

        emit sendImage(image);
        msleep(1);
        //qDebug() << "emit image" ;
    }
}
