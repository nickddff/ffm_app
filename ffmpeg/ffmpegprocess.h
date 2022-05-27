#ifndef FFMPEGPROCESS_H
#define FFMPEGPROCESS_H

#include <string>
#include <vector>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

using namespace std;

class FFmpegProcess
{
public:
    //explicit FFmpegProcess(QObject *parent = nullptr);

    FFmpegProcess();
    virtual ~FFmpegProcess();

public:
    int FFmpeg_GetVersion(void);
public:
    //解码视频流数据
    //@param inbuf 视频裸流数据
    //@param inbufSize 视频裸流数据大小
    //@param framePara 接收帧参数数组：{width,height,linesize1,linesiz2,linesize3}
    //@param outRGBBuf 输出RGB数据(若已申请内存)
    //@param outYUVBuf 输出YUV数据(若已申请内存)
    //@return 成功返回解码数据帧大小，否则<=0
    //return 0:暂未收到解码数据，-1：解码失败，1：解码成功
    //int FFmpeg_H264Decode(unsigned char * inbuf, int inbufSize, int *framePara, unsigned char *outRGBBuf, unsigned char **outYUVBuf);
    //自定义重载函数
    //int FFmpeg_H264Decode(unsigned char * inbuf, int inbufSize);
    //以下函数代码适合 FFmpeg3.4.2"Cantor"和3.3.7"Hilbert"等版本
    //int FFmpeg_VideoDecoderInit(AVCodecParameters *ctx);
    //int FFmpeg_H264DecoderInit(void);
    //int FFmpeg_VideoDecoderRelease(void);
    //int FFmpeg_H264Decode(unsigned char * inbuf, int inbufSize, int *framePara, unsigned char *outRGBBuf, unsigned char **outYUVBuf);
public:
    //配置初始化参数RTSP
    int FFmpeg_RTSPVideoInit(const char* url,int &width_t,int &height_t);
    //H264码流解码和显示
    int FFmpeg_RTSPVideoProcess(void);
    //关闭RTSP拉流功能
    int FFmpeg_RTSPVideoClose(void);
    //视频存储结束
    int FFmpeg_RTSPVideoFileSaveClose();
    //保存jpg图像
    int FFmpeg_SaveImageJPG(const char* filePath);
    //将AVFrame保存为JPEG格式的数据
    int FFmpeg_GetImageRGB(uint8_t* imageData, int &imageSize);
    //获取红外数据
    int FFmpeg_GetIRData(uint8_t* imageData, int &imageSize, int timeout);
    //get frame packet
    int FFmpeg_RTSPVideoSaveFrame(void);
    //packet unref
    void FFmpeg_RTSPpacketUnref(void);
    //packet init
    void FFmpeg_RTSPPacketInit(void);
    //int RegisterFFmpegCallback(pCallbackFunction callback, LPVOID param);
public:
    //vector<byte> getJPGImageData;
    //vector<byte> getTIFImageData;
    //vector<byte> getFFFImageData;
    //图像分辨率
    int ImageWidth = 640;
    int ImageHeight = 512;
    //定义save name eg: xx.h264
    const char *fileNamePath;
    AVPacket mVideoSrcPkt;

private:
    //--------------FFmpeg-------------------
    //byte *outBuffer;
    int outBufferSize;
    struct AVFrame *pFrameRGB;
    //struct AVFrame* pFrameSaveImage;
    struct SwsContext* pImageConvertCtx;
    //    struct AVPacket mPacket;
    //    struct AVCodecContext *pAVCodecCtx;
    //定义是否开始保存视频帧数据标记
    bool isSaveVideoFrameFlag;
    //定义是否存在新的一帧图像数据标记
    bool isHaveImageFlag;
    AVDictionary *avdic=NULL;
    //定义指针变量
    //pCallbackFunction m_pImageData = NULL;
    //LPVOID m_param = NULL;

private:
    //存储视频文件执行函数
    int FFmpeg_RTSPVideoSaveFile(void);public:
private:
    //定义视频文件保存相关的变量
    //video
    /*src视频显示和图像保存相关变量*/
    AVFormatContext* mpVideoSrcFormatCxt;
    AVCodec* mpVideoSrcCodec;
    AVCodecContext* mpVideoSrcCodecCxt;
    AVFrame *mpDecodedFrame;
    AVStream* mpVideoSrcStream;
    /*dst视频存储相关变量*/
    AVFormatContext* mpVideoDstFormatCxt;
    AVCodec* mpVideoDstCodec;
    AVCodecContext* mpVideoDstCodecCxt;
    AVFrame* mpVideoDstFrame;
    AVPacket mVideoDstPkt;
    AVStream* mpVideoDstStream;
    //输出信息
    AVOutputFormat* mpOutFormat;
    AVFormatContext* mpOutFormatCxt;
    int mVideoStreamIdx;

    //    //audio
    //    /*src*/
    //    AVFormatContext* mpAudioSrcFormatCxt;
    //    AVCodec* mpAudioSrcCodec;
    //    AVCodecContext* mpAudioSrcCodecCxt;
    //    AVPacket mAudioSrcPkt;
    //    /*dst*/
    //    AVFormatContext* mpAudioDstFormatCxt;
    //    AVCodec* mpAudioDstCodec;
    //    AVCodecContext* mpAudioDstCodecCxt;
    //    AVFrame* mpAudioDstFrame;
    //    AVPacket mAduioDstPkt;
    //    AVStream* mpAudioDstStream;
    //    int mAudioStreamIdx;
    //flags
    int miHeaderWriteFlag;
    int miVideoInit;
    //    int miAudioInit;
    //    int miOutInit;
    int miCount;
    //    long lStartTick;
    //    int miVideoFrame;
    int64_t start_pts;
    int64_t start_dts;
    int last_pts;
    int last_dts;
};

#endif // FFMPEGPROCESS_H
