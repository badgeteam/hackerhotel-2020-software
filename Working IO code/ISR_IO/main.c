/*
 * ISR_IO.c
 *
 * Created: 27/11/2019 18:15:46
 * Author : Badge.team
 *
 *   ___ ___                __                   ___ ___         __         .__
 *  /   |   \_____    ____ |  | __ ___________  /   |   \  _____/  |_  ____ |  |
 * /    ~    \__  \ _/ ___\|  |/ // __ \_  __ \/    ~    \/  _ \   __\/ __ \|  |
 * \    Y    // __ \\  \___|    <\  ___/|  | \/\    Y    (  <_> )  | \  ___/|  |__
 *  \___|_  /(____  /\___  >__|_ \\___  >__|    \___|_  / \____/|__|  \___  >____/
 *        \/      \/     \/     \/    \/              \/                  \/
 * _______________   _______________    __________             .___           ___________
 * \_____  \   _  \  \_____  \   _  \   \______   \_____     __| _/ ____   ___\__    ___/___ _____    _____
 *  /  ____/  /_\  \  /  ____/  /_\  \   |    |  _/\__  \   / __ | / ___\_/ __ \|    |_/ __ \\__  \  /     \
 * /       \  \_/   \/       \  \_/   \  |    |   \ / __ \_/ /_/ |/ /_/  >  ___/|    |\  ___/ / __ \|  Y Y  \
 * \_______ \_____  /\_______ \_____  /  |______  /(____  /\____ |\___  / \___  >____| \___  >____  /__|_|  /
 *         \/     \/         \/     \/          \/      \/      \/_____/      \/           \/     \/      \/
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
    LoadGameState();
    gameState[0]|=1;

    //Check if fine
    if (SelfTest()) {
        WipeAfterBoot(1);
        while (TRUE) ;

    //Startup
    } else {

        while (TRUE) {
            if (GenerateAudio()) {

                //Some sound effects and button readout 
                lastButtonState = buttonState;
                buttonState = CheckButtons();
                if (buttonState != 0xff) {
                    iLED[CAT] = dimValue;
                    if ((effect & 0xffe0)==0)
                        effect = 0x13f + (buttonState << 5);
                } else {
                    iLED[CAT] = 0;
                }
                --buttonMark;

                //Switch between audio port input (badge to badge comms) and onboard temperature sensor
                if (VREF_CTRLA == 0x12) SelectAuIn(); else SelectTSens();

                //Victory dance when the game is completed
                if ((gameNow == TEXT) &&
                     (gameState[10] == 0) &&    // No "all ones" EEPROM failure (due to glitching etc.) detected
                     CheckState(122) &&         // Finished BastetDictates
                     CheckState(123) &&         // Finished Lanyard
                     CheckState(124) &&         // Connected to 3 others
                     CheckState(127)) {         // Finished the text adventure (implies state 125 and 126)
                
                    //Completed!!!
                    VictoryDance();
                }

                //Light effects
                GenerateBlinks();

                //Main game, to complete: Finish sub-game MagnetMaze and MakeFriends too.
                TextAdventure();

                //Other games & user interaction checks
                MagnetMaze();
                LanyardCode();
                BastetDictates();
                MakeFriends();

                //Save progress
                SaveGameState();

                //Check light sensor status (added hysteresis to preserve writing cycles to internal EEPROM)
                if (adcPhot < 10) UpdateState(116);
                if (adcPhot > 100) UpdateState(128+116);

                lightsensorSum -= (lightsensorSum>>6);
                lightsensorSum += adcPhot>>4;
                if (lightsensorSum>>6 < 128) {
                    dimValue = lightsensorSum>>5;
                    if (dimValue < 8)
                        dimValue = 8;
                } else {
                    dimValue = 255;
                }

                //Check temperature
                HotSummer();

                /*
                Audio and light effect control explained:

                Audio:
                    -IMPORTANT: Only play samples (when not communicating with other badges AND) when a headphone is detected
                    -Samples can be stored in flash, uncompressed raw unsigned 8-bit audio, no value 0 allowed except for last value, this MUST be 0!
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
    }
}
