#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
//#include <rtscamkit.h>
//#include <rtsavapi.h>
//#include <rtsvideo.h>
#include <getopt.h>

#include "rtspservice.h"
#include "rtputils.h"
#include "ringfifo.h"

#define TAG "gree-rtsp"
#define false 0
#define true 1
extern int g_s32Quit;
int h264flag = -1;
int isp = -1;

#ifdef COMPILER_DEBUG
int Camera_h264_init(void){
    struct rts_av_profile profile;
	struct rts_isp_attr isp_attr;
	struct rts_h264_attr h264_attr;
	struct rts_video_h264_ctrl *ctrl = NULL;
	int ret;

	ret = rts_av_init();
	if (ret) {
		printf("rts_av_init fail \n");
		RTS_ERR("rts_av_init fail\n");
		return ret;
	}
	//printf("rts_av_init() OK\n");

	isp_attr.isp_id = 0;
	isp_attr.isp_buf_num = 3;//isp缓存数目
	isp = rts_av_create_isp_chn(&isp_attr);
	
	if (isp < 0) {
		printf("### rts_av_create_isp_chn fail ###\n");
		RTS_ERR("fail to create isp chn, ret = %d\n", isp);
		printf("fail to create isp chn, ret = %d\n", isp);
		ret = RTS_RETURN(RTS_E_OPEN_FAIL);
		RTS_SAFE_RELEASE(ctrl, rts_av_release_h264_ctrl);

	    if (isp >= 0) {
		 rts_av_destroy_chn(isp);
		 isp = -1;
	    }
	    if (h264flag >= 0) {
		 rts_av_destroy_chn(h264flag);
		 h264flag = -1;
	    }
	   rts_av_release();
		return ret;
	}
    //printf("rts_av_create_isp_chn() OK isp chnno :%d\n",isp);
	RTS_INFO("isp chnno:%d\n", isp);

	RTS_INFO("isp chnno:%d\n", isp);
	profile.fmt = RTS_V_FMT_YUV420SEMIPLANAR;
	profile.video.width = 1280;
	profile.video.height = 720;
	profile.video.numerator = 1;
	profile.video.denominator = 25;
	ret = rts_av_set_profile(isp, &profile);
	if (ret) {
		printf("### rts_av_set_profile fail ###\n");
		RTS_ERR("gh://set isp profile fail, ret = %d\n", ret);
		printf("set isp profile fail, ret = %d\n", ret);
		RTS_SAFE_RELEASE(ctrl, rts_av_release_h264_ctrl);
	    if (isp >= 0) {
		 rts_av_destroy_chn(isp);
		 isp = -1;
	    }
	    if (h264flag >= 0) {
		 rts_av_destroy_chn(h264flag);
		 h264flag = -1;
	    }
	   rts_av_release();
	   return ret;
	}
      //printf("rts_av_set_profile() OK\n");
	h264_attr.level = H264_LEVEL_4;
	h264_attr.qp = -1;
	h264_attr.bps = 2 * 1024 * 1024;
	h264_attr.gop = 48;
	h264_attr.videostab = 0;
	h264_attr.rotation = rts_av_rotation_0;//旋转
	h264flag = rts_av_create_h264_chn(&h264_attr);
	if (h264flag < 0) {
		printf("### rts_av_create_h264_chn faill ###\n");
		RTS_ERR("fail to create h264 chn, ret = %d\n", h264flag);
		ret = RTS_RETURN(RTS_E_OPEN_FAIL);
		RTS_SAFE_RELEASE(ctrl, rts_av_release_h264_ctrl);
	    if (isp >= 0) {
		 rts_av_destroy_chn(isp);
		 isp = -1;
	    }
	    if (h264flag >= 0) {
		 rts_av_destroy_chn(h264flag);
		 h264flag = -1;
	    }
	   rts_av_release();
	   return ret;
	}
	//printf("rts_av_create_h264_chn() OK\n");
	RTS_INFO("h264 chnno:%d\n", h264flag);

	ret = rts_av_query_h264_ctrl(h264flag, &ctrl);//分配存储空间并获�?H264 支持的初始控制属�?
	if (ret) {
		RTS_ERR("query h264 ctrl fail, ret = %d\n", ret);
		return ret;
	}
       printf("rts_av_query_h264_ctrl() OK\n");
	
	//ctrl->intra_qp_delta = 0;
	ctrl->bitrate_mode = RTS_BITRATE_MODE_C_VBR;
	ctrl->bitrate = 1024*1024*1.2;
	ctrl->max_bitrate = 1024*1024*1.8;
	ctrl->min_bitrate = 1024*1024*0.8;
	ctrl->max_qp = 25;
	ctrl->min_qp = 10;

	ret = rts_av_set_h264_ctrl(ctrl);//设置 H264 的控制属�?
	
	if (ret) {
		RTS_ERR("set h264 ctrl fail, ret = %d\n", ret);
		RTS_SAFE_RELEASE(ctrl, rts_av_release_h264_ctrl);
	    if (isp >= 0) {
		 rts_av_destroy_chn(isp);
		 isp = -1;
	    }
	    if (h264flag >= 0) {
		 rts_av_destroy_chn(h264flag);
		 h264flag = -1;
	    }
	   rts_av_release();
	   return ret;
	}
        //printf("rts_av_set_h264_ctrl() OK\n");
	
	ret = rts_av_bind(isp, h264flag);
	if (ret) {
		printf("### rts_av_bind ###\n");
		RTS_ERR("fail to bind isp and h264, ret %d\n", ret);
				RTS_SAFE_RELEASE(ctrl, rts_av_release_h264_ctrl);
	    if (isp >= 0) {
		 rts_av_destroy_chn(isp);
		 isp = -1;
	    }
	    if (h264flag >= 0) {
		 rts_av_destroy_chn(h264flag);
		 h264flag = -1;
	    }
	   rts_av_release();
	   return ret;
	}
	//printf("rts_av_bind() OK\n");

	ret = rts_av_enable_chn(isp);
	if (ret) {
		printf("### rts_av_enable_chn isp fail ###\n");
		RTS_ERR("enable isp fail, ret = %d\n", ret);
				RTS_SAFE_RELEASE(ctrl, rts_av_release_h264_ctrl);
	    if (isp >= 0) {
		 rts_av_destroy_chn(isp);
		 isp = -1;
	    }
	    if (h264flag >= 0) {
		 rts_av_destroy_chn(h264flag);
		 h264flag = -1;
	    }
	   rts_av_release();
	   return ret;
	}
        //printf("rts_av_enable_chn() OK\n");
   
	ret = rts_av_enable_chn(h264flag);
	if (ret) {
		printf("### rts_av_enable_chn h264flag fail ###\n");
		RTS_ERR("enable h264 fail, ret = %d\n", ret);
		RTS_SAFE_RELEASE(ctrl, rts_av_release_h264_ctrl);
	    if (isp >= 0) {
		 rts_av_destroy_chn(isp);
		 isp = -1;
	    }
	    if (h264flag >= 0) {
		 rts_av_destroy_chn(h264flag);
		 h264flag = -1;
	    }
	   rts_av_release();
	   return ret;
	}
	//printf("rts_av_enable_chn() OK\n");
	
	ret = rts_av_start_recv(h264flag);//Channel开始接收数�?
	if (ret) {
		printf("### rts_av_start_recv  faill ###\n");
		RTS_ERR("start recv h264 fail, ret = %d\n", ret);
		RTS_SAFE_RELEASE(ctrl, rts_av_release_h264_ctrl);
	    if (isp >= 0) {
		 rts_av_destroy_chn(isp);
		 isp = -1;
	    }
	    if (h264flag >= 0) {
		 rts_av_destroy_chn(h264flag);
		 h264flag = -1;
	    }
	   rts_av_release();
	   return ret;
	}
    printf("Camera init h264flag = %d , isp = %d \n",h264flag,isp);
	//printf("rts_av_start_recv() OK\n");

	return true;
}
#endif

