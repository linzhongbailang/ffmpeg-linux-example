
//--------------------------------------������˵����-------------------------------------------
//		����˵������OpenCV3������š�OpenCV3���鱾����ʾ������06
//		����������ʹ��VideoCapture�������Ƶ��ȡ�Ͳ���
//		�����������ò���ϵͳ�� Windows 7 64bit
//		������������IDE�汾��Visual Studio 2010
//		������������OpenCV�汾��	3.0 beta
//		2014��11�� Created by @ǳī_ë����
//		2014��11�� Revised by @ǳī_ë����
//		2015��11�� Second Revised by @ǳī_ë����
//------------------------------------------------------------------------------------------------


//---------------------------------��ͷ�ļ��������ռ�������֡�----------------------------
//		����������������ʹ�õ�ͷ�ļ��������ռ�
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


void CameraImage_To_Video(void)   //����Ӧ������ʼ�ͽ���֡�Լ�ԭ��Ƶ����
{
    string video_name = "Camera_H264.mp4";   //��Ƶ����
    VideoWriter writer;  
    VideoCapture capture(0);
    //VideoCapture capture("../1.avi");
    unsigned int i=0;
    int hight = capture.get(CAP_PROP_FRAME_HEIGHT);   //��͸߱��ֲ���
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
    //д��Ķ��󣬱���ԭ����֡�ʺʹ�С��
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

//-----------------------------------��main( )������--------------------------------------------
//		����������̨Ӧ�ó������ں��������ǵĳ�������￪ʼ
//-------------------------------------------------------------------------------------------------
int main(int argc, char** argv )  
{  
	cout << argv[0]<<" start" << endl;

    //simplest_h264_parser(argv[1]);
    simplest_udp_parser(atoi (argv[1]));
    //simplest_flv_parser(argv[1]);
	return 0;     
}  
