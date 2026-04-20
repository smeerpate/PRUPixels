/**
 * @file player.c
 * @brief Hoofdprogramma van de PRUPixels video player.
 *
 * Leest 4 GPIO ingangen om te kiezen welke video afgespeeld wordt (1..16),
 * decodeert MP4 frames via FFmpeg en schrijft de pixeldata naar de
 * PRU shared memory zodat de PRU de WS2812 LEDs kan aansturen.
 * Twee status LEDs geven via knipperpatronen de spelerstatus,
 * het actieve filmnummer en eventuele foutcodes weer.
 *
 * Compileren:
 * @code
 * gcc -o player player.c video.c pru.c utils.c pixelLUT.c io.c \
 *     $(pkg-config --cflags --libs libavformat libavcodec libswscale libavutil) \
 *     -lpthread
 * @endcode
 *
 * Installeren:
 * @code
 * sudo install -m 755 player /usr/local/sbin/
 * @endcode
 */

#include "video.h"
#include "pru.h"
#include "utils.h"
#include "io.h"
#include <stdio.h>

/** @brief Breedte van het geschaalde videoframe in pixels. */
#define PIXELFIELD_WIDTH    150

/** @brief Hoogte van het geschaalde videoframe in pixels. */
#define PIXELFIELD_HEIGHT   150

/** @brief Aantal fysiek aangesloten WS2812 LEDs. */
#define NPIXELSCONNECTED    1200

/** @brief Buildinfo string, automatisch ingevuld door de compiler. */
#define BUILDINFO           __DATE__ " " __TIME__

/**
 * @brief Houdt alle FFmpeg en PRU resources bij voor één afspeelcyclus.
 *
 * Door alle pointers te bundelen in één struct kan cleanupPlayerState()
 * altijd veilig worden aangeroepen, ook als de initialisatie halverwege
 * mislukt.
 */
typedef struct {
    AVFormatContext     *fmtCtx;             /**< FFmpeg format context.              */
    AVCodecContext      *codecCtx;           /**< FFmpeg codec context.               */
    AVStream            *videoStream;        /**< Videostream binnen fmtCtx.          */
    AVFrame             *frame;              /**< Ruwe gedecodeerde framedata.        */
    AVFrame             *RGBFrame;           /**< Geschaalde RGB framedata.           */
    uint8_t             *pixelBuffer;        /**< Geheugen voor RGBFrame pixeldata.   */
    struct SwsContext   *swsCtx;             /**< FFmpeg scaler context.              */
    void                *pruSharedMemPointer;/**< Pointer naar PRU shared memory.     */
    int                  memFd;             /**< Bestandsdescriptor van /dev/mem.    */
} PlayerState;

/**
 * @brief Initialiseert een PlayerState struct met veilige nulwaarden.
 *
 * Alle pointers worden op NULL gezet en memFd op -1, zodat
 * cleanupPlayerState() altijd veilig kan controleren wat al dan
 * niet geïnitialiseerd is.
 *
 * @param s Pointer naar de te initialiseren PlayerState.
 */
static void initPlayerState(PlayerState *s)
{
    s->fmtCtx               = NULL;
    s->codecCtx             = NULL;
    s->videoStream          = NULL;
    s->frame                = NULL;
    s->RGBFrame             = NULL;
    s->pixelBuffer          = NULL;
    s->swsCtx               = NULL;
    s->pruSharedMemPointer  = NULL;
    s->memFd                = -1;
}

/**
 * @brief Geeft alle resources van een PlayerState vrij.
 *
 * Roept cleanupVideo() en cleanupPRU() aan ongeacht welke resources
 * al dan niet geïnitialiseerd zijn. Reset de struct daarna opnieuw
 * via initPlayerState() zodat hergebruik veilig is.
 *
 * @param s Pointer naar de vrij te maken PlayerState.
 */
static void cleanupPlayerState(PlayerState *s)
{
    cleanupVideo(s->fmtCtx, s->codecCtx, s->frame, s->RGBFrame, s->pixelBuffer, s->swsCtx);

    if (s->pruSharedMemPointer != NULL && s->memFd >= 0)
        cleanupPRU(s->pruSharedMemPointer, s->memFd);

    initPlayerState(s);
}

/**
 * @brief Initialiseert alle resources en speelt één video af.
 *
 * Doorloopt de volledige initialisatieketen: video openen, frames
 * alloceren, scaler initialiseren en PRU shared memory mappen.
 * Bij elke fout worden reeds gealloceerde resources vrijgegeven
 * via cleanupPlayerState(), wordt de bijhorende foutcode ingesteld
 * via setErrorCode() en wordt LED_STATUS_ERROR geactiveerd.
 * Bij succes wordt LED_STATUS_PLAYING ingesteld en knippert de
 * film LED het opgegeven filmnummer.
 *
 * @param filename   Volledig pad naar het af te spelen MP4 bestand.
 * @param filmNumber Filmnummer (1..16) voor de film indicator LED.
 * @return 0 bij succes, -1 bij fout.
 */
