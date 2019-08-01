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

#include "../util/CPictureFormatConversion.h"
#include "encoder_src_filter.h"


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


Cmux_source_filter * mux_source_filter;





int simplest_ffmpeg_video_yuv420_to_h264_init(int in_w,int in_h,char * filename_out)
{
    int y_size, ret;
    //Compare<int > cmp1(3,7);

    //printf ("%d",cmp1.max());
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
    //yuyv_to_yuv420p(pCodecCtx->width , pCodecCtx->height,Frame_data_ptr,pFrame->data[0],pFrame->data[1],pFrame->data[2]);
    memcpy(pFrame->data[0],Frame_data_ptr,y_size);
    memcpy(pFrame->data[1],Frame_data_ptr+y_size,y_size/4);
    memcpy(pFrame->data[2],Frame_data_ptr+y_size*5/4,y_size/4);
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

void yuv420pvideo_capture_yuv420p(void)
{
    char filename_in[]="../../simplest_ffmpeg_video_encoder/ds_480x272.yuv";

    char filename_out[]="ds_480x272_image.yuv420";

    //Input raw data
	FILE *fp_in = fopen(filename_in, "rb");
	if (!fp_in) {
		printf("Could not open %s\n", filename_in);
		return ;
	}
	FILE *fp_out = fopen(filename_out, "wb");
	if (!fp_in) {
		printf("Could not open %s\n", filename_out);
		return ;
	}
    
	int y_size =480 * 272;
    
    unsigned char *FrameData=(unsigned char *)malloc(y_size*3/2);
    if(FrameData==NULL){
        printf("Could malloc FrameData LEN %d\n", y_size*3/2);
        return ;
    }
    //read 
    cout << "read file" <<endl;
    fread(FrameData,1,y_size,fp_in);	        // Y
	fread(FrameData+y_size,1,y_size/4,fp_in);  // U
	fread(FrameData+y_size*5/4,1,y_size/4,fp_in);	// V
    //write
    cout<< "write file" <<endl;
    fwrite(FrameData,1,y_size*3/2,fp_out);
            
 
    free(FrameData);
    
    
    
    if(fp_in!=NULL)
        fclose(fp_in);
    if(filename_out!=NULL)
        fclose(fp_out);

}

int simplest_ffmpeg_h264_encoder(char * filename_in,char * filename_out)
{
    int i, ret, got_output;
    FILE *fp_in;
	//FILE *fp_out;
	int y_size;
	//int framecnt=0;

	

    //yuv420pvideo_capture_yuv420p();
    //return 1;

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
    frame_data=(unsigned char *)malloc(mux_source_filter->cows*mux_source_filter->rows*3/2);
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
    delete mux_source_filter;

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
