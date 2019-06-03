#include "simplest_ffmpeg_h264_decoder_tao.h"




using namespace cv;
using namespace std;


ffmpegDecode :: ~ffmpegDecode()
{
    pCvMat->release();
    //�ͷű��ζ�ȡ��֡�ڴ�
    av_free_packet(packet);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
}

ffmpegDecode :: ffmpegDecode(char * file)
{
    pAvFrame = NULL/**pFrameRGB = NULL*/;
    pFormatCtx  = NULL;
    pCodecCtx   = NULL;
    pCodec      = NULL;

    pCvMat = new cv::Mat();
    i=0;
    videoindex=0;

    ret = 0;
    got_picture = 0;
    img_convert_ctx = NULL;
    y_size = 0;
    packet = NULL;

    if (NULL == file)
    {
        filepath =  "opencv.h264";
    }
    else
    {
        filepath = file;
    }

    init();
    openDecode();
    prepare();

    return;
}

void ffmpegDecode :: init()
{
    //ffmpegע�Ḵ�������������ȵĺ���av_register_all()��
    //�ú��������л���ffmpeg��Ӧ�ó����м������ǵ�һ�������õġ�ֻ�е����˸ú���������ʹ�ø��������������ȡ�
    //����ע�������е��ļ���ʽ�ͱ�������Ŀ⣬�������ǽ����Զ���ʹ���ڱ��򿪵ĺ��ʸ�ʽ���ļ��ϡ�ע����ֻ��Ҫ���� av_register_all()һ�Σ����������������main()�����������������ϲ����Ҳ����ֻע���ض��ĸ�ʽ�ͱ������������ͨ����û�б�Ҫ��������
    av_register_all();

    //pFormatCtx = avformat_alloc_context();
    //����Ƶ�ļ�,ͨ������filepath������ļ��������������ȡ�ļ���ͷ�����Ұ���Ϣ���浽���Ǹ���AVFormatContext�ṹ���С�
    //���2����������ָ��������ļ���ʽ�������С�͸�ʽ���������������������Ϊ��NULL����0��libavformat���Զ������Щ������
    if(avformat_open_input(&pFormatCtx,filepath,NULL,NULL)!=0)
    {
        printf("�޷����ļ�\n");
        return;
    }

    //�����ļ�������Ϣ,avformat_open_input����ֻ�Ǽ�����ļ���ͷ��������Ҫ������ļ��е�������Ϣ
    //if(av_find_stream_info(pFormatCtx)<0)
    if(avformat_find_stream_info(pFormatCtx,NULL))
    {
        printf("Couldn't find stream information.\n");
        return;
    }
    return;
}

void ffmpegDecode :: openDecode()
{
    //�����ļ��ĸ��������ҵ���һ����Ƶ��������¼�����ı�����Ϣ
    videoindex = -1;
    for(i=0; i<pFormatCtx->nb_streams; i++) 
    {
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
        {
            videoindex=i;
            break;
        }
    }
    if(videoindex==-1)
    {
        printf("Didn't find a video stream.\n");
        return;
    }
    pCodecCtx=pFormatCtx->streams[videoindex]->codec;

    //�ڿ��������֧�ָø�ʽ�Ľ�����
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL)
    {
        printf("Codec not found.\n");
        return;
    }

    //�򿪽�����
    if(avcodec_open2(pCodecCtx, pCodec,NULL) < 0)
    {
        printf("Could not open codec.\n");
        return;
    }
}

void ffmpegDecode :: prepare()
{
    //����һ��ָ֡�룬ָ�������ԭʼ֡
    pAvFrame=avcodec_alloc_frame();
    y_size = pCodecCtx->width * pCodecCtx->height;
    //����֡�ڴ�
    packet=(AVPacket *)av_malloc(sizeof(AVPacket));
    av_new_packet(packet, y_size);

    //���һ����Ϣ-----------------------------
    printf("�ļ���Ϣ-----------------------------------------\n");
    av_dump_format(pFormatCtx,0,filepath,0);
    //av_dump_formatֻ�Ǹ����Ժ���������ļ���������Ƶ���Ļ�����Ϣ�ˣ�֡�ʡ��ֱ��ʡ���Ƶ�����ȵ�
    printf("-------------------------------------------------\n");
}

