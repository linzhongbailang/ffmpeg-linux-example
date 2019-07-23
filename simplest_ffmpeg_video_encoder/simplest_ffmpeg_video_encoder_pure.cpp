/**
 * 最简单的基于FFmpeg的视频编码器（纯净版）
 * Simplest FFmpeg Video Encoder Pure
 * 
 * 雷霄骅 Lei Xiaohua
 * leixiaohua1020@126.com
 * 中国传媒大学/数字电视技术
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 * 
 * 本程序实现了YUV像素数据编码为视频码流（H264，MPEG2，VP8等等）。
 * 它仅仅使用了libavcodec（而没有使用libavformat）。
 * 是最简单的FFmpeg视频编码方面的教程。
 * 通过学习本例子可以了解FFmpeg的编码流程。
 * This software encode YUV420P data to video bitstream
 * (Such as H.264, H.265, VP8, MPEG2 etc).
 * It only uses libavcodec to encode video (without libavformat)
 * It's the simplest video encoding software based on FFmpeg. 
 * Suitable for beginner of FFmpeg 
 */


#include <stdio.h>
#include <iostream>
#include <queue>
#include <thread>
#include <mutex>          // std::mutex, std::lock_guard

#include <opencv2/opencv.hpp> 

#include <time.h>
#include <sys/time.h>



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

//test different codec
#define TEST_H264  1
#define TEST_HEVC  0

AVCodec *pCodec;
AVCodecContext *pCodecCtx= NULL;
AVPacket pkt;
AVFrame *pFrame;
int framecnt;
int Frame_index;
FILE *fp_out;


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



