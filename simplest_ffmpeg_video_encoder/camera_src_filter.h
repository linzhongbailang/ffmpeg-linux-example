#ifndef _CAMERA_SRC_FILTER_H
#define _CAMERA_SRC_FILTER_H

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



class Ccamera_src_filter{
public:
    Ccamera_src_filter(void );
    ~Ccamera_src_filter(void);

    void init_camera_device(void);
    void init_mmap(void);
    void read_frame(unsigned char * data_ptr,int data_len);
    void release_devuce(void);
    //class Cpix_buf{
    //    public:
    //        int a;


    //}pix_buf;
    
    //queue<int> frame_data;
private:
    std::mutex mut;
    int fd;
   
    int frame_index=0;

};





#endif


