#include "video.h"
#include "utils.h"
#include "pixelLUT.h"
#include <stdio.h>

AVFormatContext* initVideo(const char *filename, AVCodecContext *codecCtx, AVStream *videoStream)
{
    AVFormatContext *fmtCtx = NULL;
    if (avformat_open_input(&fmtCtx, filename, NULL, NULL) < 0)
	{
        printf("[ERROR] kon file %s niet openen\n", filename);
		fflush(stdout);
        return NULL;
    }
    if (avformat_find_stream_info(fmtCtx, NULL) < 0)
	{
        printf("[ERROR] Kon streaminfo niet vinden.\n");
		fflush(stdout);
        return NULL;
    }
    int videoStreamIndex = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (videoStreamIndex < 0)
	{
        printf("[ERROR] Kon geen video stream vinden.\n");
		fflush(stdout);
        return NULL;
    }
	
    videoStream = fmtCtx->streams[videoStreamIndex];
    AVCodecParameters *codecPar = videoStream->codecpar;
    AVCodec *codec = avcodec_find_decoder(codecPar->codec_id);
    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecPar);
    avcodec_open2(codecCtx, codec, NULL);
	
	AVRational framerate = av_guess_frame_rate(fmtCtx, videoStream, NULL);
	printf("[INFO] Video geïnitialiseerd: %s (%dpx x %dpx / %.2ffps).\n", filename, codecCtx->width, codecCtx->height, av_q2d(framerate));
	
    return fmtCtx;
}


struct SwsContext* initScaler(AVCodecContext *codecCtx, AVFrame *RGBFrame, int outWidth, int outHeight, uint8_t *pixelBuffer)
{
    struct SwsContext *swsCtx = sws_getContext(
		codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
		outWidth, outWidth, AV_PIX_FMT_RGB24,
		SWS_BILINEAR, NULL, NULL, NULL
	);
		
    int nBufferBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, outWidth, outHeight, 1);
	
    pixelBuffer = (uint8_t *)av_malloc(nBufferBytes);
    av_image_fill_arrays(RGBFrame->data, RGBFrame->linesize, pixelBuffer, AV_PIX_FMT_RGB24, outWidth, outHeight, 1);
	
    return swsCtx;
}


void playVideo(AVFormatContext *fmtCtx, AVCodecContext *codecCtx, AVStream *videoStream, AVFrame *frame, AVFrame *RGBFrame, struct SwsContext *swsCtx, 
				void *pruSharedMemPointer, int nPixelsToWrite, int pixelFieldWidth, int pixelFieldHeight)
{
	double playbackStartTime = av_gettime_relative() / 1000000.0; // in seconden
    AVPacket packet;
	
    while (av_read_frame(fmtCtx, &packet) >= 0)
	{
        if (packet.stream_index == videoStream->index)
		{
            if (avcodec_send_packet(codecCtx, &packet) == 0)
			{
                while (avcodec_receive_frame(codecCtx, frame) == 0) 
				{
                    sws_scale(swsCtx, (uint8_t const * const *)frame->data, frame->linesize, 0, codecCtx->height, RGBFrame->data, RGBFrame->linesize);
                    for (int i = 0; i < nPixelsToWrite; i++)
					{
                        uint32_t RGB;
                        getPixelRGB(RGBFrame, pixelLookupTable[i % TABLESIZE][0], pixelLookupTable[i % TABLESIZE][1], pixelFieldWidth, pixelFieldHeight, &RGB);
                        ((unsigned long *)pruSharedMemPointer)[i] = RGB;
                    }
					
                    double frameTimestamp = frame->pts * av_q2d(videoStream->time_base);
                    double currentTime = (av_gettime_relative() / 1000000.0) - playbackStartTime;
                    if (frameTimestamp > currentTime)
					{
                        double delay = frameTimestamp - currentTime;
                        av_usleep((int64_t)(delay * 1000000));
                    }
					else
					{
                        printf("[WARNING] video loopt niet in sync!\n");
						fflush(stdout);
                    }
                }
            }
        }
        av_packet_unref(&packet);
    }
}

void cleanupVideo(AVFormatContext *fmtCtx, AVCodecContext *codecCtx, AVFrame *frame, AVFrame *RGBFrame, uint8_t *pixelBuffer, struct SwsContext *swsCtx)
{
	av_free(pixelBuffer);
    av_frame_free(&frame);
    av_frame_free(&RGBFrame);
    sws_freeContext(swsCtx);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);
}
