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




#define TAG_TYPE_SCRIPT 18
#define TAG_TYPE_AUDIO  8
#define TAG_TYPE_VIDEO  9

typedef unsigned char byte;
typedef unsigned int uint;

typedef struct {
	byte Signature[3];
	byte Version;
	byte Flags;
	uint DataOffset;
} FLV_HEADER;

typedef struct {
	byte TagType;
	byte DataSize[3];
	byte Timestamp[3];
	uint Reserved;
} TAG_HEADER;

typedef struct {
    TAG_HEADER TAG_Header;
    unsigned char * TAG_data_ptr;            
    unsigned int previous_tag_len;
}TAG;

extern int simplest_flvVideoTag_to_h264(char *h264_url,unsigned char * tagDataPtr ,int tagDataLen,int file_index);

//reverse_bytes - turn a BigEndian byte array into a LittleEndian integer
uint reverse_bytes(byte *p, char c) {
    int r = 0;
    int i;

   for (i=0; i<c; i++) 
    r |= ( *(p+i) << (((c-1)*8)-8*i));
	
	return r;
}

/**
 * Analysis FLV file
 * @param url    Location of input FLV file.
 */

int simplest_flv_parser(char *in_url,char * vidoe_out_url,char * audio_out_urtl){

	//whether output audio/video stream
	int output_a=1;
	int output_v=1;
    char tmp_data[100]={0,};
	//-------------
	FILE *ifh=NULL,*vfh=NULL, *afh = NULL;

	//FILE *myout=fopen("output_log.txt","wb+");
	FILE *myout=stdout;

	FLV_HEADER flv;
	//TAG_HEADER tagheader;
    TAG tag;
	uint previoustagsize, previoustagsize_z=0;
    
	uint ts=0, ts_new=0;
    int video_count=0;

	ifh = fopen(in_url, "rb+");
	if ( ifh== NULL) {
		printf("Failed to open files!");
		return -1;
	}

	//FLV file header
	//fread((char *)&flv,1,sizeof(FLV_HEADER),ifh);
	fread(tmp_data,1,9,ifh);
    flv.Signature[0]=tmp_data[0];
    flv.Signature[1]=tmp_data[1];
    flv.Signature[2]=tmp_data[2];
    flv.Version=tmp_data[3];
    flv.Flags=tmp_data[4];
    flv.DataOffset=tmp_data[5]*0x1000000+tmp_data[6]*0x10000+tmp_data[7]*0x100+tmp_data[8];

	fprintf(myout,"============== FLV Header ==============\n");
	fprintf(myout,"Signature:  0x %c %c %c\n",flv.Signature[0],flv.Signature[1],flv.Signature[2]);
	fprintf(myout,"Version:    0x %X\n",flv.Version);
	fprintf(myout,"Flags  :    0x %X\n",flv.Flags);
	fprintf(myout,"HeaderSize: 0x %X\n",flv.DataOffset);
    fprintf(myout,"========================================\n");

	//move the file pointer to the end of the header
	fseek(ifh, flv.DataOffset, SEEK_SET);

	//process each tag
	do {
        char char_int[4];
        fread(char_int,4,1,ifh);
        previoustagsize= (char_int[0]<<24) + (char_int[1]<<16) + (char_int[2]<<8) +(char_int[3]<<0);
		fread(tmp_data,11,1,ifh);
        tag.TAG_Header.TagType=tmp_data[0];
        tag.TAG_Header.DataSize[0]=tmp_data[1];
        tag.TAG_Header.DataSize[1]=tmp_data[2];
        tag.TAG_Header.DataSize[2]=tmp_data[3];
        tag.TAG_Header.Timestamp[0]=tmp_data[4];
        tag.TAG_Header.Timestamp[1]=tmp_data[5];
        tag.TAG_Header.Timestamp[2]=tmp_data[6];
      
		int tagheader_datasize=tag.TAG_Header.DataSize[0]*65536+tag.TAG_Header.DataSize[1]*256+tag.TAG_Header.DataSize[2];
		int tagheader_timestamp=tag.TAG_Header.Timestamp[0]*65536+tag.TAG_Header.Timestamp[1]*256+tag.TAG_Header.Timestamp[2];

		char tagtype_str[10];
		switch(tag.TAG_Header.TagType){
		case TAG_TYPE_AUDIO:sprintf(tagtype_str,"AUDIO");break;
		case TAG_TYPE_VIDEO:sprintf(tagtype_str,"VIDEO");break;
		case TAG_TYPE_SCRIPT:sprintf(tagtype_str,"SCRIPT");break;
		default:sprintf(tagtype_str,"UNKNOWN");break;
		}
		fprintf(myout,"[%6s] %6d %6d |",tagtype_str,tagheader_datasize,tagheader_timestamp);

		//if we are not past the end of file, process the tag
		if (feof(ifh)) {
			break;
		}

		//process tag by type
		switch (tag.TAG_Header.TagType) {

		case TAG_TYPE_AUDIO:{ 
			char audiotag_str[100]={0};
			strcat(audiotag_str,"| ");
			char tagdata_first_byte;
			tagdata_first_byte=fgetc(ifh);
			int x=tagdata_first_byte&0xF0;
			x=x>>4;
			switch (x)
			{
			case 0:strcat(audiotag_str,"Linear PCM, platform endian");break;
			case 1:strcat(audiotag_str,"ADPCM");break;
			case 2:strcat(audiotag_str,"MP3");break;
			case 3:strcat(audiotag_str,"Linear PCM, little endian");break;
			case 4:strcat(audiotag_str,"Nellymoser 16-kHz mono");break;
			case 5:strcat(audiotag_str,"Nellymoser 8-kHz mono");break;
			case 6:strcat(audiotag_str,"Nellymoser");break;
			case 7:strcat(audiotag_str,"G.711 A-law logarithmic PCM");break;
			case 8:strcat(audiotag_str,"G.711 mu-law logarithmic PCM");break;
			case 9:strcat(audiotag_str,"reserved");break;
			case 10:strcat(audiotag_str,"AAC");break;
			case 11:strcat(audiotag_str,"Speex");break;
			case 14:strcat(audiotag_str,"MP3 8-Khz");break;
			case 15:strcat(audiotag_str,"Device-specific sound");break;
			default:strcat(audiotag_str,"UNKNOWN");break;
			}
			strcat(audiotag_str,"| ");
			x=tagdata_first_byte&0x0C;
			x=x>>2;
			switch (x)
			{
			case 0:strcat(audiotag_str,"5.5-kHz");break;
			case 1:strcat(audiotag_str,"1-kHz");break;
			case 2:strcat(audiotag_str,"22-kHz");break;
			case 3:strcat(audiotag_str,"44-kHz");break;
			default:strcat(audiotag_str,"UNKNOWN");break;
			}
			strcat(audiotag_str,"| ");
			x=tagdata_first_byte&0x02;
			x=x>>1;
			switch (x)
			{
			case 0:strcat(audiotag_str,"8Bit");break;
			case 1:strcat(audiotag_str,"16Bit");break;
			default:strcat(audiotag_str,"UNKNOWN");break;
			}
			strcat(audiotag_str,"| ");
			x=tagdata_first_byte&0x01;
			switch (x)
			{
			case 0:strcat(audiotag_str,"Mono");break;
			case 1:strcat(audiotag_str,"Stereo");break;
			default:strcat(audiotag_str,"UNKNOWN");break;
			}
			fprintf(myout,"%s",audiotag_str);

			//if the output file hasn't been opened, open it.
			if(output_a!=0&&afh == NULL){
				afh = fopen(audio_out_urtl, "wb");
			}

			//TagData - First Byte Data
			int data_size=reverse_bytes((byte *)&tag.TAG_Header.DataSize, sizeof(tag.TAG_Header.DataSize))-1;
			if(output_a!=0){
				//TagData+1
				for (int i=0; i<data_size; i++)
					fputc(fgetc(ifh),afh);

			}else{
				for (int i=0; i<data_size; i++)
					fgetc(ifh);
			}
			break;
		}
		case TAG_TYPE_VIDEO:{
			char videotag_str[100]={0};
			strcat(videotag_str,"| ");
			char tagdata_first_byte;
            long file_offset=0;

            file_offset=ftell(ifh);
			fread((void *)&tagdata_first_byte,sizeof(tagdata_first_byte),1,ifh);
			int x=tagdata_first_byte&0xF0;
			x=x>>4;
			switch (x)
			{
			case 1:strcat(videotag_str,"key frame  ");break;
			case 2:strcat(videotag_str,"inter frame");break;
			case 3:strcat(videotag_str,"disposable inter frame");break;
			case 4:strcat(videotag_str,"generated keyframe");break;
			case 5:strcat(videotag_str,"video info/command frame");break;
			default:strcat(videotag_str,"UNKNOWN");break;
			}
			strcat(videotag_str,"| ");
			x=tagdata_first_byte&0x0F;
			switch (x)
			{
			case 1:strcat(videotag_str,"JPEG (currently unused)");break;
			case 2:strcat(videotag_str,"Sorenson H.263");break;
			case 3:strcat(videotag_str,"Screen video");break;
			case 4:strcat(videotag_str,"On2 VP6");break;
			case 5:strcat(videotag_str,"On2 VP6 with alpha channel");break;
			case 6:strcat(videotag_str,"Screen video version 2");break;
			case 7:strcat(videotag_str,"AVC");break;
			default:strcat(videotag_str,"UNKNOWN");break;
			}
			fprintf(myout,"%s",videotag_str);

			fseek(ifh, -1, SEEK_CUR);
			//if the output file hasn't been opened, open it.
			if (vfh == NULL&&output_v!=0) {
				//write the flv header (reuse the original file's hdr) and first previoustagsize
					vfh = fopen(vidoe_out_url, "wb");
					//fwrite((char *)&flv,1, sizeof(flv),vfh);
					tmp_data[0]=flv.Signature[0];
					tmp_data[1]=flv.Signature[1];
					tmp_data[2]=flv.Signature[2];
					tmp_data[3]=flv.Version;
					tmp_data[4]=flv.Flags;
					tmp_data[5]=flv.DataOffset>>24;
					tmp_data[6]=flv.DataOffset>>16;
					tmp_data[7]=flv.DataOffset>>8;
					tmp_data[8]=flv.DataOffset;
					fwrite(tmp_data,1,9,vfh);
                    //printf("flv 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x ",tmp_data[0],
                    //    tmp_data[1],tmp_data[2],tmp_data[3],tmp_data[4],tmp_data[5],tmp_data[6],
                    //    tmp_data[7],tmp_data[8]);
                    previoustagsize_z=0;
                    tmp_data[0]=previoustagsize_z>>24;
					tmp_data[1]=previoustagsize_z>>16;
					tmp_data[2]=previoustagsize_z>>8;
					tmp_data[3]=previoustagsize_z;
					fwrite(tmp_data,1,sizeof(previoustagsize_z),vfh);
			}
            
           
#if 0
			//Change Timestamp
			//Get Timestamp
			ts = reverse_bytes((byte *)&tagheader.Timestamp, sizeof(tagheader.Timestamp));
			ts=ts*2;
			//Writeback Timestamp
			ts_new = reverse_bytes((byte *)&ts, sizeof(ts));
			memcpy(&tagheader.Timestamp, ((char *)&ts_new) + 1, sizeof(tagheader.Timestamp));
#endif


			//TagData + Previous Tag Size
			//TagHeader
			tmp_data[0]=tag.TAG_Header.TagType;
            tmp_data[1]=tag.TAG_Header.DataSize[0];
            tmp_data[2]=tag.TAG_Header.DataSize[1];
            tmp_data[3]=tag.TAG_Header.DataSize[2];
            tmp_data[4]=tag.TAG_Header.Timestamp[0];
            tmp_data[5]=tag.TAG_Header.Timestamp[1];
            tmp_data[6]=tag.TAG_Header.Timestamp[2];
            tmp_data[7]=0;
            tmp_data[8]=0;
            tmp_data[9]=0;
            tmp_data[10]=0;
            fwrite(tmp_data,1, 11,vfh);
            
            //tag data
            video_count++;
			int data_size=reverse_bytes((byte *)&tag.TAG_Header.DataSize, sizeof(tag.TAG_Header.DataSize));
            tag.TAG_data_ptr=(unsigned char *)malloc(data_size);
            if(tag.TAG_data_ptr == NULL){
                printf("malloc tag.TAG_data_ptr failed ,malloc len 0x%x",data_size);
                return false;
            }
            fread(tag.TAG_data_ptr,data_size,1,ifh);  
            fwrite(tag.TAG_data_ptr,1,data_size,vfh);
            simplest_flvVideoTag_to_h264("video.h264",tag.TAG_data_ptr+1,data_size-1,ftell(ifh)-data_size+1);
            if(tag.TAG_data_ptr!=NULL)
                free(tag.TAG_data_ptr);

			// previous tag size
            fread(tmp_data,1,4,ifh);
            fwrite(tmp_data,1,4,vfh);
            
			fseek(ifh, -4, SEEK_CUR);

			break;
			}
		default:

			//skip the data of this tag
			fseek(ifh, reverse_bytes((byte *)&tag.TAG_Header.DataSize, sizeof(tag.TAG_Header.DataSize)), SEEK_CUR);
            break;
		}

		fprintf(myout,"\n");

	} while (!feof(ifh));


	//_fcloseall();
	if(ifh!=NULL)
        fclose(ifh);
    if(vfh!=NULL)
        fclose(vfh);
    if(afh!=NULL)
        fclose(afh);
    
	return 0;
}
