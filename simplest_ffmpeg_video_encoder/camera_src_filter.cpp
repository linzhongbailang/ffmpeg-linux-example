#include <stdio.h>
#include <iostream>
#include <queue>
#include <thread>
#include <mutex>          // std::mutex, std::lock_guard

#include <opencv2/opencv.hpp> 

#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/ioctl.h>
#include <errno.h>

#include <unistd.h>
#include <linux/videodev2.h>
#include<sys/mman.h>


#define __STDC_CONSTANT_MACROS


#ifdef __cplusplus
extern "C"
{
#endif

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>

#ifdef __cplusplus
};
#endif

#include "encoder_src_filter.h"
#include "../util/CPictureFormatConversion.h"
#include "camera_src_filter.h"


//test different codec
#define TEST_H264  1
#define TEST_HEVC  0




/*
** 封装过程的数据源的过滤器

*/
using namespace std;
using namespace cv;





//#define TRUE            (1)
//#define FALSE           (0)

#define FILE_VIDEO      "/dev/video0"
//#define IMAGE           "./img/demo"

//#define IMAGEWIDTH      640
//#define IMAGEHEIGHT     480

//#define FRAME_NUM       4

struct buffer
{
void* start;
unsigned int length;
}*buffers;

Ccamera_src_filter::Ccamera_src_filter(void)
{

    frame_index=0;
}
Ccamera_src_filter::~Ccamera_src_filter(void)
{

}


void Ccamera_src_filter::init_camera_device(void)
{
    struct v4l2_capability cap;
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_format fmt;
    struct v4l2_streamparm stream_para;
    
    //int fd=open(FILE_VIDEO,O_RDWR|O_NONBLOCK); // 打开设备
    fd=open(FILE_VIDEO,O_RDWR); // 打开设备
    if(fd<0)
        cout << "open /dev/video0 failed" << endl;
    
    cout << "open /dev/video0 sucess fd="<< fd << endl;
    //查询设备属性
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) 
    {
        printf("Error opening device %s: unable to query device.\n",FILE_VIDEO);
        return ;
    }
    else
    {
        printf("driver:\t\t%s\n",cap.driver);
        printf("card:\t\t%s\n",cap.card);
        printf("bus_info:\t%s\n",cap.bus_info);
        printf("version:\t%d\n",cap.version);
        printf("capabilities:\t%x\n",cap.capabilities);
        
        if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE) 
        {
            printf("Device %s: supports capture.\n",FILE_VIDEO);
        }

        if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING) 
        {
            printf("Device %s: supports streaming.\n",FILE_VIDEO);
        }
    }

    //查看帧格式
    fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd,VIDIOC_G_FMT,&fmt);
    cout << "Currentdata format (" <<fmt.fmt.pix.width << "x" <<fmt.fmt.pix.height <<") " \
        << "pixFromat :" << (char) (fmt.fmt.pix.pixelformat & 0xFF) \
                         << (char) ((fmt.fmt.pix.pixelformat >> 8) & 0xFF) \
                         << (char) ((fmt.fmt.pix.pixelformat >> 16) & 0xFF) \
                         << (char) ((fmt.fmt.pix.pixelformat >> 24) & 0xFF) << endl;
#if 1
    //设置帧格式
    //struct v4l2_format fmt;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	if(-1 == ioctl(fd, VIDIOC_S_FMT, &fmt)){//设置图片格式
		perror("set format failed!");
		close(fd); // 关闭设备
		return ;
	}
    //查看帧格式
    fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd,VIDIOC_G_FMT,&fmt);
    cout << "Currentdata format (" <<fmt.fmt.pix.width << "x" <<fmt.fmt.pix.height <<") " \
        << "pixFromat :" << (char) (fmt.fmt.pix.pixelformat & 0xFF) \
                         << (char) ((fmt.fmt.pix.pixelformat >> 8) & 0xFF) \
                         << (char) ((fmt.fmt.pix.pixelformat >> 16) & 0xFF) \
                         << (char) ((fmt.fmt.pix.pixelformat >> 24) & 0xFF) << endl;
#endif



}


