/**
 * @file io.h
 * @brief GPIO aansturing en LED status thread voor PRUPixels.
 *
 * Biedt functies voor het lezen en schrijven van GPIO pinnen,
 * en een achtergrondthread die twee status LEDs aanstuurt:
 * - P9_17 (GPIO_LED_STATUS): geeft de algemene spelerstatus weer.
 * - P9_18 (GPIO_LED_FILM):   knippert het actieve filmnummer.
 */

#ifndef IO_H
#define IO_H

/** @brief GPIO nummer van de status LED (P9_17). */
#define GPIO_LED_STATUS     5

/** @brief GPIO nummer van de film indicator LED (P9_18). */
#define GPIO_LED_FILM       4

/** @brief GPIO nummers voor de 4 filmkeuze ingangen. */
#define GPIO_FILM_BIT0  31  /**< P8_13 - bit 0 (LSB) */
#define GPIO_FILM_BIT1  50  /**< P9_14 - bit 1       */
#define GPIO_FILM_BIT2  48  /**< P9_15 - bit 2       */
#define GPIO_FILM_BIT3  51  /**< P9_16 - bit 3 (MSB) */

/**
 * @brief Mogelijke toestanden voor de status LED (P9_17).
 *
 * - LED_STATUS_IDLE:    PRU niet actief, LED knippert langzaam (1x per seconde).
 * - LED_STATUS_PLAYING: Video speelt, LED is constant aan.
 * - LED_STATUS_ERROR:   Fout opgetreden, LED knippert snel (5x per seconde).
 */
typedef enum {
    LED_STATUS_IDLE    = 0, /**< PRU niet actief, langzaam knipperen. */
    LED_STATUS_PLAYING = 1, /**< Video speelt, constant aan.          */
    LED_STATUS_ERROR   = 2  /**< Fout opgetreden, snel knipperen.     */
} LedStatus;

/**
 * @brief Foutcodes die het aantal knipperingen van de status LED bepalen.
 *
 * De waarde van de enum bepaalt rechtstreeks het aantal knipperingen:
 * - LED_ERR_VIDEO:  2 knipperingen — fout bij openen videobestand.
 * - LED_ERR_FRAME:  3 knipperingen — fout bij alloceren framegeheugen.
 * - LED_ERR_SCALER: 4 knipperingen — fout bij initialiseren scaler.
 * - LED_ERR_PRU:    5 knipperingen — fout bij mappen PRU shared memory.
 */
typedef enum {
    LED_ERR_NONE     = 0, /**< Geen fout.                         */
    LED_ERR_VIDEO    = 2, /**< Fout bij openen video: 2x.         */
    LED_ERR_FRAME    = 3, /**< Fout bij alloceren frame: 3x.      */
    LED_ERR_SCALER   = 4, /**< Fout bij initialiseren scaler: 4x. */
    LED_ERR_PRU      = 5, /**< Fout bij PRU shared memory: 5x.    */
} LedErrorCode;

/**
 * @brief Stelt de richting in van een GPIO pin.
 *
 * @param gpioNr    GPIO nummer (bv. 48 voor P9_14).
 * @param direction 0 = uitgang, anders = ingang.
 */
void setGPIODirection(int gpioNr, int direction);

/**
 * @brief Leest de logische waarde van een GPIO ingang.
 *
 * @param gpioNr GPIO nummer van de in te lezen pin.
 * @return 0 of 1 bij succes, 0 bij fout.
 */
char readGPIO(int gpioNr);

/**
 * @brief Schrijft een logische waarde naar een GPIO uitgang.
 *
 * @param gpioNr GPIO nummer van de te schrijven pin.
 * @param value  0 = laag, 1 = hoog.
 */
void writeGPIO(int gpioNr, int value);

/**
 * @brief Start de LED achtergrondthread.
 *
 * Initialiseert GPIO_LED_STATUS en GPIO_LED_FILM als uitgang
 * en start een achtergrondthread die de LEDs aanstuurt op basis
 * van de huidige status en het filmnummer.
 * Moet aangeroepen worden voor setStatusLED() of setFilmNumber().
 */
void startLEDThread(void);

/**
 * @brief Stopt de LED achtergrondthread netjes.
 *
 * Wacht tot de thread volledig gestopt is voor terug te keren.
 * Zet beide LEDs uit na het stoppen.
 */
void stopLEDThread(void);

/**
 * @brief Stelt de toestand in van de status LED (P9_17).
 *
 * Thraed safe - mag vanuit elke thread aangeroepen worden.
 *
 * @param status De gewenste LED toestand (@ref LedStatus).
 */
void setStatusLED(LedStatus status);

/**
 * @brief Stelt de foutcode in die het knipperpatroon bij LED_STATUS_ERROR bepaalt.
 *
 * De foutcode bepaalt het aantal knipperingen van de status LED.
 * Thraed safe - mag vanuit elke thread aangeroepen worden.
 * Roep deze functie aan voor setStatusLED(LED_STATUS_ERROR).
 *
 * @param error De foutcode (@ref LedErrorCode).
 */
void setErrorCode(LedErrorCode error);

/**
 * @brief Stelt het filmnummer in voor de film indicator LED (P9_18).
 *
 * De LED knippert het opgegeven aantal keren, gevolgd door een pauze,
 * waarna het patroon herhaalt. Draadveilig — mag vanuit elke thread
 * aangeroepen worden.
 *
 * @param number Filmnummer van 1 tot en met 16.
 */
void setFilmNumber(int number);

/**
 * @brief Leest de 4 GPIO ingangen en geeft het filmnummer terug.
 *
 * Combineert GPIO 31, 50, 48 en 51 als bits 0..3 van een 4-bit woord.
 *
 * @return Filmnummer van 1 tot 16.
 */
int readFilmNumber(void);

#endif /* IO_H */
