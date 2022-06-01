#define _GNU_SOURCE     //在源文件开头定义_GNU_SOURCE宏
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include "config.h"

int utty_init(char *device,int baudrate);
static struct termios old_cfg;  //用于保存终端的配置参数
static int fd;      //串口终端对应的文件描述符



extern int wifi_init(char **ip_str);
extern int tid1_ffm_init(char *ip,char *filename);
extern void tid2_ffm_working(int flag);
extern void tid3_cim_working(void);

static void *new_thread1_start(void *arg)
{
    char *ip_str;

    #if RTSP_BY_WLAN
    if(wifi_init(&ip_str) == -1)
    {
        pthread_exit((void *)1);
    }
    #endif
    
    tid1_ffm_init(ip_str,"video.264");
    pthread_exit((void *)0);
}

static void *new_thread2_start(void *arg)
{
    int ret;
    int init_flag = *((int *)arg);
    ret = pthread_detach(pthread_self());
    if(ret){
        fprintf(stderr, "pthread_detach error: %s\n", strerror(ret));
        return NULL;
    }
    tid2_ffm_working(init_flag);
    pthread_exit((void *)0);
}

static void *new_thread3_start(void *arg)
{
    int ret;
    ret = pthread_detach(pthread_self());
    if(ret){
        fprintf(stderr, "pthread_detach error: %s\n", strerror(ret));
        return NULL;
    }
    tid3_cim_working();
    pthread_exit((void *)0);
}

int main(int argc, char *argv[])
{
    pthread_t tid1,tid2_ffm,tid3_cim;
    int ret;
    void *tret;

    ret = pthread_create(&tid1,NULL,new_thread1_start,NULL);
    if(ret){
        fprintf(stderr, "pthread1_create error: %s\n", strerror(ret));
        exit(-1);
    }
    
    ret = pthread_join(tid1,&tret);
    if(ret){
        fprintf(stderr, "pthread1 error: %s\n", strerror(ret));
        exit(-1);
    }
    printf("tid1 receive okey\r\n");

    if(((int)tret) == 1 )   //这里的用法很奇怪
    {
        exit(-1);
    }


    // ret = utty_init("/dev/tty0",115200);
    // if(ret == -1)
    // {
    //     fprintf(stderr,"ttyusart error :%s\n",strerror(ret));
    // }
    int init_flag = 1;
    ret = pthread_create(&tid2_ffm,NULL,new_thread2_start,(void *)&init_flag);
    if(ret){
        fprintf(stderr, "pthread2_create error: %s\n", strerror(ret));
        exit(-1);
    }

    ret = pthread_create(&tid3_cim,NULL,new_thread3_start,tret);
    if(ret){
        fprintf(stderr, "pthread3_create error: %s\n", strerror(ret));
        exit(-1);
    }    
    while(1){
        
    }
    return 0;
}



typedef struct uart_hardware_cfg {
    unsigned int baudrate;      /* 波特率 */
    unsigned char dbit;         /* 数据位 */
    char parity;                /* 奇偶校验 */
    unsigned char sbit;         /* 停止位 */
} uart_cfg_t;

/**
 ** 串口初始化操作
 ** 参数device表示串口终端的设备节点
 **/
