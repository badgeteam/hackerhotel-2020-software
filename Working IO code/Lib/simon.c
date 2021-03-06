/*
 * simon.c
 *
 * Created: 21/01/2020 15:42:42
 * Author: Badge.team
 *　　　　　　 ＿＿
 *　　　　　／＞　　フ
 *　　　　　|  _　 _l  My name is Bastet, not Simon!
 *　 　　　／`ミ＿xノ
 *　　 　 /　　　♥　\
 *　　　 /　 ヽ　　 ﾉ
 *　 　 │　　|　|　|
 *　／￣|　　 |　|　|
 *　| (￣ヽ＿_ヽ_)__)
 *　＼二つ
 */

#include <simon.h>
#include <lanyard.h>
#include <main_def.h>
#include <resources.h>

uint16_t simonScore = 0;
uint8_t simonState[BASTET_LENGTH] = {0};
uint8_t simonPos = 0,
        simonInputPos = 0,
        simonTimer = 0,
        simonWait = 0,
        simonCounter = 0;
uint8_t simonGameState = BASTET_BOOT,
        simonNextGameState = BASTET_GAME_INPUT;

/**
 * Main game loop
 * @return 0
 */
uint8_t BastetDictates() {
    if ((TEXT != gameNow) && (BASTET != gameNow))
        return 0;

    /* Keep the badge as a SimonSays^WBastetDictates game, even if you completed the challenge
    if (CheckState(BASTET_COMPLETED)) {
        if (BASTET == gameNow) {
            gameNow = TEXT;
        }
        return 0;
    }
    */

    if (TEXT == gameNow)
        if (buttonState!=0xff) {
            WingBar(0,0);
            gameNow = BASTET;
        }

    if (BASTET_BOOT == simonGameState) {
        for (uint8_t i = 0; i < BASTET_LENGTH; i++) {
            simonState[i] = (lfsr() % 4);
        }
        simonGameState = BASTET_GAME_START;
    }

    if (BASTET_GAME_START == simonGameState && BASTET == gameNow) {
        simonGameState = BASTET_GAME_INTRO;
        simonPos = 0;
        simonInputPos = 0;
        simonTimer = 0;
        simonCounter = 0;
        effect = 0;
    }

    if (BASTET_GAME_INTRO == simonGameState) {
        if (simonTimer > 3) {   // ±? seconds
            simonCounter++;
            simonTimer = 0;
        }
        if (simonCounter > 5) {
            simonGameState = BASTET_GAME_SHOW_PATTERN;
            simonTimer = 0;
            simonCounter = 0;
            SetHackerLeds(0,0);
            return 0;
        }
        iLED[HCKR[R][simonCounter]] = dimValue;
        iLED[HCKR[G][simonCounter]] = 0;
    }

    if (BASTET_GAME_SHOW_PATTERN == simonGameState) {
        if (simonTimer > 8) {   // ±.6 second
            simonCounter++;
            simonTimer = 0;
        }
        if (simonCounter > simonPos) {
            simonInputPos = 0;
            simonGameState = BASTET_GAME_INPUT;
            simonLed(0);
            simonTimer = 0;
            return 0;
        }
        if (simonTimer > 7 ) {
            simonLed(0);
        } else if (simonTimer > 5) {
            simonLed(simonState[simonCounter] + 1);
        }
    }

    if (BASTET_GAME_INPUT == simonGameState) {
        //Button pressed
        if (lastButtonState != buttonState && (buttonState+1) > 0) {
            if (simonWait == 0 && buttonState < 4) {
                simonWait = 1;
                simonTimer = 0;
                simonLed(buttonState+1);
                simonGameState = BASTET_GAME_WAIT_LEDS;
                simonNextGameState = BASTET_GAME_INPUT;

                if (simonState[simonInputPos] == buttonState) {
                    simonInputPos++;
                    if (simonInputPos > simonPos) {
                        simonPos++;
                        simonTimer = 0;
                        simonCounter = 0;
                        if (simonPos > 1) { // BASTET_LENGTH = 12
                            iLED[HCKR[G][(simonPos / 2) - 1]] = dimValue;
                        }
                        simonNextGameState = BASTET_GAME_SHOW_PATTERN;
                    }
                } else {
                    effect = 32|1;
                    for (uint8_t n=0; n<6; n++){
                        iLED[HCKR[R][n]] = dimValue;
                    }
                    simonInputPos = 0;
                    simonPos = 0;
                    simonTimer = 0;
                    simonCounter = 0;
                    simonNextGameState = BASTET_GAME_OUTRO;
                }

                if (simonInputPos >= BASTET_LENGTH || simonPos >= BASTET_LENGTH) { // beetje dubbel
                    effect = 64|2;
                    UpdateState(BASTET_COMPLETED);
                    simonTimer = 0;
                    simonCounter = 0;
                    simonNextGameState = BASTET_GAME_OVER;
                }
            }
        
        //Button released, next or reset!
        } else {
            simonWait = 0;
        }

        if (simonTimer == 75) {    // did you forget about Bastet?
            simonGameState = BASTET_GAME_SHOW_PATTERN;
            simonCounter = 0;
            simonTimer = 0;
            return 0;
        }
    }

    if (BASTET_GAME_WAIT_LEDS == simonGameState) {
        if (simonTimer >= 3) {
            // on to next state after ±.2 second
            simonLed(0);  // LEDs off
            simonWait = 0;
            simonTimer = 0;
            simonCounter = 0;
            simonGameState = simonNextGameState;
        }
    }

    if (BASTET_GAME_OUTRO == simonGameState) {
        if (simonTimer > 3) {
            simonCounter++;
            simonTimer = 0;
        }
        if (simonCounter > 5) {
            simonTimer = 0;
            simonCounter = 0;
            simonGameState = BASTET_GAME_OVER;
            effect = 0;
            return 0;
        }
        iLED[HCKR[R][5-simonCounter]] = 0;
        iLED[HCKR[G][5-simonCounter]] = 0;
    }

    if (BASTET_GAME_OVER == simonGameState) {
        simonInputPos = 0;
        simonPos = 0;
        simonGameState = BASTET_BOOT; // BASTET_GAME_START for stale "field" ツ
        gameNow = TEXT;
        simonLed(0);
    }

    ++simonTimer;
    return 0;
}

/**
 * Set the wing LED blocks to selected option
 * @param val
 */
void simonLed(uint8_t val) {
    WingBar(0,0);
    if (val==0)
        return;

    if ((effect & 0xffe0)==0)
        effect = 0x13f + ((val-1)<<5);

    if (val == 1) {                     //
        iLED[WING[L][4]] = dimValue;
    } else if (val == 2) {              // I
        iLED[WING[R][4]] = dimValue;
    } else if (val == 3) {              // II
        iLED[WING[L][2]] = dimValue;
    } else if (val == 4) {              // III
        iLED[WING[R][2]] = dimValue;
    }
}
