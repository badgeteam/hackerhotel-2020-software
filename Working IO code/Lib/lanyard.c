/*
 * maze.c
 *
 * Created: 02/02/2020 0:56
 * Author: Badge.team
 */ 

#include <lanyard.h>
#include <main_def.h>
#include <resources.h>
#include <I2C.h>

const uint8_t   lanyardCode[] = { 1,0,0,2, 1,2,0,1, 1,3,0,3, 1,3,1,0, 1,2,1,1, 1,3,1,0 };
uint8_t         lanyardPos = 0;
uint8_t         lanyardCnt = 0;
uint8_t         lanyardState = TRUE;
uint8_t         lastButtonState = 0xff;
uint8_t         digit = 0xff;
uint16_t        lanyardLastActive = 0;

void initLanyard() {
    lanyardPos      = 0;
    lanyardCnt      = 0;
    lanyardState    = TRUE;
    lastButtonState = 0xff;
    iLED[CAT]       = 0;
    iLED[EYE[G][L]] = 0;
    iLED[EYE[G][R]] = 0;
    iLED[EYE[R][L]] = 0;
    iLED[EYE[R][R]] = 0;
    for (int i=0; i<6; i++ )
        iLED[HCKR[G][i]] = 0;
}

// Main game loop
uint8_t LanyardCode(){
    if (gameNow == LANYARD && idleTimeout(lanyardLastActive,LANYARD_MAX_IDLE)) {
        /* clean up maze game and go back to text game */
        initLanyard();
        gameNow = TEXT;
        effect = 0;
        return 0;
    }

    if (CheckState(LANYARD_COMPLETED))
        return 0;

    if ( gameNow != TEXT && gameNow != LANYARD )
        return 0;

    if ( (buttonState & 0xf0) == 0)
        return 0;

    /* activate led for buttonstate */
    iLED[CAT] = (buttonState==0xff ? 0 : dimValue);

    if ((buttonState&0x0f) == (lastButtonState&0x0f))
        return 0;

    lanyardLastActive = getClock();

    if (lastButtonState == 0xff){
        switch (buttonState & 0x0f) {
            case 0b0001:
                effect = 0x19f;
                digit = 0;
                break;

            case 0b0010:
                effect = 0x17f;
                digit = 1;
                break;

            case 0b0100:
                effect = 0x13f;
                digit = 3;
                break;

            case 0b1000:
                effect = 0x15f;
                digit = 2;
                break;

            default:
                digit = 0xff;
                break;
        }
        gameNow = LANYARD;
        /* TODO play tone for button */

        if (digit == lanyardCode[lanyardPos]) {
            lanyardState &= TRUE;
            iLED[EYE[R][L]] = 0;
            iLED[EYE[R][R]] = 0;
        } else {
            lanyardState = FALSE;
            if (lanyardPos < 4 ) {
                initLanyard();
                /* TODO disable bastet for now  until is working 
                gameNow         = BASTET;
                */
                gameNow         = TEXT;
                return 0;
            }
        }
        lanyardPos++;
        lanyardCnt++;            
        if (lanyardCnt >= 4) {
            lanyardCnt = 0;
            if (lanyardState == TRUE) {
                if ((lanyardPos % 4) == 0) {
                    iLED[HCKR[G][(lanyardPos/4)-1]] = dimValue;
                    effect = 0x5f;
                }
                if (lanyardPos == sizeof(lanyardCode)) {
                    UpdateState(LANYARD_COMPLETED);
                    iLED[CAT]       = 0;
                    effect = 0x42;
                    /*TODO state = STATE_MUSIC;*/
                }
            } else {
                initLanyard();
                effect = 0x21;
            }
        }
    }

    lastButtonState = buttonState;
    return 0;
}
