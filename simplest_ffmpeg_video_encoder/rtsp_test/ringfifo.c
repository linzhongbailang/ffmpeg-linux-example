/*ringbuf .c*/

#include<stdio.h>
#include<ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ringfifo.h"
#include "rtputils.h"
//#include "rtsavdef.h"

#define NMAX 64

int iput = 0; /* 环形缓冲区的当前放入位置 */
int iget = 0; /* 缓冲区的当前取出位置 */
int n = 0; /* 环形缓冲区中的元素总数量 */

struct ringbuf ringfifo[NMAX];

extern int UpdateSpsOrPps(unsigned char *data,int frame_type,int len);

extern void UpdateSps(unsigned char *data,int len);
extern void UpdatePps(unsigned char *data,int len);

/* 环形缓冲区的地址编号计算函数，如果到达唤醒缓冲区的尾部，将绕回到头部。
环形缓冲区的有效地址编号为：0到(NMAX-1)
*/
void ringmalloc(int size)
{
    int i;
    for(i =0; i<NMAX; i++)
    {
        ringfifo[i].buffer = malloc(size);
        ringfifo[i].size = 0;
        ringfifo[i].frame_type = 0;
       // printf("FIFO INFO:idx:%d,len:%d,ptr:%x\n",i,ringfifo[i].size,(int)(ringfifo[i].buffer));
    }
    iput = 0; /* 环形缓冲区的当前放入位置 */
    iget = 0; /* 缓冲区的当前取出位置 */
    n = 0; /* 环形缓冲区中的元素总数量 */
}
/**************************************************************************************************
**
**
**
**************************************************************************************************/
void ringreset()
{
    iput = 0; /* 环形缓冲区的当前放入位置 */
    iget = 0; /* 缓冲区的当前取出位置 */
    n = 0; /* 环形缓冲区中的元素总数量 */
}
/**************************************************************************************************
**
**
**
**************************************************************************************************/
void ringfree(void)
{
    int i;
    printf("begin free mem\n");
    for(i =0; i<NMAX; i++)
    {
       // printf("FREE FIFO INFO:idx:%d,len:%d,ptr:%x\n",i,ringfifo[i].size,(int)(ringfifo[i].buffer));
        free(ringfifo[i].buffer);
        ringfifo[i].size = 0;
    }
}
/**************************************************************************************************
**
**
**
**************************************************************************************************/
int addring(int i)
{
    return (i+1) == NMAX ? 0 : i+1;
}

/**************************************************************************************************
**
**
**
**************************************************************************************************/
/* 从环形缓冲区中取一个元素 */

int ringget(struct ringbuf *getinfo)
{
    int Pos;
    if(n>0)
    {
        Pos = iget;
        iget = addring(iget);
        n--;
        getinfo->buffer = (ringfifo[Pos].buffer);
        getinfo->frame_type = ringfifo[Pos].frame_type;
        getinfo->size = ringfifo[Pos].size;
        //printf("Get FIFO INFO:idx:%d,len:%d,ptr:%x,type:%d\n",Pos,getinfo->size,(int)(getinfo->buffer),getinfo->frame_type);
		//fflush(stdout);
        return ringfifo[Pos].size;
    }
    else
    {
        //printf("Buffer is empty\n");
        return 0;
    }
}
/**************************************************************************************************
**
**
**
**************************************************************************************************/
/* 向环形缓冲区中放入一个元素*/
void ringput(unsigned char *buffer,int size,int encode_type)
{

    if(n<NMAX)
    {
        memcpy(ringfifo[iput].buffer,buffer,size);
        ringfifo[iput].size= size;
        ringfifo[iput].frame_type = encode_type;
        //printf("Put FIFO INFO:idx:%d,len:%d,ptr:%x,type:%d\n",iput,ringfifo[iput].size,(int)(ringfifo[iput].buffer),ringfifo[iput].frame_type);
        iput = addring(iput);
        n++;
    }
    else
    {
        //  printf("Buffer is full\n");
    }
}

/**************************************************************************************************
**
**
**
**************************************************************************************************/
int PutH264DataToBuffer(void *pstStream, int datalen)
{
	int head = 0,tail=0,len=0,type = 0,iframe = 0;
	len = datalen;
    unsigned char * stDataPacket;
	stDataPacket=(unsigned char*)malloc(datalen);
	memset(stDataPacket,0,datalen);
    memcpy(stDataPacket, (unsigned char *)pstStream,datalen);


    if(n<NMAX)
    {
	    memcpy(ringfifo[iput].buffer,stDataPacket,len);
	  
        while(head < len){
            if(stDataPacket[head++] == 0x00 && 
            	stDataPacket[head++] == 0x00) { 
            	
            	if(stDataPacket[head++] == 0x00 && 
            	  stDataPacket[head++] == 0x01 ){
            	        type = stDataPacket[head] &0x1f;
            			//printf("####### type = %02x\n #######",type);
            		   goto gotnal_head;
            	  }
            		
            		else
                      continue;	
            }
            else
               continue;

    	    gotnal_head:
    		
    		tail = head;  
    		while (tail < len)  
    		{  
    			if(stDataPacket[tail++] == 0x00 && 
    				stDataPacket[tail++] == 0x00 )
    			{  
                    if(stDataPacket[tail++] == 0x00 &&
                    	stDataPacket[tail++] == 0x01)
                    {	
                        if(type == 0x07){
                            UpdateSps(stDataPacket+head,(tail-4)-head);
                            iframe=1;
                            head = tail -4;
                        }

                        else if(type == 0x08){
                            UpdatePps(stDataPacket+head,(tail-4)-head);
                            head = len;
                        }
                    	break;
                    }
    			}  
    		}
        }

        ringfifo[iput].size= len;
		if(iframe)
		{
//			printf("I");
			ringfifo[iput].frame_type = FRAME_TYPE_I;
		}        	
		else
		{
			ringfifo[iput].frame_type = FRAME_TYPE_P;
//			printf("P");
		}
        iput = addring(iput);
//		printf("(%d)",iput);
//		fflush(stdout);
        free(stDataPacket);
        n++;
    }

	return 0;
}
