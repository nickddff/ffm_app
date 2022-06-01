#ifndef MYTHREAD_H
#define MYTHREAD_H


extern "C"{
   #include "v4l2h264dec.h"
}



#include "cyclebuffer.h"
#include "ffmpegprocess.h"

typedef struct{
   int width_t;
   int height_t;
}TID_PARA;

extern FFmpegProcess *ffmpeg;
extern int getParaFlag;

extern "C" void tid2_ffm_working(int flag);
extern "C" void tid3_cim_working(void);
extern "C" int tid1_ffm_init(char *ip,char *filename);

#endif // MYTHREAD_H
