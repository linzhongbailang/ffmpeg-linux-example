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



//定义类模板
template <class numtype>
class Compare
{
   public :
   Compare(numtype a,numtype b)
   {x=a;y=b;}
   numtype max( )
   {return (x>y)?x:y;}
   numtype min( )
   {return (x<y)?x:y;}
   private :
   numtype x,y;
};
    //Compare<int > cmp1(3,7);


//struct ListNode
//{
//    double value;
//    ListNode *next;
    //构造函数
//    ListNode(double valuel, ListNode *nextl = nullptr)
//    {
//        value = value1;
//        next = next1;
//    }
//};








Cmux_source_filter::Cmux_source_filter(void)
{
    std::lock_guard<std::mutex> lk(mut);
    printf ("mux_source_filter::mux_source_filter \n");
    
    frame_count=0;
}

Cmux_source_filter::~Cmux_source_filter(void)
{
    printf ("mux_source_filter::~mux_source_filter \n");

}
int Cmux_source_filter::Put_frame(unsigned char * buf_ptr,unsigned int data_len)
{
    std::lock_guard<std::mutex> lk(mut);
    printf ("mux_source_filter::Put_frame  data len%d\n",data_len);
    FRAME_DATA frame_data_temp;
    frame_count++;
    frame_data_temp.frame_index=frame_count;
 #if 1  
    if(pix_fmt==AV_PIX_FMT_YUV420P)
    {
        frame_data_temp.data_ptr=(unsigned char *)malloc(data_len);
        if(frame_data_temp.data_ptr==NULL){
            
            cout << "Cmux_source_filter::Put_frame malloc failed" <<endl;
            return 0;

        }
        memcpy(frame_data_temp.data_ptr,buf_ptr,data_len);
        frame_data_temp.data_len=data_len;
    }
#endif
    frame_data_queue.push(frame_data_temp);

    //queue_test.push(frame_count);
    return 1;
}
int Cmux_source_filter::Put_frame(Mat mat)
{
    std::lock_guard<std::mutex> lk(mut);
    
    printf ("mux_source_filter::Put_frame \n");
    FRAME_DATA frame_data_temp;
    int data_len=0;
    frame_count++;
    frame_data_temp.frame_index=frame_count;
#if 0
    if(pix_fmt==AV_PIX_FMT_YUYV422)
    {
        data_len=cows*rows*2;
        frame_data_temp.data_ptr=(unsigned char *)malloc(data_len);
        if(frame_data_temp.data_ptr==NULL){
            
            cout << "Cmux_source_filter::Put_frame malloc failed" <<endl;
            return 0;

        }
        memcpy(frame_data_temp.data_ptr,mat.data,cows*rows*2);
        frame_data_temp.data_len=data_len;
        
    }
#endif   
    frame_data_queue.push(frame_data_temp);
    return 1;

}

int Cmux_source_filter::Get_frame(unsigned char * buf_ptr)
{
    std::lock_guard<std::mutex> lk(mut);
    //printf ("mux_source_filter::Get_frame \n");
    FRAME_DATA frame_data_temp;
    frame_data_temp=frame_data_queue.front();
#if 1    
    memcpy(buf_ptr,frame_data_temp.data_ptr,frame_data_temp.data_len);
#endif

    free(frame_data_temp.data_ptr); 

    frame_data_queue.pop();
    //queue_test.pop();
    printf("mux_source_filter::get frame data len%d\n",frame_data_temp.data_len);
    //frame_data.frame_index++;
    return 1;

}
int Cmux_source_filter::frame_num(void)
{
    std::lock_guard<std::mutex> lk(mut);
    //printf ("mux_source_filter::frame_num \n");
    return frame_data_queue.size();
    //return queue_test.size();

}
int Cmux_source_filter::Empty(void)
{
    std::lock_guard<std::mutex> lk(mut);
    //printf ("mux_source_filter::frame_num \n");
    //return frame_data_queue.size();
    return frame_data_queue.empty();
    
    //return queue_test.empty();

}







extern Cmux_source_filter * mux_source_filter;




