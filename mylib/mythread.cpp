#include "mythread.h"
#include "config.h"
#include <unistd.h>

TID_PARA tid_para = {0};
int getParaFlag = 0;
char str_tmp[100] = {0};

CycleBuffer* fifo = new CycleBuffer();
FFmpegProcess *ffmpeg = new FFmpegProcess();

int tid1_ffm_init(char *ip,char *filename);

int tid1_ffm_init(char *ip,char *filename)
{


    strcat(str_tmp,"rtsp://");
    strcat(str_tmp,ip);
    strcat(str_tmp,"/");
    strcat(str_tmp,filename);
    strcat(str_tmp,"\0");
    printf("confirm ip:%s\n",str_tmp);
    #if RTSP_BY_WLAN
    ffmpeg->FFmpeg_RTSPVideoInit(str_tmp,tid_para.width_t,tid_para.height_t);
    #elif RTSP_BY_LAN
    ffmpeg->FFmpeg_RTSPVideoInit("rtsp://192.168.1.3:8554/video.264",tid_para.width_t,tid_para.height_t);
    #endif
    //ffmpeg->FFmpeg_RTSPVideoInit("rtsp://192.168.137.1/video.264",tid_para.width_t,tid_para.height_t);
    //ffmpeg->FFmpeg_RTSPVideoInit("rtsp://192.168.1.178/live",tid_para.width_t,tid_para.height_t);
    if((tid_para.width_t != 0)&&(tid_para.height_t != 0))
    {

    }
    else
    {
        perror("RTSP INIT ERR");
        exit(0);
        return -1;
    }
    return 0;
}


void tid2_ffm_working(int flag)
{
    int ret = 0;
    uint64_t utime_stamp;
    static int index = 0;
    static int init_flag = flag;
label:
    printf("init flag is %d\n",init_flag);
    if(init_flag != 1)
    {
        #if RTSP_BY_WLAN
            ffmpeg->FFmpeg_RTSPVideoInit(str_tmp,tid_para.width_t,tid_para.height_t);
        #elif RTSP_BY_LAN
        ffmpeg->FFmpeg_RTSPVideoInit("rtsp://192.168.1.3:8554/video.264",tid_para.width_t,tid_para.height_t);
        #endif
        //ffmpeg->FFmpeg_RTSPVideoInit("rtsp://192.168.137.1/video.264",tid_para.width_t,tid_para.height_t);
        //ffmpeg->FFmpeg_RTSPVideoInit("rtsp://192.168.1.178/live",tid_para.width_t,tid_para.height_t);
        if((tid_para.width_t != 0)&&(tid_para.height_t != 0))
        {

        }
        else
        {
            perror("RTSP INIT ERR");
            exit(0);
        }
    }
    init_flag = 0;
    printf("myffm run...\r\n");
    while(1)
    {
        ffmpeg->FFmpeg_RTSPPacketInit();
        ret = ffmpeg->FFmpeg_RTSPVideoSaveFrame();
        if(ret < 0)
        {
            printf("...parse fram fail~~~\r\n");
            ffmpeg->FFmpeg_RTSPpacketUnref();
            goto label;
        }
        //printf("index:%d,dur:%lld dts:%lld pts:%lld\n",index,ffmpeg->mVideoSrcPkt.duration,ffmpeg->mVideoSrcPkt.dts,DD);
        //index++;
        utime_stamp =1000000 * ffmpeg->mVideoSrcPkt.dts * ffmpeg->mpVideoSrcStream->time_base.num / ffmpeg->mpVideoSrcStream->time_base.den;

        //printf("stamp:%lld\n",utime_stamp);
        while(0 == fifo->push(&(fifo->mbufferPara),ffmpeg->mVideoSrcPkt.data,ffmpeg->mVideoSrcPkt.size,utime_stamp))
        {
            usleep(10);
            printf("F ");
        }
        ffmpeg->FFmpeg_RTSPpacketUnref();
    }
}

void tid3_cim_working(void)
{
    int ret = 0;
    long int data_size = 0;
    uint64_t utime = 0;
    uint8_t *tmp;
label3:
    printf("mycim run...\r\n");
    V4l2dec_videoplay_init( tid_para.width_t,tid_para.height_t);
    gettimeofday(&tsv0,NULL);
    while(1)
    {

        while((tmp = fifo->pop(&(fifo->mbufferPara),data_size,utime)) == NULL)
        {
            usleep(10);
            printf("fifo is empty..\r\n");
        }
        ret = v4l2dec_handling_perfame(tmp,data_size,utime);
        if(ret < 0)
        {
            printf("dec err,quit cycle\r\n");
            //v4l2dec_stop();
            goto label3;
        }
    }
    v4l2dec_stop();
}