int ffmpegDecode :: readOneFrame()
{
    int result = 0;
    result = av_read_frame(pFormatCtx, packet);
    return result;
}

cv::Mat ffmpegDecode :: getDecodedFrame()
{
    if(packet->stream_index==videoindex)
    {
        //����һ��֡
        ret = avcodec_decode_video2(pCodecCtx, pAvFrame, &got_picture, packet);
        if(ret < 0)
        {
            printf("�������\n");
            return cv::Mat();
        }
        if(got_picture)
        {
            //���ݱ�����Ϣ������Ⱦ��ʽ
            if(img_convert_ctx == NULL){
                img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                    pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
                    AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL); 
            }   
            //----------------------opencv
            if (pCvMat->empty())
            {
                pCvMat->create(cv::Size(pCodecCtx->width, pCodecCtx->height),CV_8UC3);
            }

            if(img_convert_ctx != NULL)  
            {  
                get(pCodecCtx, img_convert_ctx, pAvFrame);
            }
        }
    }
    av_free_packet(packet);

    return *pCvMat;
}

cv::Mat ffmpegDecode :: getLastFrame()
{
    ret = avcodec_decode_video2(pCodecCtx, pAvFrame, &got_picture, packet);
    if(got_picture) 
    {  
        //���ݱ�����Ϣ������Ⱦ��ʽ
        img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL); 

        if(img_convert_ctx != NULL)  
        {  
            get(pCodecCtx, img_convert_ctx,pAvFrame);
        }  
    } 
    return *pCvMat;
}

void ffmpegDecode :: get(AVCodecContext * pCodecCtx, SwsContext * img_convert_ctx, AVFrame * pFrame)
{
    if (pCvMat->empty())
    {
        pCvMat->create(cv::Size(pCodecCtx->width, pCodecCtx->height),CV_8UC3);
    }

    AVFrame *pFrameRGB = NULL;
    uint8_t  *out_bufferRGB = NULL;
    pFrameRGB = avcodec_alloc_frame();

    //��pFrameRGB֡���Ϸ�����ڴ�;
    int size = avpicture_get_size(AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
    out_bufferRGB = new uint8_t[size];
    avpicture_fill((AVPicture *)pFrameRGB, out_bufferRGB, AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);

    //YUV to RGB
    sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
    
    memcpy(pCvMat->data,out_bufferRGB,size);

    delete[] out_bufferRGB;
    av_free(pFrameRGB);
}




extern "C"
{

int simplest_ffmpeg_h264_decoder(char * filename_in,char * filename_out){
    int ret=0;
    //video_t *  video_h264;
    int video_time=0;
    int mat_count=0;
    //Mat * frame_ptr=NULL;//����һ��Mat���������ڴ洢ÿһ֡��ͼ��


    
    printf("decoder h264 file %s\n",filename_in);

    ffmpegDecode ffmpeg_tao_decoder(filename_in);
    
    while(1)
    {
        ret=ffmpeg_tao_decoder.readOneFrame();
        if(ret<0){
            printf("decoder file end\n");
            break;
        }

        //Mat  mat_decoder=NULL;//����һ��Mat���������ڴ洢ÿһ֡��ͼ��
        Mat mat_decoder=ffmpeg_tao_decoder.getDecodedFrame();

        if(mat_decoder.empty()){
            printf("mat_decoder empty\n");
        }
        else{
            mat_count++;
            printf("get mat:%d\n",mat_count);
        }

    }
#if 0
    video_h264=video_init(filename_in,&ret);
    if(video_h264==NULL){
        printf("video init failed ret=%d\n",ret);
    }
    
    video_time=video_get_alltime(video_h264);
    printf("h264 video time %d s\n",video_time);

    ret=video_get_frame(video_h264,frame_ptr);
    if(frame_ptr==NULL){
        printf("video_get_frame failed ret=%d\n",ret);
    }

    
        
    video_uninit(video_h264);
#endif
}


}
