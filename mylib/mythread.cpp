#include "mythread.h"

TID_PARA tid_para = {0};
int getParaFlag = 0;

CycleBuffer* fifo = new CycleBuffer();
FFmpegProcess *ffmpeg = new FFmpegProcess();

int tid1_ffm_init(char *ip,char *filename);

int tid1_ffm_init(char *ip,char *filename)
{
    char str_tmp[100] = {0};
    strcat(str_tmp,"rtsp://");
    strcat(str_tmp,ip);
    strcat(str_tmp,":8554/");
    strcat(str_tmp,filename);
    strcat(str_tmp,"\0");
    //printf("confirm ip:%s\n",str_tmp);
    ffmpeg->FFmpeg_RTSPVideoInit("rtsp://192.168.137.1/video.264",tid_para.width_t,tid_para.height_t);
    //ffmpeg->FFmpeg_RTSPVideoInit(str_tmp,tid_para.width_t,tid_para.height_t);
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


void tid2_ffm_working(void)
{
    int ret = 0;
    printf("myffm run...\r\n");
    while(1)
    {
        ffmpeg->FFmpeg_RTSPPacketInit();
        ret = ffmpeg->FFmpeg_RTSPVideoSaveFrame();
        if(ret < 0)
        {
            printf("...parse fram fail~~~\r\n");
        }
        //printf("step3\r\n");
        while(0 == fifo->push(&(fifo->mbufferPara),ffmpeg->mVideoSrcPkt.data,ffmpeg->mVideoSrcPkt.size))
        {
             printf("......FIFO FULL AND SLEEP~~~~~~~\r\n");
        }
        ffmpeg->FFmpeg_RTSPpacketUnref();

    }
}

void tid3_cim_working(void)
{
    int ret = 0;
    long int data_size = 0;
    uint8_t *tmp;
    printf("mycim run...\r\n");
    V4l2dec_videoplay_init( tid_para.width_t,tid_para.height_t);
    while(1)
    {
        while((tmp = fifo->pop(&(fifo->mbufferPara),data_size)) == NULL)
        {
            printf("fifo is empty..\r\n");
        }
        //printf("datasize %ld...\r\n",data_size);
        ret = v4l2dec_handling_perfame(tmp,data_size);
        if(ret < 0)
        {
            printf("dec err,quit cycle\r\n");
            break;
        }
    }
    v4l2dec_stop();
}