Cmux_source_filter::Cmux_source_filter(void)
{
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
    if(pix_fmt==AV_PIX_FMT_YUYV422)
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



Cmux_source_filter * mux_source_filter;
void mux_source_thread_main()
{
    VideoCapture capture(0);
    //VideoCapture capture("../1.avi");
    unsigned int i=0;
    int hight = capture.get(CAP_PROP_FRAME_HEIGHT);   //宽和高保持不变
    int wed = capture.get(CAP_PROP_FRAME_WIDTH);
    char Fourcc[4];
    unsigned int Fourcc_32;
    
    timeval ts;
    //capture.set(CAP_PROP_FOURCC,CV_CAP_MODE_GRAY);
    cout<< "start mux_source_thread_main, thread id : " << this_thread::get_id() << endl;
        
    

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
    mux_source_filter->pix_fmt=AV_PIX_FMT_YUYV422;
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
    
    //for (unsigned i = 0; i < capture.get(CAP_PROP_FRAME_COUNT); i++)
    for(;;)
    {
        //gettimeofday(&ts,NULL);
        //cout << ts.tv_sec << "  " <<ts.tv_usec << endl;
        capture>>image;

        cout<< " x y :" << image.cols << " " << image.rows << " type "<< image.type() \
            << "channels: "<< image.channels() <<"dims :" << image.dims  <<"size :" << image.size \
            << "elemSize:" <<image.elemSize()<<endl;

        
            
        index++;
        if(index==30)
        {   
            FILE *fp_yuv444p;
            FILE *fp_yuv420p;
            int index_y = 0;  
            int index_u = 0;  
            int index_v = 0; 
            unsigned char * yuv444p_data;
            unsigned char * yuv420p_data;
            unsigned char * y_data;
            unsigned char * u_data;
            unsigned char * v_data;
            int number_of_uchar=0;
            int data_length;
            uchar* data=image.ptr<uchar>(0);
            
            yuv444p_data=(unsigned char *)malloc(wed*hight*3);
            if(yuv444p_data==NULL){
                cout<< "yuv422_data malloc failed" <<endl;    
                break;
            }
            if(image.isContinuous())
                cout << " image is continuous"<<endl;

            
            
            //save yuv444p
            memset(yuv444p_data,0,wed*hight*3);
            y_data=yuv444p_data;
            u_data=yuv444p_data+wed*hight;
            v_data=yuv444p_data+wed*hight*2;
            cout<< "save the 30th yuv444p image" <<endl;
            //Output bitstream
        	fp_yuv444p = fopen("yuv444p_30th_1.yuv", "wb");
        	if (!fp_out) {
        		printf("Could not open %s\n", "yuv444p_30th_1.yuv");
           	}
            else
            {
                data_length=image.rows*image.cols*image.channels();
                number_of_uchar=image.elemSize();
                if(image.isContinuous())
                    cout << "image isContinuous +++++++++++++++++++++++++++++++++++++++++++" <<endl;
                for (int i = 0; i< data_length; i = i + number_of_uchar)
                {
                    //data[i] = 255;
                    //data[i+1] = 0;
                    //data[i+2] = 0;//这里我们认为图像是彩色的三通道，所以对每个像素的BGR通道分别进行单独操作
                    *(y_data+ (index_y++)) = data[i];
                    *(u_data + (index_u++)) = data[i+1]; 
                    *(v_data + (index_v++)) = data[i+2];  
                }

                //memcpy(yuv422_data,data,wed*hight*3);
                fwrite(yuv444p_data, 1, wed*hight*3, fp_yuv444p);
                //fwrite(image.data+wed*hight, 1, wed*hight/2, fp_yuv420p);
                //fwrite(image.data+wed*hight*3/2, 1, wed*hight/2, fp_yuv420p);
                fclose(fp_yuv444p);

            }

            //save yuv420p
            yuv420p_data=(unsigned char *)malloc(wed*hight*3/2);
            if(yuv420p_data==NULL){
                cout<< "yuv420p_data malloc failed" <<endl;    
                break;
            }
            memset(yuv420p_data,0,wed*hight*3/2);
            y_data=yuv420p_data;
            u_data=yuv420p_data+wed*hight;
            v_data=yuv420p_data+wed*hight*5/4;
            index_y = 0;  
            index_u = 0;  
            index_v = 0; 
            cout<< "save the 30th yuv420 image" <<endl;
            //Output bitstream
        	fp_yuv420p = fopen("yuv420p_30th_1.yuv", "wb");
        	if (!fp_out) {
        		printf("Could not open %s\n", "yuv420p_30th.yuv");
           	}
            else
            {
                data_length=image.rows*image.cols*image.channels();
                number_of_uchar=image.elemSize();
                if(image.isContinuous())
                    cout << "image isContinuous +++++++++++++++++++++++++++++++++++++++++++" <<endl;
                for (int i = 0; i< data_length; i = i + number_of_uchar)
                {
                    //data[i] = 255;
                    //data[i+1] = 0;
                    //data[i+2] = 0;//这里我们认为图像是彩色的三通道，所以对每个像素的BGR通道分别进行单独操作
                    *(y_data+ (index_y++)) = data[i];
                    if(i/number_of_uchar/image.cols%2==0)
                    {
                        if(i/number_of_uchar%image.cols%2==0)
                        {
                            *(u_data + (index_u++)) = data[i+1]; 
                            *(v_data + (index_v++)) = data[i+2];
                        }
                    }
                }

                //memcpy(yuv422_data,data,wed*hight*3);
                fwrite(yuv420p_data, 1, wed*hight*3/2, fp_yuv420p);
                //fwrite(image.data+wed*hight, 1, wed*hight/2, fp_yuv420p);
                //fwrite(image.data+wed*hight*3/2, 1, wed*hight/2, fp_yuv420p);
                fclose(fp_yuv420p);

            }

    

        }
        
        //if(index%10 == 0)
            //mux_source_filter->Put_frame(image);
            mux_source_filter->Put_frame(image.data,image.cols*image.rows*2);
            //queue_test.push(index);
        //cout << "push " << index << endl;
    }

    image.release();
}

int simplest_ffmpeg_video_yuv420_to_h264_init(int in_w,int in_h,char * filename_out)
{
    int y_size, ret;
    Compare<int > cmp1(3,7);

    printf ("%d",cmp1.max());
    framecnt=0;
    Frame_index=0;
    avcodec_register_all();
    
    pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);

    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx) {
        printf("Could not allocate video codec context\n");
        return -1;
    }
    pCodecCtx->bit_rate = 400000;
    pCodecCtx->width = in_w;
    pCodecCtx->height = in_h;
    pCodecCtx->time_base.num=1;
	pCodecCtx->time_base.den=25;
    pCodecCtx->gop_size = 10;
    pCodecCtx->max_b_frames = 1;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    
    av_opt_set(pCodecCtx->priv_data, "preset", "slow", 0);
    
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec\n");
        return -1;
    }

    pFrame = av_frame_alloc();
    if (!pFrame) {
        printf("Could not allocate video frame\n");
        return -1;
    }
    pFrame->format = pCodecCtx->pix_fmt;
    pFrame->width  = pCodecCtx->width;
    pFrame->height = pCodecCtx->height;

    
    ret = av_image_alloc(pFrame->data, pFrame->linesize, pCodecCtx->width, pCodecCtx->height,
                         pCodecCtx->pix_fmt, 16);
    if (ret < 0) {
        printf("Could not allocate raw picture buffer\n");
        return -1;
    }

    
    //Output bitstream
	fp_out = fopen(filename_out, "wb");
	if (!fp_out) {
		printf("Could not open %s\n", filename_out);
		return -1;
	}
    
}

