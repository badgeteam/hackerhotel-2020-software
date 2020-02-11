/*
 * main_def.h
 *
 * Created: 17/12/2019
 * Author: Badge.team
 */

#ifndef MAIN_DEF_H_
#define MAIN_DEF_H_

    #define F_CPU 10000000UL
    
    #include <avr/io.h>
    #include <avr/interrupt.h>
    #include <stdlib.h>

    //Used in ISRs for efficiency
    #define L_COL   GPIOR0  //LED column counter
    #define RXCNT   GPIOR1  //Serial Rx position counter
    #define FREE    GPIOR2  //
    #define A1CNT   GPIOR3  //Counter for ADC1
    
    #define RXLEN   65      //Length of serial Rx buffer (yes, it one longer than a power of two)
    #define TXLEN   33      //Length of serial Tx buffer (yes, it one longer than a power of two)
    #define HPLVL   25      //Headphone detection threshold (load is ~200 Ohm or less)

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
    #define CHEATS      BOOTCHK+4

    #define MAX_CHEATS  8           //Number of cheat codes that are accepted (Maximum = MAX_REACT/4)
    #define MAX_REACT   MAX_CHEATS*4
    #define L_BOILER    32

    //External EEPROM address
    #define EE_I2C_ADDR     0x50
    #define EXT_EE_MAX      32767

    //Keys are md5 hash of 'H@ck3r H0t3l 2020', split in two
    #define KEY_LENGTH      8
    extern const uint8_t xor_key[2][KEY_LENGTH];

    //Which game is running
    enum {TEXT, MAZE, BASTET, LANYARD, FRIENDS};

    #define FALSE       0
    #define TRUE        1
    #define GAME_OVER   2

    //Global vars
    extern volatile uint8_t iLED[40];           // 0,1,2,3,4,5(,6,7),8,9,10,11,12,13(,14,15) etc. ()=unused

    extern volatile uint8_t readDataI2C;        // If true, reads data, if false, writes address
    extern volatile uint8_t bytesLeftI2C;       // Number of bytes left for a restart command (after address write) or a NACK+STOP command
    extern volatile uint8_t *addrDataI2C;       // Address pointer to the I2C data

    extern volatile unsigned char serRx[RXLEN]; // Receive buffer
    extern volatile unsigned char *serTxAddr;   // Tx data address pointer
    extern volatile uint8_t serTxDone;          // Done transmitting
    extern volatile uint8_t serRxDone;          // Done receiving (LF detected)

    extern volatile uint8_t fastTicker;
    extern volatile uint8_t oldTicker;
    extern volatile uint8_t minuteMark;
    extern volatile uint8_t buttonMark;

    extern volatile uint16_t adcPhot;
    extern volatile uint16_t adcHall;
    extern volatile uint16_t calHall;
    extern volatile uint16_t adcBtns;
    extern volatile uint16_t adcTemp;
    extern volatile uint16_t calTemp;
    extern volatile uint8_t  detHdPh;

    // Global challenge data
    extern uint8_t buttonState;
    extern uint8_t lastButtonState;

    extern uint8_t gameState[BOOTCHK];
    extern uint16_t inventory[2];               //     
    extern uint8_t whoami;
    extern uint8_t gameNow;

    //Things for audio and LEDs
    extern volatile uint16_t effect;

    extern volatile uint8_t *auSmpAddr;         // Audio sample address pointer
    extern volatile uint8_t *auRepAddr;         // Audio loop start address pointer
    extern volatile uint8_t auVolume;           // Used for Volume control
    extern volatile uint8_t zero;               // Point to this to stop playing audio

    extern volatile uint8_t auIn;
    extern volatile uint8_t adc0Chg;

    volatile uint8_t  dimValue;
    volatile uint16_t dimValueSum;
    extern const    uint8_t QSINE[32];

    //LED translation matrices (usage iLED[name_of_const_array[][]] = value;)
    extern const uint8_t HCKR[2][6];
    extern const uint8_t EYE[2][2];
    extern const uint8_t WING[2][5];
    extern const uint8_t SCARAB[2];        //Gem on top of cat head (red, green)
    extern const uint8_t BADGER;           //Rat gem LED location inside iLED
    extern const uint8_t CAT;           //Cat lower gem LED location inside iLED

#endif  //MAIN_DEF_H_
