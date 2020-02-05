#include <main_def.h>

// Global vars
volatile uint8_t iLED[40];           // 0,1,2,3,4,5(,6,7),8,9,10,11,12,13(,14,15) etc. ()=unused

volatile uint8_t timeout_I2C;        // Used for I2C timeout error, old code was blocking further code execution.

volatile unsigned char serRx[RXLEN]; // Receive buffer
volatile unsigned char *serTxAddr;   // Tx data address pointer
volatile uint8_t serTxDone = 1;      // Done transmitting
volatile uint8_t serRxDone = 0;      // Done receiving (CR or LF detected)

volatile uint8_t minuteMark = 0;     // RTC overflows every minute, this counts up to max 255.
volatile uint8_t buttonMark = 0;     // Increments when a guaranteed new button value is available.

volatile uint16_t adcPhot;           // Photo transistor ADC value (0...4096)
volatile uint16_t adcHall;           // Hall sensor value
volatile uint16_t calHall;           // Hall sensor calibration (center)
volatile uint16_t adcBtns;           // Raw button value
volatile uint8_t  adcTemp;           // Raw temperature related value
volatile uint8_t  calTemp;           // Raw temperature calibration (sort of)
volatile uint8_t  detHdPh;           // Headphone detected (TODO)


uint8_t buttonState = 0xff;
uint8_t gameState[BOOTCHK];          // Game state plus inventory
uint16_t inventory[2] = {0,0};
uint8_t whoami = 0;
uint8_t gameNow = TEXT;

volatile uint16_t effect = 0;

volatile uint8_t *auSmpAddr = &zero;    // Audio sample address pointer
volatile uint8_t *auRepAddr = &zero;    // Audio loop start address pointer
volatile uint8_t auVolume;              // Used for Volume control
volatile uint8_t auPlayDone;            // User writes 0 just after initiation of playing a sample, after playing sample, interrupt routine writes it to 1.
volatile uint8_t auStart = 0;           // To initiate playing a sound
volatile uint8_t zero = 0;              // Point to this to stop playing audio

volatile uint8_t auIn[AULEN];           // Audio input buffer
volatile uint8_t adc0Chg = 0;           // Changed adc0 channel and reference
//Quarter sine table, can be used for audio or other things (invert for nice LED dimming curve from bright to off)
const uint8_t QSINE[32] = {0x01,0x0d,0x19,0x26,0x32,0x3e,0x4a,0x56,0x62,0x6f,0x78,0x83,0x8e,0x98,0xa2,0xab,0xb4,0xbf,0xc5,0xcf,0xd4,0xdb,0xe1,0xe7,0xec,0xf0,0xf4,0xf7,0xfa,0xfc,0xfe,0xff};

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
const uint8_t GEM[2] = {12, 10};//Gem on top of cat head (red, green)
const uint8_t RAT = 11;         //Rat gem LED location inside iLED
const uint8_t CAT = 13;         //Cat lower gem LED location inside iLED

