/*
 * friends.c
 *
 * Created: 21/01/2020 15:42:42
 * Author: Badge.team
 */ 

#include <friends.h>
#include <main_def.h>
#include <resources.h>
#include <I2C.h>

//Returns measured voltage * 4 (2V returns 8) if it is in the expected range (DELTA)
uint8_t chkVolt250(){
    static uint8_t avgVolt = 0;
        
    for (uint8_t x=25; x<250; x+=25) {
        if ((auIn[0] > (x-DELTA)) && (auIn[0] < (x+DELTA))) {
            avgVolt = x;
            break;
        }
    }
    return avgVolt/25;
}

// Main game loop
uint8_t MakeFriends(){
    static uint8_t progress;
    static uint8_t setDAC[2] = {255, 0};
    static uint8_t chkTmr = 0;
    static uint8_t jackIn = 0;
    static uint8_t candidate = 0;

    //Checking for headphones
    if (detHdPh) return 0;

    //Audio off, set voltage level (setDAC[0]*10mV)
    if (progress == NO_OTHER) {
        setDAC[0] = whoami * 50;
        auRepAddr = &setDAC[0];
        auVolume = 255;
    }

    //Check for other badges
    if ((auIn[0] < (setDAC[0] - 10)) || (auIn[0] > (setDAC[0] + 10))) {
        if (progress == NO_OTHER) {
            ++chkTmr;
            if (chkTmr >= 8) {
                progress = FIRST_CONTACT;
                chkTmr = 0;
            }
        }

        //Candidate found, check ranges
        else if (progress == FIRST_CONTACT) {
            jackIn = chkVolt250();
            if (jackIn) {
                ++chkTmr;
                if (chkTmr >= 8){
                    if ((whoami == 1)&&((jackIn == 3)||(jackIn == 4)||(jackIn == 5))) {
                        progress = SECOND_LOVE; 
                    } else if ((whoami == 2)&&((jackIn == 3)||(jackIn == 5)||(jackIn == 6))) {
                        progress = SECOND_LOVE;
                    } else  if ((whoami == 3)&&((jackIn == 4)||(jackIn == 5)||(jackIn == 7))) {
                        progress = SECOND_LOVE;
                    } else if ((whoami == 4)&&((jackIn == 5)||(jackIn == 6)||(jackIn == 7))) {
                        progress = SECOND_LOVE;
                    } else progress = NO_OTHER;
                    chkTmr = 0;
                }
            } else {
                chkTmr = 0;
                progress = NO_OTHER;
            }
        }

        //Wait for the other badge (and a bit longer)
        else if (progress == SECOND_LOVE){
            ++chkTmr;
            for (uint8_t x=0; x<5; ++x){
                iLED[WING[L][x]] = (lfsr() > 127)?dimValue:0;
                iLED[WING[R][x]] = (lfsr() > 127)?dimValue:0;
            }

            //The badge with the lowest voltage will now go to 2.5V (impedance check, for people tricking the things with a power supply)
            if (chkTmr >= 200){
                candidate = jackIn-whoami;
                if (jackIn > (whoami<<1)) setDAC[0] = 250;
                progress = THIRD_KISS;
                chkTmr = 0;
            }
        } 
        
        //Wait a bit, check Voltage again 
        else if (progress == THIRD_KISS) {
            chkTmr++;
            if (chkTmr >= 8) {
                chkTmr = 0;
            }
        }

    } else progress = NO_OTHER;
      
    return 0;
}
