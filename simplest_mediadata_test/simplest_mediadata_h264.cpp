/**
 * 最简单的视音频数据处理示例
 * Simplest MediaData Test
 *
 * 雷霄骅 Lei Xiaohua
 * leixiaohua1020@126.com
 * 中国传媒大学/数字电视技术
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 * 本项目包含如下几种视音频测试示例：
 *  (1)像素数据处理程序。包含RGB和YUV像素格式处理的函数。
 *  (2)音频采样数据处理程序。包含PCM音频采样格式处理的函数。
 *  (3)H.264码流分析程序。可以分离并解析NALU。
 *  (4)AAC码流分析程序。可以分离并解析ADTS帧。
 *  (5)FLV封装格式分析程序。可以将FLV中的MP3音频码流分离出来。
 *  (6)UDP-RTP协议分析程序。可以将分析UDP/RTP/MPEG-TS数据包。
 *
 * This project contains following samples to handling multimedia data:
 *  (1) Video pixel data handling program. It contains several examples to handle RGB and YUV data.
 *  (2) Audio sample data handling program. It contains several examples to handle PCM data.
 *  (3) H.264 stream analysis program. It can parse H.264 bitstream and analysis NALU of stream.
 *  (4) AAC stream analysis program. It can parse AAC bitstream and analysis ADTS frame of stream.
 *  (5) FLV format analysis program. It can analysis FLV file and extract MP3 audio stream.
 *  (6) UDP-RTP protocol analysis program. It can analysis UDP/RTP/MPEG-TS Packet.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	NALU_TYPE_SLICE    = 1,
	NALU_TYPE_DPA      = 2,
	NALU_TYPE_DPB      = 3,
	NALU_TYPE_DPC      = 4,
	NALU_TYPE_IDR      = 5,
	NALU_TYPE_SEI      = 6,
	NALU_TYPE_SPS      = 7,
	NALU_TYPE_PPS      = 8,
	NALU_TYPE_AUD      = 9,
	NALU_TYPE_EOSEQ    = 10,
	NALU_TYPE_EOSTREAM = 11,
	NALU_TYPE_FILL     = 12,
} NaluType;

typedef enum {
	NALU_PRIORITY_DISPOSABLE = 0,
	NALU_PRIRITY_LOW         = 1,
	NALU_PRIORITY_HIGH       = 2,
	NALU_PRIORITY_HIGHEST    = 3
} NaluPriority;


typedef struct
{
	int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
	unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
	unsigned max_size;            //! Nal Unit Buffer size
	int forbidden_bit;            //! should be always FALSE
	int nal_reference_idc;        //! NALU_PRIORITY_xxxx
	int nal_unit_type;            //! NALU_TYPE_xxxx    
	char *buf;                    //! contains the first byte followed by the EBSP
} NALU_t;

FILE *h264bitstream = NULL;                //!< the bit stream file

int info2=0, info3=0;

static int FindStartCode2 (unsigned char *Buf){
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=1) return 0; //0x000001?
	else return 1;
}

static int FindStartCode3 (unsigned char *Buf){
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1) return 0;//0x00000001?
	else return 1;
}


int GetAnnexbNALU (NALU_t *nalu){
	int pos = 0;
	int StartCodeFound, rewind;
	unsigned char *Buf;

	if ((Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL) 
		printf ("GetAnnexbNALU: Could not allocate Buf memory\n");

	nalu->startcodeprefix_len=3;

	if (3 != fread (Buf, 1, 3, h264bitstream)){
		free(Buf);
		return 0;
	}
	info2 = FindStartCode2 (Buf);
	if(info2 != 1) {
		if(1 != fread(Buf+3, 1, 1, h264bitstream)){
			free(Buf);
			return 0;
		}
		info3 = FindStartCode3 (Buf);
		if (info3 != 1){ 
			free(Buf);
			return -1;
		}
		else {
			pos = 4;
			nalu->startcodeprefix_len = 4;
		}
	}
	else{
		nalu->startcodeprefix_len = 3;
		pos = 3;
	}
	StartCodeFound = 0;
	info2 = 0;
	info3 = 0;

	while (!StartCodeFound){
		if (feof (h264bitstream)){
			nalu->len = (pos-1)-nalu->startcodeprefix_len;
			memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);     
			nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit
			nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
			nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit
			free(Buf);
			return pos-1;
		}
		Buf[pos++] = fgetc (h264bitstream);
		info3 = FindStartCode3(&Buf[pos-4]);
		if(info3 != 1)
			info2 = FindStartCode2(&Buf[pos-3]);
		StartCodeFound = (info2 == 1 || info3 == 1);
	}

	// Here, we have found another start code (and read length of startcode bytes more than we should
	// have.  Hence, go back in the file
	rewind = (info3 == 1)? -4 : -3;

	if (0 != fseek (h264bitstream, rewind, SEEK_CUR)){
		free(Buf);
		printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
	}

	// Here the Start code, the complete NALU, and the next start code is in the Buf.  
	// The size of Buf is pos, pos+rewind are the number of bytes excluding the next
	// start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code

	nalu->len = (pos+rewind)-nalu->startcodeprefix_len;
	memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);//
	nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit
	nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
	nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit
	free(Buf);

	return (pos+rewind);
}

typedef struct {
    char config_version;
    char AVCProfile_indication;
    char profile_compatibility;
    char AVCLevel_Indication;
    char Length_size_minus_one;      //（lengthSizeMinusOne & 3）+1
    char SPS_num; //E1 -- SPS 的个数，numOfSequenceParameterSets & 0x1F，实际测试时发现总为E1，计算结果为1
    short SPS_length;    //00 31 -- SPS 的长度，2个字节，计算结果49
    char SPS_NALU_Units;  //sps 数据，长度为SPS_length
    char PPS_num;                           //PPS 的个数，实际测试时发现总为E1，计算结果为1
    short PPS_length;        //PPS 的长度 2个字节
    char PPS_NALU_Units;    //pps 数据 ，长度为PPS_length
}AVC_DECODER_CONFIGURATION_RECORD;

/**
 * Analysis flv tag data to  H.264 Bitstream
 * @param  h264_url    Location of output H.264 bitstream file.
 * @param  tagDataPtr 
  * @param tagDataLen
 */