int yuyv_to_yuv420p(int width,int height,unsigned char * yuyv_data,unsigned char * y_data,unsigned char * u_data,unsigned char * v_data)
{
    int num = width*height * 2 - 4;  
    int index_y = 0;  
    int index_u = 0;  
    int index_v = 0;  
    //unsigned char * y_data=yuv420p_data;
    //unsigned char * u_data=yuv420p_data+width*height;
    //unsigned char * v_data=yuv420p_data+width*height*5/4;
    cout << "yuyv_to_yuv420p x y "<< width << " x "<< height <<endl;

#if 0
    for(int i = 0; i < num; i = i+4)  
    {  
        *(y_data+ (index_y++)) = *(yuyv_data + i);
        //if(i/(width*4)%2==0)
         //   *(u_data+ (index_u++)) = *(yuyv_data + i + 1);   
        
        *(y_data+ (index_y++)) = *(yuyv_data + i + 2); 
        //if(i/(width*4)%2==0)
         //   *(v_data + (index_v++)) = *(yuyv_data + i + 3); 
    }  
#else
    memcpy(y_data,yuyv_data,width*height);
    //memcpy(u_data,yuyv_data+width*height,width*height/4);
    //memcpy(v_data,yuyv_data+width*height*5/4,width*height/4);
#endif
    return 1;

}
int simplest_ffmpeg_video_yuv420_to_h264(unsigned char * Frame_data_ptr)
{
    int got_output,ret;   
    int y_size = pCodecCtx->width * pCodecCtx->height;

    cout<<"simplest_ffmpeg_video_yuv420_to_h264 enter"<<endl;
    av_init_packet(&pkt);
    pkt.data = NULL;    // packet data will be allocated by the encoder
    pkt.size = 0;
    yuyv_to_yuv420p(pCodecCtx->width , pCodecCtx->height,Frame_data_ptr,pFrame->data[0],pFrame->data[1],pFrame->data[2]);
  
    Frame_index++;

    //if(Frame_index==30)
    if(0)
    {
        
        FILE *fp_yuv420p;
        cout<< "save the 30th yuv420 image" <<endl;
        //Output bitstream
    	fp_yuv420p = fopen("yuv420p_30th.yuv", "wb");
    	if (!fp_out) {
    		printf("Could not open %s\n", "yuv420p_30th.yuv");
       	}
        else
        {
            fwrite(pFrame->data[0], 1, pCodecCtx->width*pCodecCtx->height, fp_yuv420p);
            fwrite(pFrame->data[1], 1, pCodecCtx->width*pCodecCtx->height/4, fp_yuv420p);
            fwrite(pFrame->data[2], 1, pCodecCtx->width*pCodecCtx->height/4, fp_yuv420p);
            fclose(fp_yuv420p);

        }
            

    }
    
    pFrame->pts = Frame_index;
    /* encode the image */
    ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_output);
    if (ret < 0) {
        printf("Error encoding frame\n");
        return -1;
    }
    
    if (got_output) {
        printf("Succeed to encode frame: %5d\tsize:%5d,frame index %d\n",framecnt,pkt.size,Frame_index);
        framecnt++;
        fwrite(pkt.data, 1, pkt.size, fp_out);
        av_free_packet(&pkt);
    }
}

