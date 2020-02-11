/*
 * friends.c
 *
 * Created: 21/01/2020 15:42:42
 * Author: Badge.team
 *           __  __
 *        ,-'  `'  \         _---``--
 *       /    _  _  ;      __        `.
 *      /    / `' \;        /`-----    )
 *     /  .-/    ,(         ),     \-. ;
 *     |  \(       \       /        )/;
 *     |   -      _5       `7       -;
 *    /    (  ___-'         `-____    |
 *   (   ___`-_                 \ ____|
 *    \ /   `,/ \     _(\__      /    \
 *     \      ;  \  .' /'  `i.  /      |
 *      |      \ _-'( _\__-/  `-       |
 *      |       `   ,`     `_          | BP
 */ 

#include <friends.h>
#include <main_def.h>
#include <resources.h>

//Returns measured voltage * 4 (2V returns 8) if it is in the expected range (DELTA)
uint8_t chkVolt250(){
    uint8_t avgVolt = 9;
        
    for (uint8_t x=225; x>24; x-=25) {
        if ((auIn > (x-DELTA)) && (auIn < (x+DELTA))) {
            break;
        }
        --avgVolt;
    }
    return avgVolt;
}

// Main game loop
uint8_t MakeFriends(){
    static uint8_t progress;
    static uint8_t setDAC[2] = {255, 0};
    static uint8_t chkTmr = 0;
    static uint8_t jackIn = 0;
    static uint8_t candidate = 0;

    ++chkTmr;

    //Check if badge has found 3 friends, if true, don't accept any new connections!
    uint8_t foundAll = 1;
    for (uint8_t x=0; x<4; ++x){
        if (CheckState(100+x) == 0) foundAll = 0;
    }
    if (foundAll) {
        UpdateState(124);
        return 0;
    }

    //Set game state, must be able to hijack from other games.
    if (progress > FIRST_CONTACT) { 
        gameNow = FRIENDS; 
    }
    
    //Clear game state if not communicating properly or headphones detected
    if (((progress == NO_OTHER) || detHdPh) && (gameNow == FRIENDS)) {
        gameNow = TEXT; 
        effect = 0;
    }

    //Audio off, set voltage level (setDAC[0]*10mV)
    if ((progress == NO_OTHER) && (detHdPh == 0)) {
        setDAC[0] = whoami * 51;
        auRepAddr = &setDAC[0];
        auVolume = 255;
    } else {
        if (progress & NEXT){
            progress++;
            progress&=0x0f;
            chkTmr = 0;
        }
    }

    //Check for other badges ()
    if ((auIn < (setDAC[0] - DELTA)) || (auIn > (setDAC[0] + DELTA)) || (progress > FIRST_CONTACT)) {
        if ((progress == NO_OTHER) && (detHdPh == 0)) {
            if (chkTmr >= 8) {
                progress |= NEXT;
            }
        }

        //Candidate found, check ranges
        else if (progress == FIRST_CONTACT) {
            jackIn = chkVolt250();
            if (jackIn) {
                if (chkTmr >= SHORT_WAIT){
                    if (   ((whoami == 1)&&((jackIn >= 3)&&(jackIn <= 5))) 
                    || ((whoami == 2)&&((jackIn == 3)||(jackIn == 5)||(jackIn == 6)))  
                    || ((whoami == 3)&&((jackIn == 4)||(jackIn == 5)||(jackIn == 7))) 
                    || ((whoami == 4)&&((jackIn >= 5)&&(jackIn <= 7)))   ) {
                        progress |= NEXT;
                    } else progress = NO_OTHER;
                }
            } else {
                progress = NO_OTHER;
            }
        }

        //Wait for the other badge (and a bit longer for effect)
        else if (progress == SECOND_LOVE){
            
            effect = 7;
            if (chkTmr >= SHORT_WAIT) {
                setDAC[0] = floatAround(0x80, 8, 100, 255); 
            }

            //The badge with the lowest voltage will now go to 2.5V (impedance check, for people tricking the things with a power supply)
            if (chkTmr >= LONG_WAIT){
                candidate = jackIn-whoami;
                if (candidate > whoami) {
                    setDAC[0] = 249;
                } else {
                    setDAC[0] = whoami * 51;
                }
                progress |= NEXT;
            }
        } 
        
        //Wait a bit, check Voltage again, if ok show who you connected with and your own number, save, exit
        else if (progress == THIRD_KISS) {
            if (chkTmr >= SHORT_WAIT) {
                jackIn = chkVolt250();
                if (jackIn == (((candidate > whoami)?candidate:whoami) + 5)) {
                    UpdateState(99+candidate);
                 
                    effect = 31;
                    WingBar(candidate, whoami);
                    
                    progress |= NEXT;
                } else progress = NO_OTHER;                
            }

        //Wait for quit
        } else if (progress == FOURTH_BASE) {
            if (chkTmr >= (SHORT_WAIT<<3)) {
                progress = NO_OTHER;
            }
        }

    } else {
        progress = NO_OTHER;
    }
      
    return 0;
}
