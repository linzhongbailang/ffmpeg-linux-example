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

int simplest_ffmpeg_video_yuv420_to_h264_init(int in_w,int in_h,char * filename_out)
{
    int y_size, ret;

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

int simplest_ffmpeg_video_yuv420_to_h264(unsigned char * Frame_data_ptr)
{
    int got_output,ret;   
    int y_size = pCodecCtx->width * pCodecCtx->height;
     
    av_init_packet(&pkt);
    pkt.data = NULL;    // packet data will be allocated by the encoder
    pkt.size = 0;

    memcpy(pFrame->data[0],Frame_data_ptr,y_size);                  //y
    memcpy(pFrame->data[1],Frame_data_ptr+y_size,y_size/4);         //u
    memcpy(pFrame->data[2],Frame_data_ptr+y_size*5/4,y_size/4);     //v
    
    Frame_index++;
    
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
            fwrite(pkt.data, 1, pkt.size, fp_out);
            av_free_packet(&pkt);
        }
    }
    
    fclose(fp_out);
    avcodec_close(pCodecCtx);
    av_free(pCodecCtx);
    av_freep(&pFrame->data[0]);
    av_frame_free(&pFrame);
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
    unsigned char * FrameData;

    simplest_ffmpeg_video_yuv420_to_h264_init(in_w,in_h,filename_out);

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
}

}