int simplest_ffmpeg_video_yuv420_to_h264_exit(void)
{
    int i, ret, got_output;
    
    //Flush Encoder
    for (got_output = 1; got_output; i++) {
        ret = avcodec_encode_video2(pCodecCtx, &pkt, NULL, &got_output);
        if (ret < 0) {
            printf("Error encoding frame\n");
            return -1;
        }
        if (got_output) {
            printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n",pkt.size);
            //fwrite(pkt.data, 1, pkt.size, fp_out);
            av_free_packet(&pkt);
        }
    }
    
    fclose(fp_out);
    avcodec_close(pCodecCtx);
    av_free(pCodecCtx);
    av_freep(&pFrame->data[0]);
    av_frame_free(&pFrame);
    //delete mux_source_filter_ptr;
}



extern "C"
{

int simplest_ffmpeg_h264_encoder(char * filename_in,char * filename_out)
{
    int i, ret, got_output;
    FILE *fp_in;
	//FILE *fp_out;
	int y_size;
	//int framecnt=0;

	//char filename_in[]="../../simplest_ffmpeg_video_encoder/ds_480x272.yuv";

    //char filename_out[]="ds_1.h264";



	int in_w=480,in_h=272;	
	int framenum=100;	
    unsigned char * frame_data;

    cout<< "start main thread, thread id : " << this_thread::get_id() << endl;
    mux_source_filter = new Cmux_source_filter();
    
    thread mux_source_thread(mux_source_thread_main);
    
    do{
    
        if(!(mux_source_filter->Empty())){
            cout << " get source frame"<< endl;
            break;
        }
        //if(!(queue_test.empty()))
        //{
        //    i = queue_test.front();
        //    queue_test.pop();
        //    cout << "pop " << i <<endl;
        //}
        this_thread::sleep_for(chrono:: milliseconds (1)); 

    }while(1);

    
    simplest_ffmpeg_video_yuv420_to_h264_init(mux_source_filter->cows,mux_source_filter->rows,filename_out);
    frame_data=(unsigned char *)malloc(mux_source_filter->cows*mux_source_filter->rows*2);
    if(frame_data==NULL){

    
        cout<< "frame_data malloc failed" <<endl;    
        return 0;
    }
    //for (i = 0; i < framenum; i++) 
    for (;;) 
    {
        //this_thread::sleep_for(chrono::seconds(2));
        if(!(mux_source_filter->Empty())){
            
            mux_source_filter->Get_frame(frame_data);
            simplest_ffmpeg_video_yuv420_to_h264(frame_data);
            //cout<< "mux_source_filter frame : "<<mux_source_filter->frame_num() << endl;
        }
        else
            this_thread::sleep_for(chrono:: milliseconds (1));
    }
    
    
    
    
    

    free(frame_data) ;


    simplest_ffmpeg_video_yuv420_to_h264_exit();

    return 1;
#if 0
	//Input raw data
	fp_in = fopen(filename_in, "rb");
	if (!fp_in) {
		printf("Could not open %s\n", filename_in);
		return -1;
	}
	

	y_size =in_w * in_h;
    //Encode
    FrameData=(unsigned char *)malloc(y_size*3/2);
    if(FrameData==NULL){
        printf("Could malloc FrameData LEN %d\n", y_size*3/2);
        return -1;
    }
    for (i = 0; i < framenum; i++) {
		//Read raw YUV data
		if (fread(FrameData,1,y_size,fp_in)<= 0||		        // Y
			fread(FrameData+y_size,1,y_size/4,fp_in)<= 0||	    // U
			fread(FrameData+y_size*5/4,1,y_size/4,fp_in)<= 0){	// V
			return -1;
		}else if(feof(fp_in)){
			break;
		}

        simplest_ffmpeg_video_yuv420_to_h264(FrameData);
    }
    free(FrameData);
    
    simplest_ffmpeg_video_yuv420_to_h264_exit();

    if(fp_in!=NULL)
        fclose(fp_in);
	return 0;
#endif
}

}
