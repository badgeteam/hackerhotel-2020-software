/*
 * main_def.c
 *
 * Created: 17/12/2019
 * Author: Badge.team
 */

#include <main_def.h>

//Keys are md5 hash of 'H@ck3r H0t3l 2020', split in two
const uint8_t xor_key[2][KEY_LENGTH] = {{0x74, 0xbf, 0xfa, 0x54, 0x1c, 0x96, 0xb2, 0x26},{0x1e, 0xeb, 0xd6, 0x8b, 0xc0, 0xc2, 0x0a, 0x61}};
//const unsigned char boiler_plate[]   = "Hacker Hotel 2020 by badge.team "; // boiler_plate must by 32 bytes long, excluding string_end(0)

// Global vars
volatile uint8_t iLED[40];           // 0,1,2,3,4,5(,6,7),8,9,10,11,12,13(,14,15) etc. ()=unused

volatile uint8_t readDataI2C;        // If true, reads data, if false, writes address
volatile uint8_t bytesLeftI2C;       // Number of bytes left for a restart command (after address write) or a NACK+STOP command
volatile uint8_t *addrDataI2C;       // Address pointer to the I2C data

volatile unsigned char serRx[RXLEN]; // Receive buffer
volatile unsigned char *serTxAddr;   // Tx data address pointer
volatile uint8_t serTxDone = 1;      // Done transmitting
volatile uint8_t serRxDone = 0;      // Done receiving (CR or LF detected)

volatile uint8_t fastTicker = 0;
volatile uint8_t oldTicker = 0;
volatile uint8_t minuteMark = 0;     // RTC overflows every minute, this counts up to max 255.
volatile uint8_t buttonMark = 0;     // Increments when a guaranteed new button value is available.

volatile uint16_t adcPhot;           // Photo transistor ADC value (0...4096)
volatile uint16_t adcHall;           // Hall sensor value
volatile uint16_t calHall;           // Hall sensor calibration (center)
volatile uint16_t adcBtns;           // Raw button value
volatile uint16_t adcTemp;           // Raw temperature related value
volatile uint16_t calTemp;           // Raw temperature calibration (sort of)
volatile uint8_t  detHdPh;           // Headphone detected


uint8_t buttonState = 0xff;
uint8_t lastButtonState = 0xff;

uint8_t gameState[BOOTCHK];          // Game state plus inventory
uint16_t inventory[2] = {0,0};
uint8_t whoami = 0;
uint8_t gameNow = TEXT;

volatile uint16_t effect = 0;

volatile uint8_t *auSmpAddr = &zero;    // Audio sample address pointer
volatile uint8_t *auRepAddr = &zero;    // Audio loop start address pointer
volatile uint8_t auVolume;              // Used for Volume control
volatile uint8_t zero = 0;              // Point to this to stop playing audio

volatile uint8_t auIn;                  // Audio input
volatile uint8_t adc0Chg = 0;           // Changed adc0 channel and reference

volatile uint16_t lightsensorSum = 0x2000;  // Add moving average to dimmer to reduce candlelight effect
volatile uint8_t  dimValue       = 0xff;    // Global dimming maximum LED value.

//LED translation matrices (usage iLED[name_of_const_array[][]] = value;)
/*
    Values should all correspond with FINAL badge and most with proto badge too.
*/
const uint8_t HCKR[2][6] = {
    {20, 18, 17, 19, 16, 21},   //Red Hacker glyphs LED locations inside iLED
    {28, 26, 25, 27, 24, 29}    //Green Hacker glyphs LED locations inside iLED 
};
const uint8_t EYE[2][2] = {
    {8, 32},                    //Red eyes (R, L) LED locations inside iLED
    {9, 37}                     //Green eyes (R, L) LED locations inside iLED
};
const uint8_t WING[2][5] = {
    {2,  1,  5,  0,  3},        //Right wing LED locations inside iLED, bottom to top
    {4,  36, 34, 33, 35}        //Left wing LED locations inside iLED, bottom to top
};
const uint8_t SCARAB[2] = {12, 10};//Gem on top of cat head (red, green)
const uint8_t BADGER = 11;         //Rat gem LED location inside iLED
const uint8_t CAT = 13;         //Cat lower gem LED location inside iLED

