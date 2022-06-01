#ifndef CYCLEBUFFER_H
#define CYCLEBUFFER_H
#include <iostream>
#include <string.h>

using namespace std;



#define  MAXSIZE  50
#define TRUE 1
#define FALSE 0
typedef struct
{
    int iSize;//数据的大小
    uint8_t *cdata;
    uint64_t time;
}bufData;
typedef struct
{
    bufData mdata[MAXSIZE];
    int iRead;//队头
    int iWrite;//队尾
}bufferPara;

class CycleBuffer
{
public:
    CycleBuffer(void);
    ~CycleBuffer(void);
    bool push(bufferPara *Q,uint8_t* src,int strLen,uint64_t time);//缓冲区存入

    uint8_t* pop(bufferPara *Q,long &dsLen,uint64_t &utime);//弹出数据

    bool InitQueue(bufferPara *Q);//初始化队列
    bufferPara  mbufferPara;
    uint8_t *dest;
private:
    uint8_t *item;

};


#endif // CYCLEBUFFER_H
