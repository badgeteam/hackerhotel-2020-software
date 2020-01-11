/*
 * ISR_IO.c
 *
 * Created: 27/11/2019 18:15:46
 * Author : Badge.team
 */ 

     /*
        TODO: 
            - Check if external Flash is empty. true? -> make I2C bus tristate until next power on (for external programming during sweatshop)
            - Enable audio output if headphone is detected, else enable badge handshake mode.
            - Use internal serial number for some things
            - Simple handshake between badges for detecting badge group (8 types?)
            - Games / challenges
            - Audio generation during games (nice to have)
            - Encryption for external EEPROM (with fake empty data trolling thing)
            - Cheat all challenges detection (simple flash content check, sends checksum over serial after special serial command is sent)
            - Backdoor codes to unlock unsolveable challenges (errors in code and/or broken things at the event)
            - LED effects and dimming
            - More
    */

#include <main_def.h>           //Global variables, constants etc.
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <resources.h>          //Functions for initializing and usage of hardware resources
#include <I2C.h>                //Fixed a semi-crappy lib found on internet, replace with interrupt driven one? If so, check hardware errata pdf!
#include <stdio.h>




//This is where it begins, inits first and main program in while(1) loop.
int main(void)
{
    setup();

    unsigned char strTest[]="\aHäckerHotel2020 badge pest!\b\b\b\b\bt\n";
    SerSend(&strTest[0]);

    //I2C 
    uint8_t devAddr = 0x50;
    uint8_t memAddr[2] = {0,0};
    volatile uint8_t retVal = 0;
    uint8_t btns = 0;
    uint8_t bla = 0;

    if (I2C_write_bytes(devAddr, &memAddr[0], 2, &strTest[0], sizeof(strTest))) retVal = 0xFA;
    SerSpeed(255);

    //Do some LED tests
    for (uint8_t n=0; n<40; n++){
        if ((n%8)>5) n+=2;
        if (n<40) iLED[n] = 255;
        for(uint8_t b=0; b<10; b++)_delay_ms(10);
    }
    for (uint8_t n=0; n<40; n++){
        if ((n%8)>5) n+=2;
        if (n<40) iLED[n] = 0;
        for(uint8_t b=0; b<10; b++)_delay_ms(10);
    }
    for (uint8_t n=0; n<5; n++){
        iLED[WING[R][n]] = 1<<n;
        iLED[WING[L][4-n]] = 1<<n;
        for(uint8_t b=0; b<10; b++)_delay_ms(10);
    }

    iLED[GEM[R]] = 255;
    iLED[GEM[L]] = 63;

    if (EEWrite(16, &strTest[0], sizeof(strTest))) retVal = 0xFA;     //Internal EEPROM write test

    while (1)
    {
        //serRxDone is 1 when a LF character is detected
        if (serRxDone) {
            while (serTxDone == 0);
            if (SerSend((uint8_t*) &serRx[0])) retVal = 0xFA; //Echo back using the serRx buffer (which is abused here and further on in code too for sending stuff, I'm lazy...)
            RXCNT = 0;
            serRxDone = 0;         
        }

        //buttonMark is updated by hardware timer, every increment there is a new button value available  
        if (buttonMark){
            serRx[0] = CheckButtons(btns); //Reading buttons and duration is done by passing old value to the function, return value is the new readout.
            /*
                Check if button value has changed here
            */
            btns = serRx[0]; 
            buttonMark = 0;

            bla++;
            if (bla%2) SelectAuIn(); else SelectTSens();    //Switch between audio in and temperature for testing
            
            //Send test sensor values
            serRx[1] = (unsigned char)(adcHall>>4);
            serRx[2] = (unsigned char)(adcPhot>>4);
            serRx[3] = (unsigned char)(adcTemp);
            serRx[4] = (unsigned char)(auIn[0]);            
            serRx[5] = (unsigned char) minuteMark;
            EERead(17, (uint8_t*)&serRx[6], 1);                                                   //"H" character (second char of strTest)
            if (I2C_read_bytes(devAddr, &memAddr[0], 2, (uint8_t*)&serRx[7], 1)) retVal = 0xFA;   //Bell character (first char of strTest)
            serRx[8] = retVal;
            while (serTxDone == 0);
            SerSend((uint8_t*) &serRx[0]);
            retVal = 0;
        }
    }
}