int simplest_flvVideoTag_to_h264(char *h264_url,unsigned char * tagDataPtr ,int tagDataLen,int file_index){

	NALU_t *n;
	int buffersize=100000;
    FILE * h264_out=NULL;
    char avc_str[50]={0,};
    static AVC_DECODER_CONFIGURATION_RECORD AVC_decoder_config;
    static int NALU_count=0;;

	//FILE *myout=fopen("output_log.txt","wb+");
	FILE *myout=stdout;

    //printf("Open file %s\n",url);
	h264_out=fopen(h264_url, "wb+");
	if (h264_out==NULL){
		printf("Open file error\n");
		return 0;
	}

    char avc_type=0;
    int tagData_index=0;
    unsigned int composition_time=0;

    avc_type=tagDataPtr[tagData_index++];
    if(avc_type== 0){
        strcat(avc_str,"    |AVC header");
        tagData_index+=3;                   //avc type header composition time 0
        //AVCDecoderconfiguratonRecord
        AVC_decoder_config.config_version=tagDataPtr[tagData_index++];
        AVC_decoder_config.AVCProfile_indication=tagDataPtr[tagData_index++];
        AVC_decoder_config.profile_compatibility=tagDataPtr[tagData_index++];
        AVC_decoder_config.AVCLevel_Indication=tagDataPtr[tagData_index++];
        AVC_decoder_config.Length_size_minus_one=(tagDataPtr[tagData_index++]&0x03)+1;
        AVC_decoder_config.SPS_num=tagDataPtr[tagData_index++]&0x1f;
        AVC_decoder_config.SPS_length=tagDataPtr[tagData_index++]*0x100+tagDataPtr[tagData_index++];
        tagData_index+=AVC_decoder_config.SPS_length;
        AVC_decoder_config.PPS_num=tagDataPtr[tagData_index++];
        AVC_decoder_config.PPS_length=tagDataPtr[tagData_index++]*0x100+tagDataPtr[tagData_index++];
        tagData_index+=AVC_decoder_config.PPS_length;
        
        fprintf(myout,"%s |config_version:0x%x |AVCProfileIndication:0x%x |profile_compatibility:0x%x |AVCLevel_Indication:0x%x |Length_size_minus_one:0x%x |SPS_num:0x%x |SPS_length:0x%x |PPS_num:0x%x |PPS_length:0x%x"
                        ,avc_str,AVC_decoder_config.config_version,AVC_decoder_config.AVCProfile_indication
                        ,AVC_decoder_config.profile_compatibility,AVC_decoder_config.AVCLevel_Indication,AVC_decoder_config.Length_size_minus_one
                        ,AVC_decoder_config.SPS_num,AVC_decoder_config.SPS_length,AVC_decoder_config.PPS_num,AVC_decoder_config.PPS_length);
    }
    else if(avc_type==1){
        int NALU_LEN;
        strcat(avc_str,"    |AVC NALU");
        fprintf(myout,"%s",avc_str);
        composition_time=(unsigned char)tagDataPtr[tagData_index++]*0x10000+(unsigned char)tagDataPtr[tagData_index++]*0x100+(unsigned char)tagDataPtr[tagData_index++];
        sprintf(avc_str," |composition_time %6d ",composition_time);
        fprintf(myout,"%s",avc_str);
        if(AVC_decoder_config.Length_size_minus_one!=4){
            printf("avc length size minux one error %d",AVC_decoder_config.Length_size_minus_one);
        }
        do{
            NALU_count++;
            NALU_LEN=(unsigned char)tagDataPtr[tagData_index++]*0x1000000+(unsigned char)tagDataPtr[tagData_index++]*0x10000+(unsigned char)tagDataPtr[tagData_index++]*0x100+(unsigned char)tagDataPtr[tagData_index++];
            tagData_index+=NALU_LEN;
            sprintf(avc_str," |NALU:%d len:0x%x ",NALU_count,NALU_LEN);
            fprintf(myout,"%s",avc_str);
        }
        while(tagData_index<tagDataLen);
        
    }
    else if(avc_type==2){
        strcat(avc_str,"    |AVC end");
        fprintf(myout,"%s",avc_str);

    }
    else{
        strcat(avc_str,"    |AVC unknown");
        fprintf(myout,"%s",avc_str);
    }
    


    if(h264_out!=NULL)
        fclose(h264_out);

    return true;

}

