#ifndef _DECODE_VIDEO_H
#define _DECODE_VIDEO_H

#include "opencv2/opencv.hpp"




#ifdef __cplusplus
extern "C"
{
#endif

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
};
#endif


class ffmpegDecode
{
public:
    ffmpegDecode(char * file = NULL);
    ~ffmpegDecode();

    cv::Mat getDecodedFrame();
    cv::Mat getLastFrame();
    int readOneFrame();
    int getFrameInterval();
    int get_got_picture();

    cv::Mat *pCvMatYuv422p;
    int m_skippedFrame;
    int Frame_count;

private:
    AVFrame *pAvFrame;
    AVFormatContext *pFormatCtx;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;

    int i; 
    int videoindex;
    
    char *filepath;
    int ret, got_picture;
    SwsContext *img_convert_ctx_toYUV420;
    SwsContext *img_convert_ctx_toBGR24;
    int y_size;
    AVPacket *packet;

    cv::Mat *pCvMatBGR24;
        

    void init();
    void openDecode();
    void prepare();
    void get(AVCodecContext *pCodecCtx,AVFrame *pFrame);
};

#endif