void mux_source_thread_main(void)
{
    cout << "start mux_source_thread_main" << endl;

    Ccamera_src_filter * camera_src_filter = new Ccamera_src_filter();
    

    camera_src_filter->init_camera_device();
    
    camera_src_filter->init_mmap();

 
    mux_source_filter->pix_fmt=AV_PIX_FMT_YUV420P;
    mux_source_filter->cows=640;
    mux_source_filter->rows=480;
    unsigned char * yuv420p_data; 

    yuv420p_data=(unsigned char *)malloc(640*480*3/2);
    if(yuv420p_data==NULL){
        cout<< "yuv420p_data malloc failed" <<endl;    
    }
    
    while(1)
    {
        
        camera_src_filter->read_frame(yuv420p_data,640*480*3/2);
        mux_source_filter->Put_frame(yuv420p_data,640*480*3/2);        
        
    }
#if 0
    //显示所有支持帧格式
    fmtdesc.index=0;
    fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("Support format:\n");
    while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)!=-1)
    {
        printf("\t%d.%s\n",fmtdesc.index+1,fmtdesc.description);
        fmtdesc.index++;
    }

    //检查是否支持某帧格式
    struct v4l2_format fmt_test;
    fmt_test.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt_test.fmt.pix.pixelformat=V4L2_PIX_FMT_RGB32;
    if(ioctl(fd,VIDIOC_TRY_FMT,&fmt_test)==-1)
    {
        printf("not support format RGB32!\n");      
    }
    else
    {
        printf("support format RGB32\n");
    }

    //检查是否支持某帧格式
    //struct v4l2_format fmt_test;
    fmt_test.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt_test.fmt.pix.pixelformat=V4L2_PIX_FMT_YUYV;
    if(ioctl(fd,VIDIOC_TRY_FMT,&fmt_test)==-1)
    {
        printf("not support format YUYV!\n");      
    }
    else
    {
        printf("support format YUYV\n");
    }

    
    fmt_test.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt_test.fmt.pix.pixelformat=V4L2_PIX_FMT_YUV420M;
    if(ioctl(fd,VIDIOC_TRY_FMT,&fmt_test)==-1)
    {
        printf("not support format YUV420P!\n");      
    }
    else
    {
        printf("support format YUV420P\n");
    }
#endif 

    free(yuv420p_data);
    camera_src_filter->release_devuce();
#if 0
    cout<< "start mux_source_thread_main, thread id : " << this_thread::get_id() << endl;
    if(!capture.isOpened()){
        cout<<"capture open failed"<<endl;
        return ;
    }
    mux_source_filter->pix_fmt=AV_PIX_FMT_YUV420P;
    mux_source_filter->cows=wed;
    mux_source_filter->rows=hight;
    
    cout<<"cows "<<mux_source_filter->cows << "rows "<<mux_source_filter->rows<<endl;
    //
    //writer = VideoWriter(video_name, -1, capture.get(CAP_PROP_FPS), Size(wed,hight), 0);
    //写入的对象，保持原来的帧率和大小。
    //writer.open(video_name,capture.get(CAP_PROP_FOURCC), capture.get(CAP_PROP_FPS), Size(wed, hight), true);  
    //writer.open(video_name, CV_FOURCC('M', 'P', '4', '2'), capture.get(CAP_PROP_FPS),Size(wed, hight), true);  
    //writer.open(video_name, CV_FOURCC('H', '2', '6', '4'), capture.get(CAP_PROP_FPS),Size(wed, hight), true);  
    

    Mat image;
    int index=0;
    unsigned char * yuv420p_data; 

    yuv420p_data=(unsigned char *)malloc(wed*hight*3/2);
    if(yuv420p_data==NULL){
        cout<< "yuv420p_data malloc failed" <<endl;    
    }
    
    //for (unsigned i = 0; i < capture.get(CAP_PROP_FRAME_COUNT); i++)
    for(;;)
    {
        //gettimeofday(&ts,NULL);
        //cout << ts.tv_sec << "  " <<ts.tv_usec << endl;
        capture>>image;

        cout<< " x y :" << image.cols << " " << image.rows << " type "<< image.type() \
            << "channels: "<< image.channels() <<"dims :" << image.dims  <<"size :" << image.size \
            << "elemSize:" <<image.elemSize()<<endl;

        
            
        mat_to_yuv420p(image,yuv420p_data);
        
        //if(index%10 == 0)
            //mux_source_filter->Put_frame(image);
            
            mux_source_filter->Put_frame(yuv420p_data,image.cols*image.rows*3/2);
            //queue_test.push(index);
        //cout << "push " << index << endl;
    }

    image.release();
#endif
}



