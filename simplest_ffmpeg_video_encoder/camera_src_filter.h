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

    //void Ccamera_src_filter::process_image(const void * p, int size);
    void errno_exit(const char * s);
    int xioctl(int fd, int request, void * arg);
    int read_frame(unsigned char * yuv420p_data);
    void process_image(const void * p, int size);

    void stop_capturing(void);
    void start_capturing(void);
    void uninit_device(void);
    void init_read(unsigned int buffer_size);
    void init_mmap(void);
    void init_userp(unsigned int buffer_size);
    void init_device(void);
    void close_device(void);
    void open_device(void);



    
private:
    std::mutex mut;


    
 
    typedef enum {
    	IO_METHOD_READ, IO_METHOD_MMAP, IO_METHOD_USERPTR,
    } io_method;
     
    struct buffer {
    	void * start;
    	size_t length;
    };


    char * dev_name;
    io_method io ;
    int fd ;
    buffer * buffers ;
    unsigned int n_buffers ;

    FILE *fp;
    char *filename ;

    int frame_index;

};





#endif


