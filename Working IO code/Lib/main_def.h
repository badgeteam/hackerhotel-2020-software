#ifndef MAIN_DEF_H_
#define MAIN_DEF_H_

    #define F_CPU 10000000UL
    
    #include <avr/io.h>
    #include <avr/interrupt.h>
    #include <stdlib.h>

    //Used in ISRs for efficiency
    #define L_COL   GPIOR0  //LED column counter
    #define RXCNT   GPIOR1  //Serial Rx position counter
    #define AUPOS   GPIOR2  //Audio in position counter
    #define A1CNT   GPIOR3  //Counter for ADC1
    
    #define RXLEN   65      //Length of serial Rx buffer (yes, it one longer than a power of two)
    #define TXLEN   33      //Length of serial Tx buffer (yes, it one longer than a power of two)
    #define AULEN   32      //Audio input buffer length: MUST be a power of 2

    #define USART0_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)
    
    //For 2D LED arrays, first dimension is Red, Green, Right or Left
    #define R       0
    #define L       1
    #define G       1

    #define BTN_TMR 34      //RTC Compare incremental value for button readout timing (512 = 1 second)

    //Internal EEPROM
    #define STATLEN     16          //Length of game progress data
    #define INVADDR     STATLEN     //Address of inventory (2x uint16_t)
    #define BOOTCHK     STATLEN+4   //Address of boot check result in internal EEPROM
    #define MAX_REACT   32
    #define MAX_CHEATS  8           //Number of cheat codes that are accepted (Maximum = MAX_REACT/4)
    #define CHEATS      BOOTCHK+4 
    #define EE_I2C_ADDR 0x50 //External EEPROM address

    enum {TEXT, MAZE, BASTET, LANYARD};

    #define TRUE    1
    #define FALSE   0

    //Global vars
    extern volatile uint8_t iLED[40];           // 0,1,2,3,4,5(,6,7),8,9,10,11,12,13(,14,15) etc. ()=unused

    extern volatile uint8_t timeout_I2C;        // 

    extern volatile unsigned char serRx[RXLEN]; // Receive buffer
    extern volatile unsigned char *serTxAddr;   // Tx data address pointer
    extern volatile uint8_t serTxDone;          // Done transmitting
    extern volatile uint8_t serRxDone;          // Done receiving (LF detected)

    extern volatile uint8_t minuteMark;
    extern volatile uint8_t buttonMark;

    extern volatile uint16_t adcPhot;
    extern volatile uint16_t adcHall;
    extern volatile uint16_t calHall;
    extern volatile uint16_t adcBtns;
    extern volatile uint8_t  adcTemp;
    extern volatile uint8_t  calTemp;
    extern volatile uint8_t  detHdPh;

    // Global challenge data
    extern uint8_t buttonState;
    extern uint8_t gameState[BOOTCHK];
    extern uint16_t inventory[2];               //     
    extern uint8_t whoami;
    extern uint8_t gameNow;

    //Things for audio and LEDs
    extern volatile uint16_t effect;

    extern volatile uint8_t *auSmpAddr;         // Audio sample address pointer
    extern volatile uint8_t *auRepAddr;         // Audio loop start address pointer
    extern volatile uint8_t auVolume;           // Used for Volume control
    extern volatile uint8_t auPlayDone;         // User writes 0 just after initiation of playing a sample, after playing sample, interrupt routine writes it to 1.
    extern volatile uint8_t auStart;            // To initiate playing a sound
    extern volatile uint8_t zero;               // Point to this to stop playing audio

    extern volatile uint8_t auIn[AULEN];
    extern volatile uint8_t adc0Chg;
    extern const    uint8_t QSINE[32];

    //LED translation matrices (usage iLED[name_of_const_array[][]] = value;)
    extern const uint8_t HCKR[2][6];
    extern const uint8_t EYE[2][2];
    extern const uint8_t WING[2][5];
    extern const uint8_t GEM[2];        //Gem on top of cat head (red, green)
    extern const uint8_t BADGER;           //Rat gem LED location inside iLED
    extern const uint8_t CAT;           //Cat lower gem LED location inside iLED

#endif  //MAIN_DEF_H_