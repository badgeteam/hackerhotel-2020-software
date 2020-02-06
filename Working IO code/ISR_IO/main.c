/*
 * ISR_IO.c
 *
 * Created: 27/11/2019 18:15:46
 * Author : Badge.team
 */ 

     /*
        TODO: 
            - Enable audio output if headphone is detected, else enable badge handshake mode.
            - Simple handshake between badges for detecting badge group (4 types) already modeled
            - Audio generation during games (nice to have)
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
#include <simon.h>              //Bastet Dictates (Simon clone)
#include <maze.h>               //MagnetMaze game
#include <lanyard.h>            //Lanyard Puzzle
#include <friends.h>
#include <stdio.h>




//This is where it begins, inits first and main program in while(1) loop.
int main(void)
{
    Setup();

    SerSpeed(0xff);
    ////Turn on LEDs on low setting to check for interrupt glitches

    //"Hacker"
    for (uint8_t n=0; n<6; n++){
        iLED[HCKR[R][n]] = 1;
        iLED[HCKR[G][n]] = 1;
    }

    //"Wings"
    for (uint8_t n=0; n<5; n++){
        iLED[WING[L][n]] = 1;
        iLED[WING[R][n]] = 1;
    }    

    //"Others"
    iLED[EYE[G][R]] = 1;
    iLED[EYE[G][L]] = 1;
    iLED[EYE[R][R]] = 1;
    iLED[EYE[R][L]] = 1;
    iLED[SCARAB[R]] = 1;
    iLED[SCARAB[G]] = 1;
    iLED[BADGER] = 1;
    iLED[CAT] = 1;

    LoadGameState();

    while (TRUE) {
        if (GenerateAudio()) {
            buttonState = CheckButtons(buttonState);
            buttonMark = 0;
            
            GenerateBlinks();           
            
            //Main game, to complete: Finish sub-game MagnetMaze and MakeFriends too.
            TextAdventure();
          
            //Other games & user interaction checks
            MagnetMaze();
            BastetDictates();
            LanyardCode();
            MakeFriends();

            //Save progress
            SaveGameState(); 
            
            //Check light sensor status (added hysteresis to preserve writing cycles to internal EEPROM)
            if (adcPhot < 10) WriteStatusBit(116, 1);
            if (adcPhot > 100) WriteStatusBit(116, 0);

            //Check temperature 
            HotSummer();

      }

        /* 
            Audio and light effect control explained:
            
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
                -The HCKR[2][6] EYE[2][2] WING[2][5] SCARAB[2] BADGER CAT values can be used to substitute n for easy LED addressing, for 2 dimensional arrays, the first dimension is the LED color.
        */
    }
}
