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
            - Simple handshake between badges for detecting badge group (4 types) already modeled
            - Games / challenges
            - Audio generation during games (nice to have)
            - Encryption for external EEPROM (with fake empty data trolling thing)
            - Cheat all challenges detection (simple flash content check, sends checksum over serial after special serial command is sent)
            - Backdoor codes to unlock unsolvable challenges (errors in code and/or broken things at the event)
            - LED effects and dimming
            - More
    */

#include <main_def.h>           //Global variables, constants etc.
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <resources.h>          //Functions for initializing and usage of hardware resources
#include <I2C.h>                //Fixed a semi-crappy lib found on internet, replace with interrupt driven one? If so, check hardware errata pdf!
#include <text_adv.h>           //Text adventure stuff
#include <stdio.h>




//This is where it begins, inits first and main program in while(1) loop.
int main(void)
{
    Setup();
    EERead(0, &gameState[0], 16);   //Load game status bits from EEPROM

    uint8_t idSet = 0;
    for (uint8_t x=0; x<4; ++x){
        idSet += ReadStatusBit(110+x);
    }

    //idSet can be used to detect cheating. After cheating idSet will be 0, with a virgin badge idSet will be 3.
    if (idSet != 1) {
        Reset();
    }

    SerSpeed(1);
    unsigned char strTest[]="\aHäckerHotel2020 badge pest!\b\b\b\b\bt\n";
    SerSend(&strTest[0]);

    //uint8_t bla = 0;
    SerSpeed(255);

    //Turn on LEDs on low setting to check for interrupt glitches
    for (uint8_t n=0; n<40; n++){
        if ((n%8)>5) n+=2;
        if (n<40) iLED[n] = 2;
    }

    //Test audio play, play a rainstorm with howling wind.
    uint8_t wind[7]={0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0};

    auRepAddr = &wind[0];
    auVolume = 127;

    while (1)
    {
        for (uint8_t x=1; x<6; ++x){
            if (x%3) wind[x] = floatAround(0x80, 6, 0x01, 0x00);
        }
        
        if (buttonMark){
            /*
                Check if button value has changed here
            */
            buttonMark = 0;

            //Floating speed
            floatSpeed(6, 0x0300, 0x0700);
            
            //Floating volume and wind howl during 8 bit rainstorm needs some more randomness
            auVolume = floatAround(auVolume, 2, 0x10, 0xA0);
            wind[0] = floatAround(wind[0], 2, 0x70, 0x90);
            wind[3] = 0xFF-wind[0];  //Inverse value of wind[0] produces a whistle   
                    
            TextAdventure();
          
            //Other games & user interaction checks
            //MagnetMaze();
            //BastetDictates();
            //MasterMind-ishThing(); //Not sure if to be implemented
            //LanyardCode();
            //MakeFriends();
             
            //Check light sensor status (added hysteresis to preserve writing cycles to internal EEPROM)
            if (adcPhot < 10) WriteStatusBit(116, 1);
            if (adcPhot > 100) WriteStatusBit(116, 0);

            //Check temperature 
        }

        /* 
            Audio and light effect control is done here:
            
            Audio:
                -IMPORTANT: Only play samples (when not communicating with other badges AND) when a headphone is detected
                -Samples are stored in flash, uncompressed raw unsigned 8-bit audio, no value 0 allowed except for last value, this MUST be 0!
                -To play a sample, point auRepAddr to zero, point auSmpAddr to the first byte of the sample.
                -To repeat a sample, point auRepAddr to the byte of the sample that you want to repeat from, the end of the repeating section is always the end of the sample. 
                -To stop repeating and let the sample finish playing to the end, point auRepAddr to zero.
                -To stop immediately, also point auSmpAddr to zero immediately after auRepAddr.
                -auVolume sets the volume, 0 is silent, 255 is loudest.
                -The speed is controlled by TCB1_CCMP, at 907 (0x038B) it plays at approximately (10MHz/907=)11025sps, setting this value too low will make the badge stop functioning.

            LEDs:
                -Usage: iLED[n] = value; NOTE: n must be < 40 and (n%8)>5 is not used.
                -The HCKR[2][6] EYE[2][2] WING[2][5] GEM[2] RAT CAT values can be used to substitute n for easy LED addressing, for 2 dimensional arrays, the first dimension is the LED color.
        */
    }
}