/*void* get_frame_thread(){
  struct rts_av_buffer *pdata=NULL;
  usleep(1000);
   printf("get_frame_thread start !!! h264flag = %d \n",h264flag);
  while(!g_s32Quit){
   if (rts_av_poll(h264flag)){//检�?Channel 中是否有数据可读
    printf("no data in channel \n");
    continue;
   }
			
   if (rts_av_recv(h264flag, &pdata)){//接收数据放入buffer
    printf("get data in buffer file \n");
	continue;
   }

   	PutH264DataToBuffer(pdata->vm_addr,pdata->bytesused);
    rts_av_put_buffer(pdata);
  }
  return 0;
}*/

#ifdef COMPILER_DEBUG
void Camera_free(void){
    rts_av_disable_chn(isp);
    rts_av_disable_chn(h264flag);
	rts_av_release();
}
#endif
/*void GetandSaveFrame(void){
   int ret;
   struct rts_av_buffer *pdata=NULL;
   Camera_h264_init();
   printf("H264 init finish h264flag = %d isp = %d!!\n",h264flag,isp);
   
  while(!g_s32Quit){
  	usleep(1000);
	ret = rts_av_poll(h264flag);
    if (ret){//检�?Channel 中是否有数据可读
     printf("no data in channel  ret = %d\n",ret);
     continue;
    }
			
    if (rts_av_recv(h264flag, &pdata)){//接收数据放入buffer
    printf("get data in buffer file \n");
	continue;
    }

   	 PutH264DataToBuffer(pdata->vm_addr,pdata->bytesused);
     rts_av_put_buffer(pdata);
  }
}*/

