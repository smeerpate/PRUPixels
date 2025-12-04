#ifndef VIDEO_H
#define VIDEO_H

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>

AVFormatContext* initVideo(const char *filename, AVCodecContext *codecCtx, AVStream *videoStream);
struct SwsContext* initScaler(AVCodecContext *codecCtx, AVFrame *RGBFrame, int outWidth, int outHeight, uint8_t *pixelBuffer);
void playVideo(AVFormatContext *fmtCtx, AVCodecContext *codecCtx, AVStream *videoStream, AVFrame *frame, AVFrame *RGBFrame, struct SwsContext *swsCtx,
                void *pruSharedMemPointer, int nPixelsToWrite, int pixelFieldWidth, int pixelFieldHeight);
void cleanupVideo(AVFormatContext *fmtCtx, AVCodecContext *codecCtx, AVFrame *frame, AVFrame *RGBFrame, uint8_t *pixelBuffer, struct SwsContext *swsCtx);

#endif // VIDEO_H