static int runPlayer(const char *filename, int filmNumber)
{
    PlayerState s;
    initPlayerState(&s);

    s.fmtCtx = initVideo(filename, &s.codecCtx, &s.videoStream);
    if (!s.fmtCtx)
    {
        printf("[ERROR] initVideo mislukt voor %s\n", filename);
        fflush(stdout);
		setErrorCode(LED_ERR_VIDEO);
        setStatusLED(LED_STATUS_ERROR);
        cleanupPlayerState(&s);
        return -1;
    }

    s.frame = av_frame_alloc();
    if (!s.frame)
    {
        printf("[ERROR] av_frame_alloc (frame) mislukt\n");
        fflush(stdout);
		setErrorCode(LED_ERR_FRAME);
        setStatusLED(LED_STATUS_ERROR);
        cleanupPlayerState(&s);
        return -1;
    }

    s.RGBFrame = av_frame_alloc();
    if (!s.RGBFrame)
    {
        printf("[ERROR] av_frame_alloc (RGBFrame) mislukt\n");
        fflush(stdout);
		setErrorCode(LED_ERR_FRAME);
        setStatusLED(LED_STATUS_ERROR);
        cleanupPlayerState(&s);
        return -1;
    }

    initScaler(s.codecCtx, s.RGBFrame, PIXELFIELD_WIDTH, PIXELFIELD_HEIGHT, &s.pixelBuffer, &s.swsCtx);
    if (!s.pixelBuffer || !s.swsCtx)
    {
        printf("[ERROR] initScaler mislukt\n");
        fflush(stdout);
		setErrorCode(LED_ERR_SCALER);
        setStatusLED(LED_STATUS_ERROR);
        cleanupPlayerState(&s);
        return -1;
    }

    s.pruSharedMemPointer = initPRUSharedMem(&s.memFd);
    if (!s.pruSharedMemPointer)
    {
        printf("[ERROR] initPRUSharedMem mislukt\n");
        fflush(stdout);
		setErrorCode(LED_ERR_PRU);
        setStatusLED(LED_STATUS_ERROR);
        cleanupPlayerState(&s);
        return -1;
    }

    /* Alles OK: statusLED aan, filmLED knippert filmnummer */
    setErrorCode(LED_ERR_NONE);
    setFilmNumber(filmNumber);
    setStatusLED(LED_STATUS_PLAYING);

    playVideo(s.fmtCtx, s.codecCtx, s.videoStream,
              s.frame, s.RGBFrame, s.swsCtx,
              s.pruSharedMemPointer,
              NPIXELSCONNECTED, PIXELFIELD_WIDTH, PIXELFIELD_HEIGHT);

    cleanupPlayerState(&s);
    return 0;
}

/**
 * @brief Programmaingang van de PRUPixels player.
 *
 * Initialiseert de GPIO ingangen voor filmkeuze en start de LED thread.
 * Wacht in een oneindige lus tot de PRU actief is, leest het filmnummer
 * via 4 GPIO ingangen (GPIO_FILM_BIT0..3) en roept runPlayer() aan.
 * Bij een fout of een niet-actieve PRU wordt 3 seconden gewacht voor
 * de volgende poging.
 *
 * @return Wordt normaal nooit bereikt. Geeft 0 terug bij een onverwacht
 *         einde van de hoofdlus.
 */
int main()
{
    printf("[INFO] PRUPixels Player wordt gestart. (Build = %s).\n", BUILDINFO);
    fflush(stdout);

    /** @brief Paden naar de beschikbare videobestanden. */
    const char *filenames[] = {
        "/home/debian/PRUPixels/Player/video1.mp4",
        "/home/debian/PRUPixels/Player/video2.mp4",
		"/home/debian/PRUPixels/Player/video3.mp4",
        "/home/debian/PRUPixels/Player/video4.mp4",
		"/home/debian/PRUPixels/Player/video5.mp4",
        "/home/debian/PRUPixels/Player/video6.mp4",
		"/home/debian/PRUPixels/Player/video7.mp4",
        "/home/debian/PRUPixels/Player/video8.mp4",
		"/home/debian/PRUPixels/Player/video9.mp4",
        "/home/debian/PRUPixels/Player/video10.mp4",
		"/home/debian/PRUPixels/Player/video11.mp4",
        "/home/debian/PRUPixels/Player/video12.mp4",
		"/home/debian/PRUPixels/Player/video13.mp4",
        "/home/debian/PRUPixels/Player/video14.mp4",
		"/home/debian/PRUPixels/Player/video15.mp4",
        "/home/debian/PRUPixels/Player/video16.mp4",
    };

	/* Initialiseer alle 4 filmkeuze ingangen */
	setGPIODirection(GPIO_FILM_BIT0, 1);
	setGPIODirection(GPIO_FILM_BIT1, 1);
	setGPIODirection(GPIO_FILM_BIT2, 1);
	setGPIODirection(GPIO_FILM_BIT3, 1);
	
    startLEDThread();

    while (1)
    {
        if (isPRURunning())
        {
			int filmNumber = readFilmNumber();
            printf("[INFO] Filmnummer %d geselecteerd.\n", filmNumber);
            fflush(stdout);
			
            setStatusLED(LED_STATUS_IDLE);

            if (runPlayer(filenames[filmNumber - 1], filmNumber) != 0)
            {
                printf("[WARNING] runPlayer mislukt, wacht 3s voor herstart...\n");
                fflush(stdout);
                av_usleep(3000000);
            }
        }
        else
        {
            setStatusLED(LED_STATUS_IDLE);
            printf("[INFO] PRU is niet actief, wacht 3s...\n");
            fflush(stdout);
            av_usleep(3000000);
        }
    }

	/* Wordt normaal nooit bereikt */
    stopLEDThread();
    return 0;
}
