#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>

#include "rtspservice.h"
#include "rtputils.h"
#include "rtsputils.h"

#define BigLittleSwap32(x) \
     ((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >>  8) |	      \
      (((x) & 0x0000ff00u) <<  8) | (((x) & 0x000000ffu) << 24))

#define BigLittleSwap16(x) \
     ((((x) >> 8) & 0xffu) | (((x) & 0xffu) << 8))
 #define bzero(s,n) (memset((s), '\0', (n)), (void) 0)
     
#ifdef __cplusplus
extern "C" {
#endif
//#define SAVE_NALU 1
typedef struct
{
    /**//* byte 0 */
    unsigned char u4CSrcLen:4;      /**//* expect 0 */
    unsigned char u1Externsion:1;   /**//* expect 1, see RTP_OP below */
    unsigned char u1Padding:1;      /**//* expect 0 */
    unsigned char u2Version:2;      /**//* expect 2 */
    /**//* byte 1 */
    unsigned char u7Payload:7;      /**//* RTP_PAYLOAD_RTSP */
    unsigned char u1Marker:1;       /**//* expect 1 */
    /**//* bytes 2, 3 */
    unsigned short u16SeqNum;
    /**//* bytes 4-7 */
    unsigned long u32TimeStamp;
    /**//* bytes 8-11 */
    unsigned long u32SSrc;          /**//* stream number is used here. */
} StRtpFixedHdr;

typedef struct
{
    //byte 0
    unsigned char u5Type:5;
    unsigned char u2Nri:2;
    unsigned char u1F:1;
} StNaluHdr; /**/ /* 1 BYTES */

typedef struct
{
    //byte 0
    unsigned char u5Type:5;
    unsigned char u2Nri:2;
    unsigned char u1F:1;
} StFuIndicator; /**/ /* 1 BYTES */

typedef struct
{
    //byte 0
    unsigned char u5Type:5;
    unsigned char u1R:1;
    unsigned char u1E:1;
    unsigned char u1S:1;
} StFuHdr; /**/ /* 1 BYTES */

typedef struct _tagStRtpHandle
{
    int                 s32Sock;
    struct sockaddr_in  stServAddr;
    unsigned short      u16SeqNum;
    unsigned long long        u32TimeStampInc;
    unsigned long long        u32TimeStampCurr;
    unsigned long long      u32CurrTime;
    unsigned long long      u32PrevTime;
    unsigned int        u32SSrc;
    StRtpFixedHdr       *pRtpFixedHdr;//rtp 头
    StNaluHdr           *pNaluHdr; //nalu头
    StFuIndicator       *pFuInd;
    StFuHdr             *pFuHdr;
    EmRtpPayload        emPayload;
#ifdef SAVE_NALU
    FILE                *pNaluFile;
#endif
} StRtpObj, *HndRtp;
unsigned int local_ip=0;
unsigned long server_port = RTP_DEFAULT_UDP_PORT;


/**************************************************************************************************
**
**
**
**************************************************************************************************/
unsigned int RtpCreate(unsigned int u32IP, int s32Port, EmRtpPayload emPayload)
{
    HndRtp hRtp = NULL;
    struct timeval stTimeval;
    struct ifreq stIfr;
	int s32Broadcast = 1;
    struct sockaddr_in addr;
	
    hRtp = (HndRtp)calloc(1, sizeof(StRtpObj));
    if(NULL == hRtp)
    {
        printf("Failed to create RTP handle\n");
        goto cleanup;
    }


    hRtp->s32Sock = -1;
    if((hRtp->s32Sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Failed to create socket\n");
        goto cleanup;
    }

    if(0xFF000000 == (u32IP & 0xFF000000))
    {
        if(-1 == setsockopt(hRtp->s32Sock, SOL_SOCKET, SO_BROADCAST, (char *)&s32Broadcast, sizeof(s32Broadcast)))
        {
            printf("Failed to set socket\n");
            goto cleanup;
        }
    }
	memset(&addr, 0, sizeof(addr));
	while(1)
	{
		addr.sin_port = BigLittleSwap16(server_port);
	    addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		
	    hRtp->stServAddr.sin_family = AF_INET;
	    hRtp->stServAddr.sin_port = BigLittleSwap16(s32Port);
	    hRtp->stServAddr.sin_addr.s_addr = u32IP;
	    bzero(&(hRtp->stServAddr.sin_zero), 8);
		if (bind(hRtp->s32Sock, (struct sockaddr *)&addr, sizeof(addr)))
		{
			printf("can't bind !!!!!!!!!!!!!!!!!!!!!!!!!!!");
			server_port++;
		}
		else
			break;
		
	}

    //初始化序号
    hRtp->u16SeqNum = 0;
    //初始化时间戳
    hRtp->u32TimeStampInc = 0;
    hRtp->u32TimeStampCurr = 0;

    //获取当前时间
    if(gettimeofday(&stTimeval, NULL) == -1)
    {
        printf("Failed to get os time\n");
        goto cleanup;
    }

    hRtp->u32PrevTime = stTimeval.tv_sec * 1000 + stTimeval.tv_usec / 1000;

    hRtp->emPayload = emPayload;

    //获取本机网络设备名
    strcpy(stIfr.ifr_name, "br0");
    if(ioctl(hRtp->s32Sock, SIOCGIFADDR, &stIfr) < 0)
    {
        //printf("Failed to get host ip\n");
        strcpy(stIfr.ifr_name, "eth0");
        if(ioctl(hRtp->s32Sock, SIOCGIFADDR, &stIfr) < 0)
        {
            printf("Failed to get host eth0 or wlan0 ip\n");
            goto cleanup;
        }
    }

    hRtp->u32SSrc = BigLittleSwap32(((struct sockaddr_in *)(&stIfr.ifr_addr))->sin_addr.s_addr);
	local_ip = hRtp->u32SSrc;
    //hRtp->u32SSrc = htonl(((struct sockaddr_in *)(&stIfr.ifr_addr))->sin_addr.s_addr);
    printf("!!!!!!!!!!!!!!!!!!!!!!rtp create:addr:%x,port:%d,local%x\n",u32IP,s32Port,hRtp->u32SSrc);
#ifdef SAVE_NALU
    hRtp->pNaluFile = fopen("nalu.264", "wb+");
    if(NULL == hRtp->pNaluFile)
    {
        printf("Failed to open nalu file!\n");
        goto cleanup;
    }
#endif
    printf("<><><><>success creat RTP<><><><>\n");

    return (unsigned int)hRtp;

cleanup:
    if(hRtp)
    {
        if(hRtp->s32Sock >= 0)
        {
            close(hRtp->s32Sock);
        }

        free(hRtp);
    }

    return 0;
}
/**************************************************************************************************
**
**
**
**************************************************************************************************/
void RtpDelete(unsigned int u32Rtp)
{
    HndRtp hRtp = (HndRtp)u32Rtp;

    if(hRtp)
    {
#ifdef SAVE_NALU
        if(hRtp->pNaluFile)
        {
            fclose(hRtp->pNaluFile);
        }
#endif

        if(hRtp->s32Sock >= 0)
        {
            close(hRtp->s32Sock);
        }

        free(hRtp);
    }
}
/**************************************************************************************************
**
**
**
************************************************************************************************/

extern unsigned char sps_tmp[256];
extern unsigned char pps_tmp[256];
extern int sps_len;
extern int pps_len;
//static uint32_t ts_current = 0;
//uint32_t ts_current = 0;
static int firstflag = 1;
unsigned long long F_ts = 0;
unsigned long long S_ts = 0;
static int test_SendNalu264(HndRtp hRtp, unsigned char *pNalBuf, int s32NalBufSize)
{
    unsigned char *nalu_buf;
    nalu_buf = pNalBuf;
	unsigned char *pSendBuf;
	int ret = 0;
    int len_sendbuf;
    unsigned char FirstNaluBytes;
	int fu_pack_num;        /* nalu 需要分片发送时分割的个数 */
    int last_fu_pack_size;  /* 最后一个分片的大小 */
    int fu_seq;             /* fu-A 序号 */
	struct timeval now;
    unsigned long long ts;
    pSendBuf = (unsigned char *)calloc(MAX_RTP_PKT_LENGTH + 100, sizeof(unsigned char));
    if(NULL == pSendBuf)
    {
        ret = -1;
        goto cleanup;
    }
	  gettimeofday(&now,NULL);
      ts = (now.tv_sec*1000 + now.tv_usec/1000);
	  if(firstflag){
        F_ts = ts;
	    firstflag = 0;
	    ts = ts *90;
	    printf("Run in set firstflag !!!!!\n");
      }//毫秒
      else
      ts = (ts - F_ts)*90;
     
	
     //ts_current += (90000 / 25);  /* 90000 / 25 = 3600 */
	 hRtp->pRtpFixedHdr = (StRtpFixedHdr *)pSendBuf;
	 hRtp->pRtpFixedHdr->u4CSrcLen   = 0;
	 hRtp->pRtpFixedHdr->u1Externsion  = 0;
	 hRtp->pRtpFixedHdr->u1Padding   =0;
     hRtp->pRtpFixedHdr->u7Payload   = H264;
     hRtp->pRtpFixedHdr->u2Version   = 2;
	 hRtp->pRtpFixedHdr->u32TimeStamp  = BigLittleSwap32(ts);
	 hRtp->pRtpFixedHdr->u32SSrc = hRtp->u32SSrc;

	
    FirstNaluBytes = *pNalBuf;
	
	if(s32NalBufSize < 1 ||(FirstNaluBytes&0x1f) == 0x08 ||(FirstNaluBytes&0x1f) == 0x07){
	     //ts = ts -2*(90000 / 25);	
       goto cleanup;
	}
	 /* if(S_ts){
          ts = S_ts;
	      S_ts = 0;
	  }*/
    
  /* if((FirstNaluBytes&0x1f) == 0x07){
        hRtp->pRtpFixedHdr->u1Marker    = 0;
		//hRtp->pRtpFixedHdr->u16SeqNum   = BigLittleSwap16((hRtp->u16SeqNum++));
	        S_ts = ts;
		if(sps_len>0)
		{	
		    hRtp->pRtpFixedHdr->u16SeqNum   = BigLittleSwap16(hRtp->u16SeqNum);
			hRtp->u16SeqNum++;
	        memcpy(pSendBuf+12, sps_tmp, sps_len);
	        if(sendto(hRtp->s32Sock, pSendBuf, sps_len+12, 0, (struct sockaddr *)&hRtp->stServAddr, sizeof(hRtp->stServAddr)) < 0)
	        {
	            ret = -1;
	            goto cleanup;
	        }
			//printf("Send sps success!!\n");
			//printf("send sps u16SeqNum = %u\n",hRtp->u16SeqNum);
		}
		if(pps_len>0)
		{	    
		         
		    hRtp->pRtpFixedHdr->u16SeqNum   = BigLittleSwap16(hRtp->u16SeqNum);
			hRtp->u16SeqNum++;
	        memcpy(pSendBuf+12, pps_tmp, pps_len);
	        if(sendto(hRtp->s32Sock, pSendBuf, pps_len+12, 0, (struct sockaddr *)&hRtp->stServAddr, sizeof(hRtp->stServAddr)) < 0)
	        {
	            ret = -1;
	            goto cleanup;
	        }
			//printf("Send pps success !!\n");
			//printf("send pps u16SeqNum = %u\n",hRtp->u16SeqNum);
		}
		goto cleanup;
    }*/
	  

	if(s32NalBufSize <= MAX_RTP_PKT_LENGTH){
      hRtp->pRtpFixedHdr->u1Marker    = 1;
	  hRtp->pRtpFixedHdr->u16SeqNum   = BigLittleSwap16(hRtp->u16SeqNum);
	  hRtp->u16SeqNum++;
	  hRtp->pNaluHdr                  = (StNaluHdr *)(pSendBuf + 12);
      hRtp->pNaluHdr->u1F             = (FirstNaluBytes & 0x80) >> 7;
      hRtp->pNaluHdr->u2Nri           = (FirstNaluBytes & 0x60) >> 5;
      hRtp->pNaluHdr->u5Type          = FirstNaluBytes & 0x1f;
	  memcpy(pSendBuf + 13, nalu_buf + 1, s32NalBufSize - 1);
      len_sendbuf = 12 + s32NalBufSize;
      if(sendto(hRtp->s32Sock, pSendBuf, len_sendbuf, 0, (struct sockaddr *)&hRtp->stServAddr, sizeof(hRtp->stServAddr)) < 0)
      {
            ret = -1;
            goto cleanup;
      }
	   //printf("send one packet u16SeqNum = %u\n",hRtp->u16SeqNum);
	   goto cleanup;
	}else{
            fu_pack_num = s32NalBufSize % MAX_RTP_PKT_LENGTH ? (s32NalBufSize / MAX_RTP_PKT_LENGTH + 1) : s32NalBufSize / MAX_RTP_PKT_LENGTH;
            last_fu_pack_size = s32NalBufSize % MAX_RTP_PKT_LENGTH ? s32NalBufSize % MAX_RTP_PKT_LENGTH : MAX_RTP_PKT_LENGTH;
            fu_seq = 0;
			//printf("fu_pack_num = %d \n",fu_pack_num);
			//printf("Send for FU-A !!!\n");
			for (fu_seq = 0; fu_seq < fu_pack_num; fu_seq++) {
                 if (fu_seq == 0) {
	               hRtp->pRtpFixedHdr->u16SeqNum   = BigLittleSwap16(hRtp->u16SeqNum);
			       hRtp->u16SeqNum++;
	               hRtp->pRtpFixedHdr->u1Marker    = 0;
				  //指定fu indicator位置
                   hRtp->pFuInd            = (StFuIndicator *)(pSendBuf + 12);
                   hRtp->pFuInd->u1F       = (FirstNaluBytes & 0x80) >> 7;
                   hRtp->pFuInd->u2Nri     = (FirstNaluBytes & 0x60) >> 5;
                   hRtp->pFuInd->u5Type    = 28;

                   //指定fu header位置
                   hRtp->pFuHdr            = (StFuHdr *)(pSendBuf + 13);
                   hRtp->pFuHdr->u1S       = 1;
				   hRtp->pFuHdr->u1E       = 0;
				   hRtp->pFuHdr->u1R       = 0;
                   hRtp->pFuHdr->u5Type    = FirstNaluBytes & 0x1f;

				   memcpy(pSendBuf + 14, nalu_buf + 1, MAX_RTP_PKT_LENGTH - 1);
				   len_sendbuf = 12 + 2 + (MAX_RTP_PKT_LENGTH - 1);
				   if(sendto(hRtp->s32Sock, pSendBuf, len_sendbuf, 0, (struct sockaddr *)&hRtp->stServAddr, sizeof(hRtp->stServAddr)) < 0)
                   {
                       ret = -1;
                       goto cleanup;
                   }
				      //printf("send FU-A First packet u16SeqNum = %u\n",hRtp->u16SeqNum);
				      //printf("Send FU-A First packet!! TimeStamp = %ld fu_seq = %d ts_current = %d\n",hRtp->pRtpFixedHdr->u32TimeStamp,fu_seq,ts_current);
                 }else if (fu_seq < fu_pack_num - 1) { 
	               hRtp->pRtpFixedHdr->u16SeqNum   = BigLittleSwap16(hRtp->u16SeqNum);
			       hRtp->u16SeqNum++;
	               hRtp->pRtpFixedHdr->u1Marker    = 0;
				  //指定fu indicator位置
                   hRtp->pFuInd            = (StFuIndicator *)(pSendBuf + 12);
                   hRtp->pFuInd->u1F       = (FirstNaluBytes & 0x80) >> 7;
                   hRtp->pFuInd->u2Nri     = (FirstNaluBytes & 0x60) >> 5;
                   hRtp->pFuInd->u5Type    = 28;

                   //指定fu header位置
                   hRtp->pFuHdr            = (StFuHdr *)(pSendBuf + 13);
                   hRtp->pFuHdr->u1S       = 0;
				   hRtp->pFuHdr->u1E       = 0;
				   hRtp->pFuHdr->u1R       = 0;
                   hRtp->pFuHdr->u5Type    = FirstNaluBytes & 0x1f;

				   memcpy(pSendBuf + 14, nalu_buf + MAX_RTP_PKT_LENGTH * fu_seq, MAX_RTP_PKT_LENGTH); 
                   len_sendbuf = 12 + 2 + MAX_RTP_PKT_LENGTH;
                   if(sendto(hRtp->s32Sock, pSendBuf, len_sendbuf, 0, (struct sockaddr *)&hRtp->stServAddr, sizeof(hRtp->stServAddr)) < 0)
                   {
                    ret = -1;
                    goto cleanup;
                   }
				   //printf("send FU-A mid packet u16SeqNum = %u\n",hRtp->u16SeqNum);
                    //printf("Send FU-A mid Packet !!! TimeStamp = %ld fu_seq = %d ts_current = %d\n",hRtp->pRtpFixedHdr->u32TimeStamp,fu_seq,ts_current);
				 }else{
				     
	               hRtp->pRtpFixedHdr->u16SeqNum   = BigLittleSwap16(hRtp->u16SeqNum);
			       hRtp->u16SeqNum++;
	               hRtp->pRtpFixedHdr->u1Marker    = 1;
				  //指定fu indicator位置
                   hRtp->pFuInd            = (StFuIndicator *)(pSendBuf + 12);
                   hRtp->pFuInd->u1F       = (FirstNaluBytes & 0x80) >> 7;
                   hRtp->pFuInd->u2Nri     = (FirstNaluBytes & 0x60) >> 5;
                   hRtp->pFuInd->u5Type    = 28;

                   //指定fu header位置
                   hRtp->pFuHdr            = (StFuHdr *)(pSendBuf + 13);
                   hRtp->pFuHdr->u1S       = 0;
				   hRtp->pFuHdr->u1E       = 1;
				   hRtp->pFuHdr->u1R       = 0;
                   hRtp->pFuHdr->u5Type    = FirstNaluBytes & 0x1f;
				   memcpy(pSendBuf + 14, nalu_buf + MAX_RTP_PKT_LENGTH * fu_seq, last_fu_pack_size);
				   len_sendbuf = 12 + 2 + last_fu_pack_size;
				   if(sendto(hRtp->s32Sock, pSendBuf, len_sendbuf, 0, (struct sockaddr *)&hRtp->stServAddr, sizeof(hRtp->stServAddr)) < 0)
                   {
                    ret = -1;
                    goto cleanup;
                   }
				   //printf("Send FU-A last packet u16SeqNum = %u\n",hRtp->u16SeqNum);
				   //printf("Send FU-A last Packet !!! TimeStamp = %ld fu_seq = %d ts_current = %d\n",hRtp->pRtpFixedHdr->u32TimeStamp,fu_seq,ts_current);
				 }

			}
              goto cleanup;
	}


	

cleanup:
    if(pSendBuf)
    {
        free((void *)pSendBuf);
		pSendBuf = NULL;
    }
	//printf("\n");
	//fflush(stdout);

    return ret;	
}

/*static int SendNalu264(HndRtp hRtp, unsigned char *pNalBuf, int s32NalBufSize)
{
    unsigned char *pNaluPayload;
    unsigned char *pSendBuf;
    int s32Bytes = 0;
    int s32Ret = 0;
    struct timeval stTimeval;
    unsigned char *pNaluCurr;
    int s32NaluRemain;
    unsigned char u8NaluBytes;
    pSendBuf = (unsigned char *)calloc(MAX_RTP_PKT_LENGTH + 100, sizeof(unsigned char));
    if(NULL == pSendBuf)
    {
        s32Ret = -1;
        goto cleanup;
    }

    hRtp->pRtpFixedHdr = (StRtpFixedHdr *)pSendBuf;
    hRtp->pRtpFixedHdr->u7Payload   = H264;
    hRtp->pRtpFixedHdr->u2Version   = 2;
    hRtp->pRtpFixedHdr->u1Marker    = 0;
    hRtp->pRtpFixedHdr->u32SSrc     = hRtp->u32SSrc;
    //计算时间戳
    hRtp->pRtpFixedHdr->u32TimeStamp = BigLittleSwap32(hRtp->u32TimeStampCurr * (90000 / 1000));
    //printf("timestamp:%lld\n",hRtp->u32TimeStampCurr);
    if(gettimeofday(&stTimeval, NULL) == -1)
    {
        printf("Failed to get os time\n");
        s32Ret = -1;
        goto cleanup;
    }

    //保存nalu首byte
    u8NaluBytes = *pNalBuf;
	
    //设置未发送的Nalu数据指针位置
    pNaluCurr = pNalBuf + 1;
    //设置剩余的Nalu数据数量
    s32NaluRemain = s32NalBufSize - 1;
		 
    if((u8NaluBytes&0x1f) == 0x05)
	 printf("find I Frame s32NaluRemain = %d\n",s32NaluRemain);
		 
    if ((u8NaluBytes&0x1f) == 0x07 ||(u8NaluBytes&0x1f) == 0x08)
	{
	    printf("sps_len : %d pps_len : %d u8NaluBytes = %02x\n",sps_len,pps_len,u8NaluBytes&0x1f);
		//hRtp->pRtpFixedHdr->u1Marker    = 1;
		pNaluPayload = (pSendBuf + 12);
		if(sps_len>0 && (u8NaluBytes&0x1f) == 0x07)
		{	        
	        memcpy(pNaluPayload, sps_tmp, sps_len);
	        if(sendto(hRtp->s32Sock, pSendBuf, sps_len+12, 0, (struct sockaddr *)&hRtp->stServAddr, sizeof(hRtp->stServAddr)) < 0)
	        {
	            s32Ret = -1;
	            goto cleanup;
	        }
			printf("Send sps success cleanup!!\n");
			goto cleanup;
		}
		if(pps_len>0 && (u8NaluBytes&0x1f) == 0x08)
		{	        
	        memcpy(pNaluPayload, pps_tmp, pps_len);
	        if(sendto(hRtp->s32Sock, pSendBuf, pps_len+12, 0, (struct sockaddr *)&hRtp->stServAddr, sizeof(hRtp->stServAddr)) < 0)
	        {
	            s32Ret = -1;
	            goto cleanup;
	        }
			printf("Send pps success cleanup!!\n");
			goto cleanup;
		}
		
	}

	
	
    //NALU包小于等于最大包长度，直接发送
    if(s32NaluRemain <= MAX_RTP_PKT_LENGTH)
    {
        hRtp->pRtpFixedHdr->u1Marker    = 1;
        hRtp->pRtpFixedHdr->u16SeqNum   = BigLittleSwap16((hRtp->u16SeqNum++));
        hRtp->pNaluHdr                  = (StNaluHdr *)(pSendBuf + 12);
        hRtp->pNaluHdr->u1F             = (u8NaluBytes & 0x80) >> 7;
        hRtp->pNaluHdr->u2Nri           = (u8NaluBytes & 0x60) >> 5;
        hRtp->pNaluHdr->u5Type          = u8NaluBytes & 0x1f;

        pNaluPayload = (pSendBuf + 13);
        memcpy(pNaluPayload, pNaluCurr, s32NaluRemain);

        s32Bytes = s32NaluRemain + 13;
		printf("Send one packet , type = %02x!!!\n",hRtp->pNaluHdr->u5Type);
		fflush(stdout);
        if(sendto(hRtp->s32Sock, pSendBuf, s32Bytes, 0, (struct sockaddr *)&hRtp->stServAddr, sizeof(hRtp->stServAddr)) < 0)
        {
            s32Ret = -1;
            goto cleanup;
        }
//#ifdef SAVE_NALU
  //      fwrite(pSendBuf, s32Bytes, 1, hRtp->pNaluFile);
//#endif
    }
    //NALU包大于最大包长度，分批发送
    else
    {
        //指定fu indicator位置
        hRtp->pFuInd            = (StFuIndicator *)(pSendBuf + 12);
        hRtp->pFuInd->u1F       = (u8NaluBytes & 0x80) >> 7;
        hRtp->pFuInd->u2Nri     = (u8NaluBytes & 0x60) >> 5;
        hRtp->pFuInd->u5Type    = 28;

        //指定fu header位置
        hRtp->pFuHdr            = (StFuHdr *)(pSendBuf + 13);
        hRtp->pFuHdr->u1R       = 0;
        hRtp->pFuHdr->u5Type    = u8NaluBytes & 0x1f;

        //指定payload位置
        pNaluPayload = (pSendBuf + 14);

        //当剩余Nalu数据多于0时分批发送nalu数据
        while(s32NaluRemain > 0)
        {
            //配置fixed header
            //每个包序号增1
            hRtp->pRtpFixedHdr->u16SeqNum = BigLittleSwap16(hRtp->u16SeqNum++);
            hRtp->pRtpFixedHdr->u1Marker = (s32NaluRemain <= MAX_RTP_PKT_LENGTH) ? 1 : 0;

            //配置fu header
            //最后一批数据则置1
            hRtp->pFuHdr->u1E       = (s32NaluRemain <= MAX_RTP_PKT_LENGTH) ? 1 : 0;
            //第一批数据则置1
            hRtp->pFuHdr->u1S       = (s32NaluRemain == (s32NalBufSize - 1)) ? 1 : 0;

            s32Bytes = (s32NaluRemain < MAX_RTP_PKT_LENGTH) ? s32NaluRemain : MAX_RTP_PKT_LENGTH;


            memcpy(pNaluPayload, pNaluCurr, s32Bytes);

            //发送本批次
            s32Bytes = s32Bytes + 14;
			printf("Send for FU-A ,type = %02x\n",u8NaluBytes&0x1f);
			fflush(stdout);
            if(sendto(hRtp->s32Sock, pSendBuf, s32Bytes, 0, (struct sockaddr *)&hRtp->stServAddr, sizeof(hRtp->stServAddr)) < 0)
            {
                s32Ret = -1;
                goto cleanup;
            }
//#ifdef SAVE_NALU
            //fwrite(pSendBuf, s32Bytes, 1, hRtp->pNaluFile);
//#endif

            //指向下批数据
            pNaluCurr += MAX_RTP_PKT_LENGTH;
            //计算剩余的nalu数据长度
            s32NaluRemain -= MAX_RTP_PKT_LENGTH;
        }
    }

cleanup:
    if(pSendBuf)
    {
        free((void *)pSendBuf);
    }
	printf("\n");
	fflush(stdout);

    return s32Ret;
}*/
/**************************************************************************************************
**
**
**
**************************************************************************************************/
/*static int SendNalu711(HndRtp hRtp, unsigned char *buf, int bufsize)
{
    char *pSendBuf;
    int s32Bytes = 0;
    int s32Ret = 0;

    pSendBuf = (char *)calloc(MAX_RTP_PKT_LENGTH + 100, sizeof(char));
    if(NULL == pSendBuf)
    {
        s32Ret = -1;
        goto cleanup;
    }
    hRtp->pRtpFixedHdr = (StRtpFixedHdr *)pSendBuf;
    hRtp->pRtpFixedHdr->u7Payload     = G711;
    hRtp->pRtpFixedHdr->u2Version     = 2;

    hRtp->pRtpFixedHdr->u1Marker = 1;   //标志位，由具体协议规定其值。

    hRtp->pRtpFixedHdr->u32SSrc = hRtp->u32SSrc;

    hRtp->pRtpFixedHdr->u16SeqNum  = htons(hRtp->u16SeqNum ++);

    memcpy(pSendBuf + 12, buf, bufsize);

    hRtp->pRtpFixedHdr->u32TimeStamp = htonl(hRtp->u32TimeStampCurr);
    //printf("SendNalu711 timestamp:%lld\n",hRtp->pRtpFixedHdr->u32TimeStamp);
    s32Bytes = bufsize + 12;
    if(sendto(hRtp->s32Sock, pSendBuf, s32Bytes, 0, (struct sockaddr *)&hRtp->stServAddr, sizeof(hRtp->stServAddr)) < 0)
    {
        printf("Failed to send!");
        s32Ret = -1;
        goto cleanup;
    }
cleanup:
    if(pSendBuf)
    {
        free((void *)pSendBuf);
    }
    return s32Ret;
}*/
/**************************************************************************************************
**
**
**
**************************************************************************************************/
	
	FILE* tmp_file;
	int first_in = 1;

unsigned int RtpSend(unsigned int u32Rtp, unsigned char *pData, int s32DataSize, unsigned int u32TimeStamp)
{
    int s32NalSize = 0;
    int nalhead_pos = 0,naltail_pos = 0;
    HndRtp hRtp = (HndRtp)u32Rtp;
	unsigned char * nalubuffer = NULL;
    

    hRtp->u32TimeStampCurr = u32TimeStamp;

    while(nalhead_pos<s32DataSize)
    {
        if(pData[nalhead_pos++] == 0x00 && 
			pData[nalhead_pos++] == 0x00) 
		{ 	
				if(pData[nalhead_pos++] == 0x00 && 
					pData[nalhead_pos++] == 0x01 ){
				   goto gotnal_head;
				}
				
				else
					continue;
		 }
		else 
			continue;

gotnal_head:
		naltail_pos = nalhead_pos;  
		
		while (naltail_pos<s32DataSize)  
		{  
			if(pData[naltail_pos++] == 0x00 && 
				pData[naltail_pos++] == 0x00 )
			{  
					if(pData[naltail_pos++] == 0x00 &&
						pData[naltail_pos++] == 0x01)
					{	
						s32NalSize = (naltail_pos-4)-nalhead_pos;
						break;
					}
			}  
		}
		
      if(nalhead_pos<s32DataSize && naltail_pos<s32DataSize){
        nalubuffer=(unsigned char*)malloc(s32NalSize);
	    memset(nalubuffer,0,s32NalSize);
		memcpy(nalubuffer,pData+nalhead_pos,s32NalSize);
		//SendNalu264(hRtp, nalubuffer,s32NalSize);
		test_SendNalu264(hRtp, nalubuffer,s32NalSize);
		free(nalubuffer);
		nalhead_pos = naltail_pos-4;
      }

	  if(nalhead_pos<s32DataSize && naltail_pos>=s32DataSize){
	  	s32NalSize = s32DataSize -nalhead_pos;
        nalubuffer=(unsigned char*)malloc(s32NalSize);
	    memset(nalubuffer,0,s32NalSize);
		memcpy(nalubuffer,pData+nalhead_pos,s32NalSize);
		//SendNalu264(hRtp, nalubuffer,s32NalSize);
		test_SendNalu264(hRtp, nalubuffer,s32NalSize);
		free(nalubuffer);
		nalhead_pos = naltail_pos;
      }
		
		
    }//while
                    
    return 0;
}
/**************************************************************************************************
**
**
**
**************************************************************************************************/

#ifdef __cplusplus
}
#endif
