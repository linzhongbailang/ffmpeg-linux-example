
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


/*
**  filename_in         yuv420文件名
**  filename_out        h264文件名
*/
int simplest_ffmpeg_h264_encoder(char * filename_in,char * filename_out);
/*
**  filename_in         h264文件名
**  filename_out        yuv420文件名
*/

//int simplest_ffmpeg_h264_decoder(char * filename_in,char * filename_out);
    
//int simplest_ffmpeg_rtsp_test(void);

//int simplest_ffmpeg_rtsp_server(void);

int main(int argc, char* argv[]){

    if(argc<2){
        printf("simplest_video_test h264-encoder\n");
        printf("simplest_video_test h264-decoder\n");
        printf("simplest_video_test rtsp-test\n");
        printf("simplest_video_test rtsp-server\n");
    }
	//Test
	//simplest_yuv420_split("lena_256x256_yuv420p.yuv",256,256,1);

	//simplest_yuv444_split("lena_256x256_yuv444p.yuv",256,256,1);
	
	//simplest_yuv420_halfy("lena_256x256_yuv420p.yuv",256,256,1);

	//simplest_yuv420_gray("lena_256x256_yuv420p.yuv",256,256,1);

	//simplest_yuv420_border("lena_256x256_yuv420p.yuv",256,256,20,1);

	//simplest_yuv420_graybar(640, 360,0,255,10,"graybar_640x360.yuv");

	//simplest_yuv420_psnr("lena_256x256_yuv420p.yuv","lena_distort_256x256_yuv420p.yuv",256,256,1);

	//simplest_rgb24_split("cie1931_500x500.rgb", 500, 500,1);

	//simplest_rgb24_to_bmp("lena_256x256_rgb24.rgb",256,256,"output_lena.bmp");

	//simplest_rgb24_to_yuv420("lena_256x256_rgb24.rgb",256,256,1,"output_lena.yuv");

	//simplest_rgb24_colorbar(640, 360,"colorbar_640x360.rgb");

	//simplest_pcm16le_split("NocturneNo2inEflat_44.1k_s16le.pcm");

	//simplest_pcm16le_halfvolumeleft("NocturneNo2inEflat_44.1k_s16le.pcm");

	//simplest_pcm16le_doublespeed("NocturneNo2inEflat_44.1k_s16le.pcm");

	//simplest_pcm16le_to_pcm8("NocturneNo2inEflat_44.1k_s16le.pcm");

	//simplest_pcm16le_cut_singlechannel("drum.pcm",2360,120);

	//simplest_pcm16le_to_wave("NocturneNo2inEflat_44.1k_s16le.pcm",2,44100,"output_nocturne.wav");
    else if(0==strcmp(argv[1],"h264-encoder")){
        printf("simplest_ffmpeg_h264_encoder test\n");
        //simplest_h264_parser("../../simplest_mediadata_test/sintel.h264");
        simplest_ffmpeg_h264_encoder("../../simplest_ffmpeg_video_encoder/ds_480x272.yuv","ds.h264");
    }
	else if(0==strcmp(argv[1],"h264-decoder")){
        printf("simplest_ffmpeg_h264_decoder test\n");
        //simplest_ffmpeg_h264_decoder("../../../../../opencv/opencv_extra/testdata/highgui/video/big_buck_bunny.mp4","ds.yuv");
        //simplest_ffmpeg_h264_decoder("../../simplest_mediadata_test/cuc_ieschool.flv","ds.yuv");
	    //simplest_ffmpeg_h264_decoder("ds.h264","ds.yuv");


        //simplest_ffmpeg_h264_decoder("rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov","ds.yuv");
        //simplest_ffmpeg_h264_decoder("rtsp://10.59.8.45:8554/test","ds.yuv");
	    //simplest_ffmpeg_h264_decoder("../../simplest_mediadata_test/sintel.h264","ds.yuv");
    }
    else if(0==strcmp(argv[1],"rtsp-test")){
        printf("simplest_ffmpeg_rtsp_test test\n");
        //simplest_ffmpeg_h264_decoder("../../../../../opencv/opencv_extra/testdata/highgui/video/big_buck_bunny.mp4","ds.yuv");
        //simplest_ffmpeg_h264_decoder("../../simplest_mediadata_test/cuc_ieschool.flv","ds.yuv");
	    //simplest_ffmpeg_rtsp_test();
	    //simplest_ffmpeg_h264_decoder("../../simplest_mediadata_test/sintel.h264","ds.yuv");
    }
    else if(0==strcmp(argv[1],"rtsp-server")){
        printf("simplest_ffmpeg_rtsp_server test\n");
        //simplest_ffmpeg_h264_decoder("../../../../../opencv/opencv_extra/testdata/highgui/video/big_buck_bunny.mp4","ds.yuv");
        //simplest_ffmpeg_h264_decoder("../../simplest_mediadata_test/cuc_ieschool.flv","ds.yuv");
	    //simplest_ffmpeg_rtsp_server();
	    //simplest_ffmpeg_h264_decoder("../../simplest_mediadata_test/sintel.h264","ds.yuv");
    }
    else{
        printf("argv[] error\n");

    }


    


    
	return 1 ;




}
