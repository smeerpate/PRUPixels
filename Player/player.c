/*


gcc -o player player.c video.c pru.c utils.c pixelLUT.c io.c $(pkg-config --cflags --libs libavformat libavcodec libswscale libavutil)

sudo ./player

To install this file use this command:
	sudo install -m 755 player /usr/local/sbin/

*/

#include "video.h"
#include "pru.h"
#include "utils.h"
#include "io.h"

#define PIXELFIELD_WIDTH 150
#define PIXELFIELD_HEIGHT 150
#define NPIXELSCONNECTED 1200

int main() 
{
    const char *filename1 = "/home/debian/PRUPixels/Player/video1.mp4";
	const char *filename2 = "/home/debian/PRUPixels/Player/video2.mp4";
	setGPIODirection(38, 1); // P8_3 as input

    while (1)
	{
        if (isPRURunning()) 
		{
            AVCodecContext *codecCtx;
            AVStream *videoStream;
			AVFormatContext *fmtCtx;
			
			if (readGPIO(38) == 0)
				fmtCtx = initVideo(filename1, &codecCtx, &videoStream);
			else
				fmtCtx = initVideo(filename2, &codecCtx, &videoStream);
			
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

