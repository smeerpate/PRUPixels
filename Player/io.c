/**
 * @file io.c
 * @brief Implementatie van GPIO aansturing en LED status thread.
 *
 * De LED thread draait op de achtergrond en stuurt twee LEDs aan
 * op basis van gedeelde toestandsvariabelen. Een mutex beschermt
 * de gedeelde variabelen tegen race conditions tussen threads.
 */

#include "io.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

/** @defgroup blink_timings Knippertimings in microseconden
 * @{
 */
#define BLINK_SHORT_ON      100000  /**< 100ms aan  — één knipperpuls.          */
#define BLINK_SHORT_OFF     200000  /**< 200ms uit  — pauze tussen pulsen.      */
#define BLINK_PAUSE         800000  /**< 800ms pauze na volledig knipperpatroon. */
#define BLINK_SLOW_ON       500000  /**< 500ms aan  — langzaam knipperen (IDLE). */
#define BLINK_SLOW_OFF      500000  /**< 500ms uit  — langzaam knipperen (IDLE). */
#define BLINK_FAST_ON        80000  /**<  80ms aan  — snel knipperen (ERROR).    */
#define BLINK_FAST_OFF       80000  /**<  80ms uit  — snel knipperen (ERROR).    */
/** @} */

/**
 * @defgroup led_state Gedeelde toestand tussen hoofdthread en LED thread
 *
 * Deze variabelen worden beschermd door @ref ledMutex.
 * @{
 */
static volatile LedStatus   currentStatus     = LED_STATUS_IDLE; /**< Huidige spelerstatus.  */
static volatile int         currentFilmNumber = 1;               /**< Huidig filmnummer.     */
static volatile int         ledThreadRunning  = 0;               /**< 1 = thread actief.     */
/** @} */

static pthread_t            ledThread;                           /**< Handle van de LED thread.      */
static pthread_mutex_t      ledMutex = PTHREAD_MUTEX_INITIALIZER; /**< Mutex voor gedeelde toestand. */

/* ------------------------------------------------------------------ */
/* GPIO hulpfuncties                                                   */
/* ------------------------------------------------------------------ */

/**
 * @brief Stelt de richting in van een GPIO pin via sysfs.
 *
 * @param gpioNr    GPIO nummer (bv. 48 voor P9_14).
 * @param direction 0 = uitgang, anders = ingang.
 */
void setGPIODirection(int gpioNr, int direction)
{
    char path[64];
    sprintf(path, "/sys/class/gpio/gpio%d/direction", gpioNr);

    FILE *fp = fopen(path, "w");
    if (fp == NULL)
    {
        printf("[ERROR] kon file %s niet openen om direction te schrijven.\n", path);
        fflush(stdout);
        return;
    }

    fprintf(fp, direction == 0 ? "out" : "in");
    fclose(fp);
}

/**
 * @brief Leest de logische waarde van een GPIO ingang via sysfs.
 *
 * De ASCII tekens '0' en '1' uit sysfs worden omgezet naar
 * de numerieke waarden 0 en 1.
 *
 * @param gpioNr GPIO nummer van de in te lezen pin.
 * @return 0 of 1 bij succes, 0 bij fout of onbekende waarde.
 */
char readGPIO(int gpioNr)
{
    char path[64];
    char value;

    sprintf(path, "/sys/class/gpio/gpio%d/value", gpioNr);

    FILE *fp = fopen(path, "r");
    if (fp == NULL)
    {
        printf("[ERROR] kon file %s niet openen om input waarde te lezen.\n", path);
        fflush(stdout);
        return 0;
    }
    fread(&value, 1, 1, fp);
    fclose(fp);

    printf("[INFO] GPIO%d waarde is %c.\n", gpioNr, value);
    fflush(stdout);

    if (value == '0')      return 0;
    else if (value == '1') return 1;
    else
    {
        printf("[WARNING] GPIO%d value %c is not recognized, assuming logic level 1.\n", gpioNr, value);
        return 1;
    }
}

/**
 * @brief Schrijft een logische waarde naar een GPIO uitgang via sysfs.
 *
 * @param gpioNr GPIO nummer van de te schrijven pin.
 * @param value  0 = laag, 1 = hoog.
 */
void writeGPIO(int gpioNr, int value)
{
    char path[64];
    sprintf(path, "/sys/class/gpio/gpio%d/value", gpioNr);

    FILE *fp = fopen(path, "w");
    if (fp == NULL)
    {
        printf("[ERROR] kon file %s niet openen om waarde te schrijven.\n", path);
        fflush(stdout);
        return;
    }
    fprintf(fp, value ? "1" : "0");
    fclose(fp);
}

/* ------------------------------------------------------------------ */
/* Interne LED hulpfuncties                                            */
/* ------------------------------------------------------------------ */

/**
 * @brief Laat de film LED (P9_18) een aantal keren knipperen.
 *
 * Stuurt @p count korte pulsen uit op GPIO_LED_FILM, gevolgd door
 * een langere pauze zodat het patroon leesbaar blijft.
 * Enkel bedoeld voor gebruik binnen de LED thread.
 *
 * @param count Aantal knipperpulsen (overeenkomend met het filmnummer).
 */
static void blinkFilmLED(int count)
{
    for (int i = 0; i < count; i++)
    {
        writeGPIO(GPIO_LED_FILM, 1);
        usleep(BLINK_SHORT_ON);
        writeGPIO(GPIO_LED_FILM, 0);
        usleep(BLINK_SHORT_OFF);
    }
    usleep(BLINK_PAUSE);
}

