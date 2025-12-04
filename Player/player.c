/*


gcc -o player player.c video.c pru.c utils.c pixelLUT.c $(pkg-config --cflags --libs libavformat libavcodec libswscale libavutil)

sudo ./player

To install this file use this command:
	sudo install -m 755 player /usr/local/sbin/

*/

#include "video.h"
#include "pru.h"
#include "utils.h"

#define PIXELFIELD_WIDTH 150
#define PIXELFIELD_HEIGHT 150
#define NPIXELSCONNECTED 1200

int main() 
{
    const char *filename = "/home/debian/PRUPixels/Player/video.mp4";

    while (1)
	{
        if (isPRURunning()) 
		{
            AVCodecContext *codecCtx;
            AVStream *videoStream;
            AVFormatContext *fmtCtx = initVideo(filename, &codecCtx, &videoStream);
			
			printf("[INFO] Codec context: bitrate:%d, size:(%d x %d).\n", codecCtx->bit_rate, codecCtx->width, codecCtx->height);
			printf("[INFO] AV Stream: id:%d, stream index:%d.\n", videoStream->id, videoStream->index);
			fflush(stdout);
			
            AVFrame *frame = av_frame_alloc();
            AVFrame *RGBFrame = av_frame_alloc();
			
            uint8_t *pixelBuffer;
            struct SwsContext *swsCtx;
			initScaler(codecCtx, RGBFrame, PIXELFIELD_WIDTH, PIXELFIELD_HEIGHT, &pixelBuffer, &swsCtx);

            int memFd;
            void *pruSharedMemPointer = initPRUSharedMem(&memFd);

            playVideo(fmtCtx, codecCtx, videoStream, frame, RGBFrame, swsCtx, pruSharedMemPointer, NPIXELSCONNECTED, PIXELFIELD_WIDTH, PIXELFIELD_HEIGHT);

            cleanupVideo(fmtCtx, codecCtx, frame, RGBFrame, pixelBuffer, swsCtx);
            cleanupPRU(pruSharedMemPointer, memFd);
        }
		else
		{
            av_usleep(3000000);
        }
    }
    return 0;
}

