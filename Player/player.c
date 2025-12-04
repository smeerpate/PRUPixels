/*


gcc -o player player.c video.c pru.c utils.c pixelLUT.c $(pkg-config --cflags --libs libavformat libavcodec libswscale libavutil)

sudo ./player

To install this file use this command:
	sudo install -m 755 player /usr/local/sbin/

*/

#include "video.h"
#include "pru.h"
#include "utils.h"

int main() 
{
    const char *filename = "/home/debian/PRUPixels/video.mp4";

    while (1)
	{
        if (isPRURunning()) 
		{
            AVCodecContext *codecCtx;
            AVStream *videoStream;
            AVFormatContext *fmtCtx = initVideo(filename, &codecCtx, &videoStream);

            AVFrame *frame = av_frame_alloc();
            AVFrame *RGBFrame = av_frame_alloc();
            uint8_t *pixelBuffer;
            struct SwsContext *swsCtx = initScaler(codecCtx, RGBFrame, 150, 150, &pixelBuffer);

            int memFd;
            void *pruSharedMemPointer = initPRUSharedMem(&memFd);

            playVideo(fmtCtx, codecCtx, videoStream, frame, RGBFrame, swsCtx, pruSharedMemPointer);

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

