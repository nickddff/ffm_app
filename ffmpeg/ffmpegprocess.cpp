#include "ffmpegprocess.h"

FFmpegProcess::FFmpegProcess()
{

}

FFmpegProcess::~FFmpegProcess()
{
    av_dict_free(&avdic);
    avformat_close_input(&mpVideoSrcFormatCxt);
}


int FFmpegProcess::FFmpeg_GetVersion()
{
    //输出版本号
    int version = 0;
    version = (int)avcodec_version();
    return version;
}

int FFmpegProcess::FFmpeg_RTSPVideoInit(const char* url,int &width_t,int &height_t)
{
    int ret = -1;
    avformat_network_init();

    /****open tcp***/
    av_dict_set(&avdic,"rtsp_transport","tcp",0);
    av_dict_set(&avdic,"max_delay","5000000",0);
    mpVideoSrcFormatCxt = avformat_alloc_context();
    const char* rtspUrl = url;
    //打开输入的URL
    ret = avformat_open_input(&mpVideoSrcFormatCxt, rtspUrl, NULL, &avdic);  //open
    //ret = avformat_open_input(&mpVideoSrcFormatCxt, rtspUrl, NULL, NULL);
    if (ret != 0)
    {
        fprintf(stderr, "could not open input file! \n");
        return -1;
    }
    ret = avformat_find_stream_info(mpVideoSrcFormatCxt, NULL);
    if (ret < 0)
    {
        fprintf(stderr, "could not find stream info! \n");
        return -1;
    }
    for (unsigned i = 0; i<mpVideoSrcFormatCxt->nb_streams; i++)    //
    {
        if (mpVideoSrcFormatCxt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            mpVideoSrcStream = mpVideoSrcFormatCxt->streams[i];
            break;
        }
    }
    if (mpVideoSrcStream == NULL)
    {
        fprintf(stderr, "didn't find any video stream! \n");
        return -1;
    }

    //打印信息
        fprintf(stderr, "---------------- File Information ---------------\n");
    av_dump_format(mpVideoSrcFormatCxt, 0, rtspUrl, 0);
        fprintf(stderr, "---------------- File Information ---------------\n");
    //申请AVCodecContext空间。需要传递一个编码器，也可以不传，但不会包含编码器。
    mpVideoSrcCodecCxt = avcodec_alloc_context3(NULL);
    //该函数用于将流里面的参数，也就是AVStream里面的参数直接复制到AVCodecContext的上下文当中
    avcodec_parameters_to_context(mpVideoSrcCodecCxt, mpVideoSrcStream->codecpar);
    //复制解码器上下文空间，如果失败则释放
    if (!mpVideoSrcCodecCxt)
    {
        printf("Failed to alloc codec context! \r\n");
        //释放所有解码器
        //FFmpeg_RTSPVideoClose();
        return -3;
    }
    //查找H264解码器 AV_CODEC_ID_H264
    mpVideoSrcCodec = avcodec_find_decoder(mpVideoSrcStream->codecpar->codec_id);
    printf("decoder: %d\r\n",mpVideoSrcStream->codecpar->codec_id);
    //如果找不到则返回错误
    if (mpVideoSrcCodec == NULL)
    {
        fprintf(stderr, "can not find codec_id! \n");
        return -1;
    }
    //打开解码器上下文输出，初始化一个视音频编解码器的AVCoFFmpeg_RTSPVideoSaveFramedecContex
    ret = avcodec_open2(mpVideoSrcCodecCxt, mpVideoSrcCodec, NULL);
    if (ret < 0)
    {
        printf("avcodec_open2 error! \r\n");
        //FFmpeg_RTSPVideoClose();
        return -4;
    }
    width_t = mpVideoSrcCodecCxt->width;
    height_t = mpVideoSrcCodecCxt->height;


    //分配帧空间
    //pFrameRGB = av_frame_alloc();
    if (mpVideoSrcFormatCxt == NULL)
    {
        fprintf(stderr, "mpVideoSrcFormatCxt is NULL! \n");
        return -1;
    }
    return ret;
}

void FFmpegProcess::FFmpeg_RTSPPacketInit(void)
{
    av_init_packet(&mVideoSrcPkt);
}

int FFmpegProcess::FFmpeg_RTSPVideoSaveFrame(void)
{
    int ret = 0;
    //printf("step1\r\n");
    ret = av_read_frame(mpVideoSrcFormatCxt, &mVideoSrcPkt);
    if (ret < 0)
    {
        fprintf(stderr, "av_read_frame error! \n");
        return ret;
    }
    //printf("step2\r\n");
    return ret ;
}

void FFmpegProcess::FFmpeg_RTSPpacketUnref(void)
{
    av_packet_unref(&mVideoSrcPkt);
}




