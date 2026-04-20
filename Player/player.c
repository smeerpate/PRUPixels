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

/* Houdt alle resources bij die vrijgemaakt moeten worden */
typedef struct
{
    AVFormatContext     *fmtCtx;
    AVCodecContext      *codecCtx;
    AVStream            *videoStream;
    AVFrame             *frame;
    AVFrame             *RGBFrame;
    uint8_t             *pixelBuffer;
    struct SwsContext   *swsCtx;
    void                *pruSharedMemPointer;
    int                  memFd;
} PlayerState;

/* Initialiseer de struct met veilige NULL/ongeldige waarden */
static void initPlayerState(PlayerState *s)
{
    s->fmtCtx            = NULL;
    s->codecCtx          = NULL;
    s->videoStream       = NULL;
    s->frame             = NULL;
    s->RGBFrame          = NULL;
    s->pixelBuffer       = NULL;
    s->swsCtx            = NULL;
    s->pruSharedMemPointer = NULL;
    s->memFd             = -1;
}

/* Ruimt alle resources op, ongeacht welke al dan niet geinitialiseerd zijn */
static void cleanupPlayerState(PlayerState *s)
{
    /* video cleanup controleert intern op NULL */
    cleanupVideo(s->fmtCtx, s->codecCtx, s->frame, s->RGBFrame, s->pixelBuffer, s->swsCtx);
 
    /* PRU cleanup enkel als geheugen gemapt was */
    if (s->pruSharedMemPointer != NULL && s->memFd >= 0)
        cleanupPRU(s->pruSharedMemPointer, s->memFd);
 
    /* Reset naar veilige waarden na cleanup */
    initPlayerState(s);
}

/* Speelt één video af van begin tot einde. Geeft 0 terug bij succes, -1 bij fout. */
static int runPlayer(const char *filename)
{
    PlayerState s;
    initPlayerState(&s);
 
    s.fmtCtx = initVideo(filename, &s.codecCtx, &s.videoStream);
    if (!s.fmtCtx)
    {
        printf("[ERROR] initVideo mislukt voor %s\n", filename);
        fflush(stdout);
        cleanupPlayerState(&s);
        return -1;
    }
 
    s.frame = av_frame_alloc();
    if (!s.frame)
    {
        printf("[ERROR] av_frame_alloc (frame) mislukt\n");
        fflush(stdout);
        cleanupPlayerState(&s);
        return -1;
    }
 
    s.RGBFrame = av_frame_alloc();
    if (!s.RGBFrame)
    {
        printf("[ERROR] av_frame_alloc (RGBFrame) mislukt\n");
        fflush(stdout);
        cleanupPlayerState(&s);
        return -1;
    }
 
    initScaler(s.codecCtx, s.RGBFrame, PIXELFIELD_WIDTH, PIXELFIELD_HEIGHT, &s.pixelBuffer, &s.swsCtx);
    if (!s.pixelBuffer || !s.swsCtx)
    {
        printf("[ERROR] initScaler mislukt\n");
        fflush(stdout);
        cleanupPlayerState(&s);
        return -1;
    }
 
    s.pruSharedMemPointer = initPRUSharedMem(&s.memFd);
    if (!s.pruSharedMemPointer)
    {
        printf("[ERROR] initPRUSharedMem mislukt\n");
        fflush(stdout);
        cleanupPlayerState(&s);
        return -1;
    }
 
    playVideo(s.fmtCtx, s.codecCtx, s.videoStream,
              s.frame, s.RGBFrame, s.swsCtx,
              s.pruSharedMemPointer,
              NPIXELSCONNECTED, PIXELFIELD_WIDTH, PIXELFIELD_HEIGHT);
 
    cleanupPlayerState(&s);
    return 0;
}

int main() 
{
    const char *filename1 = "/home/debian/PRUPixels/Player/video1.mp4";
	const char *filename2 = "/home/debian/PRUPixels/Player/video2.mp4";
	
	setGPIODirection(48, 1); // P9_14 as input

    while (1)
    {
        if (isPRURunning())
        {
            const char *filename = (readGPIO(48) == 0) ? filename1 : filename2;
            if (runPlayer(filename) != 0)
            {
                printf("[WARNING] runPlayer mislukt, wacht 3s voor herstart...\n");
                fflush(stdout);
                av_usleep(3000000);
            }
        }
        else
        {
            printf("[INFO] PRU is niet actief, wacht 3s...\n");
            fflush(stdout);
            av_usleep(3000000);
        }
    }
    return 0;
}

