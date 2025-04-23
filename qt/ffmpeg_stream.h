#ifndef FFMPEG_STREAM_H
#define FFMPEG_STREAM_H
#include <QProcess>
#include <QByteArray>
#include <QDebug>
class ffmpeg_stream:public QObject
{

public:

    ffmpeg_stream();
    void init_ffmpeg_stream();
    void push_to_stream(const QByteArray &frame_data);
    void stop_stream();
};

#endif // FFMPEG_STREAM_H
