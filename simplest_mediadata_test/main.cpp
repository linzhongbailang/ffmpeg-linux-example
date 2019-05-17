
//--------------------------------------【程序说明】-------------------------------------------
//		程序说明：《OpenCV3编程入门》OpenCV3版书本配套示例程序06
//		程序描述：使用VideoCapture类进行视频读取和播放
//		开发测试所用操作系统： Windows 7 64bit
//		开发测试所用IDE版本：Visual Studio 2010
//		开发测试所用OpenCV版本：	3.0 beta
//		2014年11月 Created by @浅墨_毛星云
//		2014年11月 Revised by @浅墨_毛星云
//		2015年11月 Second Revised by @浅墨_毛星云
//------------------------------------------------------------------------------------------------


//---------------------------------【头文件、命名空间包含部分】----------------------------
//		描述：包含程序所使用的头文件和命名空间
//-------------------------------------------------------------------------------------------------
#include <opencv2/opencv.hpp> 
#include <stdlib.h>
#include <iomanip>
#include<stdio.h>  
#include<stdlib.h>  
#include<string.h>  
#include<errno.h>  
#include<sys/types.h>  
#include<sys/socket.h>  
#include<netinet/in.h>  

using namespace cv;
using namespace std;



int simplest_h264_parser(char *url);
int simplest_udp_parser(int port);
int simplest_flv_parser(char *in_url,char * vidoe_out_url,char * audio_out_urtl);


void CameraImage_To_Video(void)   //输入应该是起始和结束帧以及原视频对象
{
    string video_name = "Camera_H264.mp4";   //视频名字
    VideoWriter writer;  
    VideoCapture capture(0);
    //VideoCapture capture("../1.avi");
    unsigned int i=0;
    int hight = capture.get(CAP_PROP_FRAME_HEIGHT);   //宽和高保持不变
    int wed = capture.get(CAP_PROP_FRAME_WIDTH);
    char Fourcc[4];
    unsigned int Fourcc_32;
    
        

	cout<<"fps "<<setw(5)<<capture.get(CAP_PROP_FPS)<<endl;
    Fourcc_32=capture.get(CAP_PROP_FOURCC);
    Fourcc[0]=Fourcc_32;
    Fourcc[1]=Fourcc_32>>8;
    Fourcc[2]=Fourcc_32>>16;
    Fourcc[3]=Fourcc_32>>24;
    cout<<"four character code  "<<Fourcc[0]<<Fourcc[1]<<Fourcc[2]<<Fourcc[3]<<endl;
			
    if(!capture.isOpened()){
        cout<<"capture open failed"<<endl;
        return ;
    }

    //
    //writer = VideoWriter(video_name, -1, capture.get(CAP_PROP_FPS), Size(wed,hight), 0);
    //写入的对象，保持原来的帧率和大小。
    //writer.open(video_name,capture.get(CAP_PROP_FOURCC), capture.get(CAP_PROP_FPS), Size(wed, hight), true);  
    //writer.open(video_name, CV_FOURCC('M', 'P', '4', '2'), capture.get(CAP_PROP_FPS),Size(wed, hight), true);  
    writer.open(video_name, CV_FOURCC('H', '2', '6', '4'), capture.get(CAP_PROP_FPS),Size(wed, hight), true);  
    

    Mat image;
    //for (unsigned i = 0; i < capture.get(CAP_PROP_FRAME_COUNT); i++)
    for(;;)
    {
        cout << i++<< "  " <<getTickCount() << endl;
        capture>>image;
        //imshow("test", image);
        if (waitKey(10)=='q' || image.empty() || i>300)
        {
            cout << "transform task was done!" << endl;
            break;
        } 
        writer<<image;
              
    }
}

//-----------------------------------【main( )函数】--------------------------------------------
//		描述：控制台应用程序的入口函数，我们的程序从这里开始
//-------------------------------------------------------------------------------------------------
int main(int argc, char** argv )  
{  
	cout << argv[0]<<" start" << endl;

    //simplest_h264_parser(argv[1]);
    simplest_udp_parser(atoi (argv[1]));
    //simplest_flv_parser(argv[1]);
	return 0;     
}  