/* ------------------------------------------------------------------ */
/* LED thread                                                          */
/* ------------------------------------------------------------------ */

/**
 * @brief Hoofdfunctie van de LED achtergrondthread.
 *
 * Leest continu de gedeelde toestandsvariabelen via @ref ledMutex
 * en stuurt de LEDs aan op basis van de huidige status:
 * - LED_STATUS_IDLE:    P9_17 langzaam knipperen, P9_18 uit.
 * - LED_STATUS_PLAYING: P9_17 constant aan, P9_18 knippert filmnummer.
 * - LED_STATUS_ERROR:   P9_17 snel knipperen, P9_18 uit.
 *
 * De mutex wordt kort vergrendeld om de waarden te kopiëren,
 * daarna onmiddellijk vrijgegeven zodat de hoofdthread niet
 * geblokkeerd wordt tijdens de knipperpatronen.
 *
 * @param arg Niet gebruikt (vereist door pthread interface).
 * @return NULL
 */
static void *ledThreadFunc(void *arg)
{
    (void)arg;

    while (ledThreadRunning)
    {
        pthread_mutex_lock(&ledMutex);
        LedStatus status = currentStatus;
        int       filmNr = currentFilmNumber;
        pthread_mutex_unlock(&ledMutex);

        switch (status)
        {
            case LED_STATUS_IDLE:
                writeGPIO(GPIO_LED_STATUS, 1);
                usleep(BLINK_SLOW_ON);
                writeGPIO(GPIO_LED_STATUS, 0);
                usleep(BLINK_SLOW_OFF);
                break;

            case LED_STATUS_PLAYING:
                writeGPIO(GPIO_LED_STATUS, 1);
                blinkFilmLED(filmNr);
                break;

            case LED_STATUS_ERROR:
                writeGPIO(GPIO_LED_STATUS, 1);
                usleep(BLINK_FAST_ON);
                writeGPIO(GPIO_LED_STATUS, 0);
                usleep(BLINK_FAST_OFF);
                break;
        }
    }

    writeGPIO(GPIO_LED_STATUS, 0);
    writeGPIO(GPIO_LED_FILM, 0);

    return NULL;
}

/* ------------------------------------------------------------------ */
/* Publieke interface                                                  */
/* ------------------------------------------------------------------ */

/**
 * @brief Start de LED achtergrondthread.
 *
 * Initialiseert GPIO_LED_STATUS en GPIO_LED_FILM als uitgang,
 * zet beide LEDs uit en start @ref ledThreadFunc als achtergrondthread.
 * Moet aangeroepen worden voor setStatusLED() of setFilmNumber().
 */
void startLEDThread(void)
{
    setGPIODirection(GPIO_LED_STATUS, 0);
    setGPIODirection(GPIO_LED_FILM, 0);
    writeGPIO(GPIO_LED_STATUS, 0);
    writeGPIO(GPIO_LED_FILM, 0);

    ledThreadRunning = 1;
    pthread_create(&ledThread, NULL, ledThreadFunc, NULL);

    printf("[INFO] LED thread gestart.\n");
    fflush(stdout);
}

/**
 * @brief Stopt de LED achtergrondthread netjes.
 *
 * Zet @ref ledThreadRunning op 0 en wacht via pthread_join() tot
 * de thread volledig gestopt is. De thread zet beide LEDs uit
 * voor hij eindigt.
 */
void stopLEDThread(void)
{
    ledThreadRunning = 0;
    pthread_join(ledThread, NULL);

    printf("[INFO] LED thread gestopt.\n");
    fflush(stdout);
}

/**
 * @brief Stelt de toestand in van de status LED (P9_17).
 *
 * Draadveilig — vergrendelt @ref ledMutex voor het schrijven.
 *
 * @param status De gewenste LED toestand (@ref LedStatus).
 */
void setStatusLED(LedStatus status)
{
    pthread_mutex_lock(&ledMutex);
    currentStatus = status;
    pthread_mutex_unlock(&ledMutex);
}

/**
 * @brief Stelt het filmnummer in voor de film indicator LED (P9_18).
 *
 * Draadveilig — vergrendelt @ref ledMutex voor het schrijven.
 * Logt een waarschuwing en doet niets bij een ongeldig nummer.
 *
 * @param number Filmnummer van 1 tot en met 16.
 */
void setFilmNumber(int number)
{
    if (number < 1 || number > 16)
    {
        printf("[WARNING] setFilmNumber: ongeldig filmnummer %d (verwacht 1..16).\n", number);
        fflush(stdout);
        return;
    }

    pthread_mutex_lock(&ledMutex);
    currentFilmNumber = number;
    pthread_mutex_unlock(&ledMutex);
}

/**
 * @brief Leest de 4 GPIO ingangen en geeft het filmnummer terug.
 *
 * Combineert GPIO 31, 50, 48 en 51 als bits 0..3 van een 4-bit woord.
 *
 * @return Filmnummer van 1 tot 16.
 */
int readFilmNumber(void)
{
    int bit0 = readGPIO(GPIO_FILM_BIT0);
    int bit1 = readGPIO(GPIO_FILM_BIT1);
    int bit2 = readGPIO(GPIO_FILM_BIT2);
    int bit3 = readGPIO(GPIO_FILM_BIT3);

    int filmNumber = (bit3 << 3) | (bit2 << 2) | (bit1 << 1) | bit0;

    /* +1 zodat het bereik 1..16 is in plaats van 0..15 */
    return filmNumber + 1;
}
