#include "cyclebuffer.h"
#include <pthread.h>

static pthread_mutex_t mutex;

CycleBuffer::CycleBuffer(void)
{
    InitQueue(&mbufferPara);
    dest = new uint8_t[1024*100];
    printf("...........buffer init........~~~~~~~\r\n");
    memset(dest,0,1024*100);
}

CycleBuffer::~CycleBuffer(void)
{
    for (int i=0;i<MAXSIZE;i++)
    {
        delete[] mbufferPara.mdata[i].cdata;
    }
}

bool CycleBuffer::push(bufferPara *Q,uint8_t* src,int srcLen)
{
    pthread_mutex_lock(&mutex);
    if ((Q->iWrite+1)%MAXSIZE!=Q->iRead)
    {
        memcpy(Q->mdata[Q->iWrite].cdata,src,srcLen);
        //Q->mdata[Q->iWrite].cdata = src;
        Q->mdata[Q->iWrite].iSize = srcLen;
        Q->iWrite = (Q->iWrite+1)%MAXSIZE;
    //     if(Q->iWrite > Q->iRead)
    //    {
    //       printf("have1:%d\r\n",(Q->iWrite-Q->iRead));
    //    }
    //    else printf("have2:%d\r\n",50-(Q->iRead - Q->iWrite));
        pthread_mutex_unlock(&mutex);
        return TRUE;
    }
    else
    {
        pthread_mutex_unlock(&mutex);
        return FALSE;
    }

}

uint8_t* CycleBuffer::pop(bufferPara *Q,long &dstLen)
{
    pthread_mutex_lock(&mutex);
    if (Q->iRead!=Q->iWrite)
    {
        //dest = Q->mdata[Q->iRead].cdata;
        dstLen = Q->mdata[Q->iRead].iSize;
        //printf("iread:%d,isize:%ld",Q->iRead,dstLen);
        memcpy(dest,Q->mdata[Q->iRead].cdata,dstLen);
        Q->iRead = (Q->iRead+1)%MAXSIZE;
        pthread_mutex_unlock(&mutex);
        return dest;
    }
    else
    {
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

}

bool CycleBuffer::InitQueue(bufferPara * Q)
{
    Q->iRead =0;
    Q->iWrite=0;
    for (int i=0;i<MAXSIZE;i++)
    {
        Q->mdata[i].cdata = new uint8_t[1024*100];
        memset(Q->mdata[i].cdata,0,1024*100);
        Q->mdata[i].iSize = 0;
    }
    return TRUE;
}
