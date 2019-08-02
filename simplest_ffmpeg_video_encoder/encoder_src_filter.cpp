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
        printf ("mux_source_filter::Put_frame  data len%d\n",data_len);
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
    

    camera_src_filter->open_device();
    
    camera_src_filter->init_device();
    camera_src_filter->start_capturing();

 
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
        camera_src_filter->read_frame(yuv420p_data);
        //memset (yuv420p_data,21,640*480*3/2);
        mux_source_filter->Put_frame(yuv420p_data,640*480*3/2);      
        
    }


    free(yuv420p_data);
    camera_src_filter->start_capturing();
    camera_src_filter->uninit_device();
    camera_src_filter->close_device();

}