static int uart_init(const char *device)
{
    /* 打开串口终端 */
    fd = open(device, O_RDWR | O_NOCTTY);
    if (0 > fd) {
        fprintf(stderr, "open error: %s: %s\n", device, strerror(errno));
        return -1;
    }

    /* 获取串口当前的配置参数 */
    if (0 > tcgetattr(fd, &old_cfg)) {
        fprintf(stderr, "tcgetattr error: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    return 0;
}

/**
 ** 串口配置
 ** 参数cfg指向一个uart_cfg_t结构体对象
 **/
static int uart_cfg(const uart_cfg_t *cfg)
{
    struct termios new_cfg = {0};   //将new_cfg对象清零
    speed_t speed;

    /* 设置为原始模式 */
    cfmakeraw(&new_cfg);

    /* 使能接收 */
    new_cfg.c_cflag |= CREAD;

    /* 设置波特率 */
    switch (cfg->baudrate) {
    case 1200: speed = B1200;
        break;
    case 1800: speed = B1800;
        break;
    case 2400: speed = B2400;
        break;
    case 4800: speed = B4800;
        break;
    case 9600: speed = B9600;
        break;
    case 19200: speed = B19200;
        break;
    case 38400: speed = B38400;
        break;
    case 57600: speed = B57600;
        break;
    case 115200: speed = B115200;
        break;
    case 230400: speed = B230400;
        break;
    case 460800: speed = B460800;
        break;
    case 500000: speed = B500000;
        break;
    default:    //默认配置为115200
        speed = B115200;
        printf("default baud rate: 115200\n");
        break;
    }

    if (0 > cfsetspeed(&new_cfg, speed)) {
        fprintf(stderr, "cfsetspeed error: %s\n", strerror(errno));
        return -1;
    }

    /* 设置数据位大小 */
    new_cfg.c_cflag &= ~CSIZE;  //将数据位相关的比特位清零
    switch (cfg->dbit) {
    case 5:
        new_cfg.c_cflag |= CS5;
        break;
    case 6:
        new_cfg.c_cflag |= CS6;
        break;
    case 7:
        new_cfg.c_cflag |= CS7;
        break;
    case 8:
        new_cfg.c_cflag |= CS8;
        break;
    default:    //默认数据位大小为8
        new_cfg.c_cflag |= CS8;
        printf("default data bit size: 8\n");
        break;
    }

    /* 设置奇偶校验 */
    switch (cfg->parity) {
    case 'N':       //无校验
        new_cfg.c_cflag &= ~PARENB;
        new_cfg.c_iflag &= ~INPCK;
        break;
    case 'O':       //奇校验
        new_cfg.c_cflag |= (PARODD | PARENB);
        new_cfg.c_iflag |= INPCK;
        break;
    case 'E':       //偶校验
        new_cfg.c_cflag |= PARENB;
        new_cfg.c_cflag &= ~PARODD; /* 清除PARODD标志，配置为偶校验 */
        new_cfg.c_iflag |= INPCK;
        break;
    default:    //默认配置为无校验
        new_cfg.c_cflag &= ~PARENB;
        new_cfg.c_iflag &= ~INPCK;
        printf("default parity: N\n");
        break;
    }

    /* 设置停止位 */
    switch (cfg->sbit) {
    case 1:     //1个停止位
        new_cfg.c_cflag &= ~CSTOPB;
        break;
    case 2:     //2个停止位
        new_cfg.c_cflag |= CSTOPB;
        break;
    default:    //默认配置为1个停止位
        new_cfg.c_cflag &= ~CSTOPB;
        printf("default stop bit size: 1\n");
        break;
    }

    /* 将MIN和TIME设置为0 */
    new_cfg.c_cc[VTIME] = 0;
    new_cfg.c_cc[VMIN] = 0;

    /* 清空缓冲区 */
    if (0 > tcflush(fd, TCIOFLUSH)) {
        fprintf(stderr, "tcflush error: %s\n", strerror(errno));
        return -1;
    }

    /* 写入配置、使配置生效 */
    if (0 > tcsetattr(fd, TCSANOW, &new_cfg)) {
        fprintf(stderr, "tcsetattr error: %s\n", strerror(errno));
        return -1;
    }

    /* 配置OK 退出 */
    return 0;
}



/**
 ** 信号处理函数，当串口有数据可读时，会跳转到该函数执行
 **/
static void io_handler(int sig, siginfo_t *info, void *context)
{
    unsigned char buf[10] = {0};
    int ret;
    int n;

    if(SIGRTMIN != sig)
        return;

    /* 判断串口是否有数据可读 */
    if (POLL_IN == info->si_code) {
        ret = read(fd, buf, 8);     //一次最多读8个字节数据
        printf("[ ");
        for (n = 0; n < ret; n++)
            printf("0x%hhx ", buf[n]);
        printf("]\n");
    }
}

/**
 ** 异步I/O初始化函数
 **/
static void async_io_init(void)
{
    struct sigaction sigatn;
    int flag;

    /* 使能异步I/O */
    flag = fcntl(fd, F_GETFL);  //使能串口的异步I/O功能
    flag |= O_ASYNC;
    fcntl(fd, F_SETFL, flag);

    /* 设置异步I/O的所有者 */
    fcntl(fd, F_SETOWN, getpid());

    /* 指定实时信号SIGRTMIN作为异步I/O通知信号 */
    fcntl(fd, F_SETSIG, SIGRTMIN);

    /* 为实时信号SIGRTMIN注册信号处理函数 */
    sigatn.sa_sigaction = io_handler;   //当串口有数据可读时，会跳转到io_handler函数
    sigatn.sa_flags = SA_SIGINFO;
    sigemptyset(&sigatn.sa_mask);
    sigaction(SIGRTMIN, &sigatn, NULL);
}

int utty_init(char *device,int baudrate)
{
    uart_cfg_t cfg = {0};
    unsigned char w_buf[10] = {0x11, 0x22, 0x33, 0x44,
                0x55, 0x66, 0x77, 0x88};    //通过串口发送出去的数据

    device = "/dev/tty0";
    cfg.baudrate = baudrate;
    cfg.dbit = '8';
    cfg.parity = 'N';
    cfg.sbit = 1;

    /* 串口初始化 */
    if (uart_init(device))return -1;

    /* 串口配置 */
    if (uart_cfg(&cfg)) {
        tcsetattr(fd, TCSANOW, &old_cfg);   //恢复到之前的配置
        close(fd);
        return -1;
    }
    async_io_init();	//我们使用异步I/O方式读取串口的数据，调用该函数去初始化串口的异步I/O
    /* 读|写串口 */
 
    return 0;
    /* 退出 */
    //tcsetattr(fd, TCSANOW, &old_cfg);   //恢复到之前的配置
    //close(fd);
    //exit(EXIT_SUCCESS);
}
