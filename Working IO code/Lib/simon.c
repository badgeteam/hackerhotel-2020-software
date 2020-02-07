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
#include <main_def.h>
#include <resources.h>
#include <I2C.h>

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
    if (CheckState(BASTET_COMPLETED))
        return 0;

    if ((TEXT != gameNow) && (BASTET != gameNow))
        return 0;

    if (BASTET_BOOT == simonGameState) {
        simonPos = ((adcPhot+adcTemp)&0x3f);
        for(uint8_t x=0; x<simonPos; ++x) lfsr();

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
    }

    if (BASTET_GAME_INTRO == simonGameState) {
        if (simonTimer > 5) {   // ±.33 seconds
            simonCounter++;
            simonTimer = 0;
        }
        if (simonCounter > 6) {
            simonGameState = BASTET_GAME_SHOW_PATTERN;
//            for (uint8_t n=0; n<6; n++){
//                iLED[HCKR[R][n]] = 1;
//            }
            simonTimer = 0;
            simonCounter = 0;
            return 0;
        }
        iLED[HCKR[R][simonCounter]] = dimValue;
    }

    if (BASTET_GAME_SHOW_PATTERN == simonGameState) {
        if (simonTimer > 15) {   // ± ! second
            simonCounter++;
            simonTimer = 0;
        }
        if (simonCounter > simonPos) {
            simonGameState = BASTET_GAME_INPUT;
            simonLed(0);
            simonTimer = 0;
            return 0;
        }
        simonLed(simonState[simonCounter]+1);
    }

    if (BASTET_GAME_INPUT == simonGameState) {
        //Button pressed
        if ((buttonState+1) > 0) {
            if (simonWait == 0) {
                simonWait = 1;
                simonTimer = 0;
                simonLed(buttonState+1);
                simonGameState = BASTET_GAME_WAIT_LEDS;
                simonNextGameState = BASTET_GAME_INPUT;

                // is the below check correct .. do I have off by one ?
                if (simonState[simonInputPos] == (buttonState+1)) {
                    simonInputPos++;
                    if (simonInputPos >= simonPos) {
                        effect = 0x42;
                        simonPos++;
                        simonInputPos = 0;
                        simonTimer = 0;
                        simonCounter = 0;
                        simonNextGameState = BASTET_GAME_SHOW_PATTERN;
                    }
                } else {
                    effect = 0x0105;    // TODO fail sound
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
                    // TODO win animu !!
                    effect = 0x0106;    // TODO win sound ?!
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
    }

    if (BASTET_GAME_WAIT_LEDS == simonGameState) {
        if (simonTimer >= 7) {
            // on to next state after ±.5 second
            simonLed(0);  // LEDs off
            simonWait = 0;
            simonTimer = 0;
            simonCounter = 0;
            simonGameState = simonNextGameState;
        }
    }

    if (BASTET_GAME_OUTRO == simonGameState) {
        if (simonTimer > 5) {   // ±.33 second
            simonCounter++;
            simonTimer = 0;
        }
        if (simonCounter > 6) {
            simonTimer = 0;
            simonCounter = 0;
            simonGameState = BASTET_GAME_OVER;
            return 0;
        }
        iLED[HCKR[R][5-simonCounter]] = 1;
    }

    if (BASTET_GAME_OVER == simonGameState) {
        simonInputPos = 0;
        simonPos = 0;
        simonGameState = BASTET_GAME_START; // BASTET_BOOT for fresh "field" ??
        gameNow = TEXT;
        for (uint8_t n=0; n<5; n++){
            iLED[WING[L][n]] = 1;
            iLED[WING[R][n]] = 1;
        }
    }

    ++simonTimer;
    return 0;
}

/**
 * Set the wing LED blocks to selected option
 * @param val
 */
void simonLed(uint8_t val) {
    for (uint8_t n = 0; n < 5; n++) {
        iLED[WING[L][n]] = 0;
        iLED[WING[R][n]] = 0;
    }
    if (val == 3) {         // II
        iLED[WING[L][0]] = dimValue;
        iLED[WING[L][1]] = dimValue;
        iLED[WING[L][2]] = dimValue;
        effect = 0x013f;
    } else if (val == 1) {  //
        iLED[WING[L][3]] = dimValue;
        iLED[WING[L][4]] = dimValue;
        effect = 0x015f;
    } else if (val == 4) {  // III
        iLED[WING[R][0]] = dimValue;
        iLED[WING[R][1]] = dimValue;
        iLED[WING[R][2]] = dimValue;
        effect = 0x017f;
    } else if (val == 2) {  // I
        iLED[WING[R][3]] = dimValue;
        iLED[WING[R][4]] = dimValue;
        effect = 0x019f;
    }
}
