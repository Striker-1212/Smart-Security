#include "ffmpeg_stream.h"

// 全局定义FFmpeg进程
QProcess *ffmpegProcess;

ffmpeg_stream::ffmpeg_stream()
{

}

// 初始化FFmpeg推流进程
void ffmpeg_stream::init_ffmpeg_stream() {

    ffmpegProcess = new QProcess();
    QStringList args;

    // 根据摄像头格式调整参数（示例：YUV420P输入，H.264编码，RTMP推流）
    args << "-f" << "rawvideo"          // 输入格式为原始视频
         //<< "-pixel_format" << "yuv422p" // 像素格式
         << "-video_size" << "640x480"   // 分辨率
         << "-framerate" << "10"        // 帧率
         << "-i" << "pipe:0"            // 从标准输入读取数据
         << "-c:v" << "libx264"         // 编码器
         << "-preset" << "ultrafast"    // 编码速度预设
         << "-f" << "flv"               // 输出格式
         << "rtmp://127.0.0.1:1935/live/wei"; // 推流地址

    ffmpegProcess->start("ffmpeg", args);
    qDebug()<< "exec ffmpeg command!" << endl;
}

void ffmpeg_stream::push_to_stream(const QByteArray &frame_data) {
    if (ffmpegProcess && ffmpegProcess->state() == QProcess::Running) {
        ffmpegProcess->write(frame_data);
    }
}

// 停止推流
void ffmpeg_stream::stop_stream() {
    if (ffmpegProcess) {
        ffmpegProcess->closeWriteChannel(); // 关闭输入管道
        ffmpegProcess->waitForFinished();
        delete ffmpegProcess;
        ffmpegProcess = nullptr;
    }
}