void Ccamera_src_filter::init_mmap(void)
{
    struct v4l2_requestbuffers req;
    req.count=5;
    req.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory=V4L2_MEMORY_MMAP;
    
    ioctl(fd,VIDIOC_REQBUFS,&req);

    if (req.count < 2){
		perror("buffer memory is Insufficient! \n");
        close(fd); // 关闭设备
		return ;
	}
    cout << "request buffers number:" << req.count << endl;

    //映射
    buffers =(buffer*)calloc (req.count, sizeof (*buffers));
    if (!buffers) {
        fprintf (stderr,"Out of memory/n");
        close(fd); // 关闭设备
		return ;
    }
    
    for (unsigned int n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;
        memset(&buf,0,sizeof(buf));
        buf.type =V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory =V4L2_MEMORY_MMAP;
        buf.index =n_buffers;
        // 查询序号为n_buffers 的缓冲区，得到其起始物理地址和大小
        if (-1 == ioctl(fd, VIDIOC_QUERYBUF, &buf))
        {    
            close(fd); // 关闭设备
            cout << "query buf error" <<endl;
		    return ;
        }
        buffers[n_buffers].length= buf.length;
        // 映射内存
        buffers[n_buffers].start=mmap (NULL,buf.length,PROT_READ | PROT_WRITE ,MAP_SHARED,fd, buf.m.offset);
        if (MAP_FAILED== buffers[n_buffers].start)
        {    
            close(fd); // 关闭设备
            cout << "mmap  error" <<endl;
		    return ;
        }
        cout << "n_buffers loop" <<endl;
    }

    //
    unsigned int i;
    enum v4l2_buf_type type;
    // 将缓冲帧放入队列
    for (i = 0; i< req.count; ++i)
    {
        struct v4l2_buffer buf;
        buf.type =V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory =V4L2_MEMORY_MMAP;
        buf.index = i;
        ioctl (fd,VIDIOC_QBUF, &buf);
    }
    type =V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl (fd,VIDIOC_STREAMON, &type);

    cout << "exit init mmap" <<endl;

    

}


void Ccamera_src_filter::read_frame(unsigned char * data_ptr,int data_len)
{
    
    struct v4l2_buffer buf;

    //memset(&buf,0,sizeof(v4l2_buffer));
    buf.type =V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory =V4L2_MEMORY_MMAP;

    // 从缓冲区取出一个缓冲帧
    if(-1==ioctl (fd,VIDIOC_DQBUF, &buf))
        cout << "vidioc dqbuf error"<<endl;
    frame_index++;
#if 1
    if(frame_index==10)
    {
        FILE *fp_yuv422;
        
        cout<< "save the 10th yuv422 image" <<endl;
        fp_yuv422 = fopen("yuv422_10th_640x480.yuv", "wb");
    	if (!fp_yuv422) {
    		printf("Could not open %s\n", "yuv422_10th_640x480.yuv");
       	}
        else
        {
            fwrite(buffers[buf.index].start, 1,buffers[buf.index].length, fp_yuv422);
            fclose(fp_yuv422);

        }

        FILE *fp_yuv422p;       
        cout<< "save the 10th yuv422p image" <<endl;
        fp_yuv422p = fopen("yuv422p_10th_640x480.yuv", "wb");
    	if (!fp_yuv422p) {
    		printf("Could not open %s\n", "yuv422p_10th_640x480.yuv");
       	}
        else
        {
            unsigned char * yu422p_buff = (unsigned char *) malloc(buffers[buf.index].length);
            if(yu422p_buff == NULL)
            {
                cout << "malloc yuv422p_buff failed" << endl;
            }
            else
            {
                PictureFormatConversion::yuv422_to_yuv422p(640,480, (unsigned char *)buffers[buf.index].start,yu422p_buff);
                fwrite(yu422p_buff, 1,buffers[buf.index].length, fp_yuv422p);
            }
            free(yu422p_buff);
            fclose(fp_yuv422p);

        }

        //save yuv420p image
        FILE *fp_yuv420p;       
        cout<< "save the 10th yuv420p image" <<endl;
        fp_yuv420p = fopen("yuv420p_10th_640x480.yuv", "wb");
    	if (!fp_yuv420p) {
    		printf("Could not open %s\n", "yuv420p_10th_640x480.yuv");
       	}
        else
        {
            unsigned char * yu420p_buff = (unsigned char *) malloc(buffers[buf.index].length*3/4);
            if(yu420p_buff == NULL)
            {
                cout << "malloc yuv422p_buff failed" << endl;
            }
            else
            {
                PictureFormatConversion::yuv422_to_yuv420p(640,480, (unsigned char *)buffers[buf.index].start,yu420p_buff);
                fwrite(yu420p_buff, 1,buffers[buf.index].length*3/4, fp_yuv420p);
            }
            free(yu420p_buff);
            fclose(fp_yuv420p);

        }
    }

#endif
    //process_image(buffers[buf.index].start);       
    PictureFormatConversion::yuv422_to_yuv420p(640,480, (unsigned char *)buffers[buf.index].start,data_ptr);
    //mux_source_filter->Put_frame(yuv420p_data,640*480*3/2);        
    //cout << " " << buffers[buf.index].length<< " " <<frame_index << endl;
    
    // 将取出的缓冲帧放回缓冲区
    if(-1==ioctl (fd, VIDIOC_QBUF,&buf)){
        cout<< "vidioc qbuf error" <<endl;
    }
    

}

void Ccamera_src_filter::release_devuce(void)
{
    enum v4l2_buf_type type;
    type =V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl (fd,VIDIOC_STREAMOFF, &type);
    close(fd); // 关闭设备
}