/*void * EvenLoop_Thread(void * arg){
    int s32MainFd = *(int *)arg;
	struct timespec ts = { 0, 200000000 };
	printf("s32MainFd = %d\n",s32MainFd);
   	while (!g_s32Quit)
	{
		nanosleep(&ts, NULL);
		EventLoop(s32MainFd);
	}
	return 0;
}*/

void * FRAME_SOURCE_THREAD(){
   int ret;
#ifdef COMPILER_DEBUG
   Camera_h264_init();
#endif
   printf("H264 init finish h264flag = %d isp = %d!!\n",h264flag,isp);
   
  while(!g_s32Quit){
#ifdef COMPILER_DEBUG
  	    struct rts_av_buffer *pdata=NULL;
#else
        unsigned char *pData=NULL;
        int DataLen=0;
#endif
  	    usleep(1000);//50000

#ifdef COMPILER_DEBUG
	    ret = rts_av_poll(h264flag);
        if (ret){//检�?Channel 中是否有数据可读
            //printf("no data in channel  ret = %d\n",ret);
            continue;
        }
        if (rts_av_recv(h264flag, &pdata)){//接收数据放入buffer
            printf("get data in buffer file \n");
    	    continue;
        }
#endif
	    //printf("get data success !!!\n");

#ifdef COMPILER_DEBUG
   	    PutH264DataToBuffer(pdata->vm_addr,pdata->bytesused);
#else
        PutH264DataToBuffer(pData,DataLen);
#endif

#ifdef COMPILER_DEBUG
        rts_av_put_buffer(pdata);
#endif
  }
  return 0;
}


int simplest_ffmpeg_rtsp_server(void)
{
    int s32MainFd;
	pthread_t framesource_id;
	struct timespec ts = { 0, 200000000 };
	ringmalloc(720*576);//64个大小为720*576的环形缓冲区
	printf("RTSP server START\n");
	PrefsInit();
	printf("listen for client connecting...\n");
	signal(SIGINT, IntHandl);
	s32MainFd = tcp_listen(SERVER_RTSP_PORT_DEFAULT);//554
	printf(" funtion tcp_listen finish !!\n");
	if (ScheduleInit() == ERR_FATAL)
	{
		fprintf(stderr,"Fatal: Can't start scheduler %s, %i \nServer is aborting.\n", __FILE__, __LINE__);
		return 0;
	}
	RTP_port_pool_init(RTP_DEFAULT_PORT);
	printf(" funtion RTP_port_pool_init finish !!\n");
	pthread_create(&framesource_id,NULL,FRAME_SOURCE_THREAD,NULL);
	//pthread_create(&framesource_id,NULL,EvenLoop_Thread,&s32MainFd);
	//GetandSaveFrame();
	while (!g_s32Quit)
	{
		nanosleep(&ts, NULL);
		EventLoop(s32MainFd);
	}
	sleep(2);
#ifdef COMPILER_DEBUG
	Camera_free();
#endif
	ringfree();
	printf("The Server quit!\n");

	return 0;

}
