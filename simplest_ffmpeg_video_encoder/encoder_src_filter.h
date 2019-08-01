#ifndef _ENCODER_SRC_FILTER_H
#define _ENCODER_SRC_FILTER_H

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


using namespace std;
using namespace cv;



typedef struct {
    unsigned char * data_ptr;
    unsigned int data_len;
    int frame_index;

}FRAME_DATA;


class Cmux_source_filter{
public:
    Cmux_source_filter(void );
    ~Cmux_source_filter(void);

    int Put_frame(unsigned char * buf_ptr,unsigned int data_len);
    int Put_frame(Mat mat);
    int Get_frame(unsigned char * buf_ptr);    
    int frame_num(void);
    int Empty(void);
    enum AVPixelFormat pix_fmt;
    int cows;
    int rows;
    
     
    //class Cpix_buf{
    //    public:
    //        int a;


    //}pix_buf;
    
    //queue<int> frame_data;
private:
    queue<FRAME_DATA> frame_data_queue;
    queue <int > queue_test;
    unsigned int frame_count;

    int reserve;
    std::mutex mut;

};



void mux_source_thread_main(void);


#endif