/**
 * Analysis H.264 Bitstream
 * @param url    Location of input H.264 bitstream file.
 */
int simplest_h264_parser(char *url){

	NALU_t *n;
	int buffersize=100000;

	//FILE *myout=fopen("output_log.txt","wb+");
	FILE *myout=stdout;

    printf("parser h264 file %s\n",url);
	h264bitstream=fopen(url, "rb+");
	if (h264bitstream==NULL){
		printf("Open file error\n");
		return 0;
	}

	n = (NALU_t*)calloc (1, sizeof (NALU_t));
	if (n == NULL){
		printf("Alloc NALU Error\n");
		return 0;
	}

	n->max_size=buffersize;
	n->buf = (char*)calloc (buffersize, sizeof (char));
	if (n->buf == NULL){
		free (n);
		printf ("AllocNALU: n->buf");
		return 0;
	}

	int data_offset=0;
	int nal_num=0;
	printf("-----+-------- NALU Table ------+---------+\n");
	printf(" NUM |    POS  |    IDC |  TYPE |   LEN   |\n");
	printf("-----+---------+--------+-------+---------+\n");

	while(!feof(h264bitstream)) 
	{
		int data_lenth;
		data_lenth=GetAnnexbNALU(n);

		char type_str[20]={0};
		switch(n->nal_unit_type){
			case NALU_TYPE_SLICE:sprintf(type_str,"SLICE");break;
			case NALU_TYPE_DPA:sprintf(type_str,"DPA");break;
			case NALU_TYPE_DPB:sprintf(type_str,"DPB");break;
			case NALU_TYPE_DPC:sprintf(type_str,"DPC");break;
			case NALU_TYPE_IDR:sprintf(type_str,"IDR");break;
			case NALU_TYPE_SEI:sprintf(type_str,"SEI");break;
			case NALU_TYPE_SPS:sprintf(type_str,"SPS");break;
			case NALU_TYPE_PPS:sprintf(type_str,"PPS");break;
			case NALU_TYPE_AUD:sprintf(type_str,"AUD");break;
			case NALU_TYPE_EOSEQ:sprintf(type_str,"EOSEQ");break;
			case NALU_TYPE_EOSTREAM:sprintf(type_str,"EOSTREAM");break;
			case NALU_TYPE_FILL:sprintf(type_str,"FILL");break;
		}
		char idc_str[20]={0};
		switch(n->nal_reference_idc>>5){
			case NALU_PRIORITY_DISPOSABLE:sprintf(idc_str,"DISPOS");break;
			case NALU_PRIRITY_LOW:sprintf(idc_str,"LOW");break;
			case NALU_PRIORITY_HIGH:sprintf(idc_str,"HIGH");break;
			case NALU_PRIORITY_HIGHEST:sprintf(idc_str,"HIGHEST");break;
		}

		fprintf(myout,"%5d| %8d| %7s| %6s| %8d|\n",nal_num,data_offset,idc_str,type_str,n->len);

		data_offset=data_offset+data_lenth;

		nal_num++;
	}

	//Free
	if (n){
		if (n->buf){
			free(n->buf);
			n->buf=NULL;
		}
		free (n);
	}

    fclose(h264bitstream);
	return 0;
}

