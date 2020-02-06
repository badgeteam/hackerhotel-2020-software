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

// Main game loop
uint8_t LanyardCode(){
    if (CheckState(LANYARD_COMPLETED)) {
        /*
        if (lanyardPos == sizeof(lanyardCode)) {
            gameNow   = TEXT;
            lanyardPos   = 0;
            lanyardState = TRUE;
            iLED[SCARAB[G]]    = 0;
            iLED[EYE[G][L]] = 0;
            iLED[EYE[G][R]] = 0;
            iLED[EYE[R][L]] = 0;
            iLED[EYE[R][R]] = 0;
            for (int i=0; i<6; i++ )
                iLED[HCKR[G][i]] = 0;
        }
        */
        return 0;
    }

    if ( gameNow != TEXT && gameNow != LANYARD )
        return 0;

    /* activate led for buttonstate */
    iLED[SCARAB[G]] = (buttonState==0xff ? 0 : 255);

    if ( (buttonState & 0xf0) == 0)
        return 0;

    if ((buttonState&0x0f) == (lastButtonState&0x0f))
        return 0;

    if (lastButtonState == 0xff){
        switch (buttonState & 0x0f) {
            case 0b0001: {
                digit = 0;
                break;
            }

            case 0b0010: {
                digit = 1;
                break;
            }

            case 0b0100: {
                digit = 3;
                break;
            }

            case 0b1000: {
                digit = 2;
                break;
            }

            default: {
                digit = 0xff;
                break;
            }
        }
        gameNow = LANYARD;
        /* play tone for button */

        if (digit == lanyardCode[lanyardPos]) {
            lanyardState &= TRUE;
            iLED[CAT]       = 128;
            iLED[EYE[R][L]] = 0;
            iLED[EYE[R][R]] = 0;
        } else {
            lanyardState = FALSE;
            iLED[CAT]       = 0;
            if (lanyardPos < 4 ) {
                /*gameNow         = BASTET;*/
                gameNow         = TEXT;
                lanyardPos      = 0;
                lanyardCnt      = 0;
                lanyardState    = TRUE;
                lastButtonState = 0xff;
                return 0;
            }
        }
        lanyardPos++;
        lanyardCnt++;            
        if (lanyardCnt >= 4) {
            lanyardCnt = 0;
            if (lanyardState == TRUE) {
                iLED[HCKR[G][(lanyardPos/4)-1]] = 255;
                if (lanyardPos == sizeof(lanyardCode)) {
                    UpdateState(LANYARD_COMPLETED);
                    iLED[SCARAB[G]]    = 0;
                    iLED[EYE[R][L]] = 0;
                    iLED[EYE[R][R]] = 0;
                    iLED[EYE[G][L]] = 255;
                    iLED[EYE[G][R]] = 255;
                    /*state = STATE_MUSIC;*/
                }
            } else {
                gameNow         = TEXT;
                lanyardPos      = 0;
                lanyardState    = TRUE;
                lastButtonState = 0xff;
                iLED[SCARAB[G]]    = 0;
                iLED[EYE[G][L]] = 0;
                iLED[EYE[G][R]] = 0;
                iLED[EYE[R][L]] = 255;
                iLED[EYE[R][R]] = 255;
                for (int i=0; i<6; i++ )
                    iLED[HCKR[G][i]] = 0;
            }
        }
    }

    lastButtonState = buttonState;
    return 0;
}
