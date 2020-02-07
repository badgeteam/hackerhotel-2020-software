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
uint8_t simonPos = 1;
uint8_t simonInputPos = 0;
uint8_t simonGameState = BASTET_BOOT;
uint8_t simonTimer = 0;
uint8_t simonProcessed = 0;

void simonLed(uint8_t val) {
    for (uint8_t n = 0; n < 5; n++) {
        iLED[WING[L][n]] = 0;
        iLED[WING[R][n]] = 0;
    }
    if (val == 1) {
        iLED[WING[L][0]] = dimValue;
        iLED[WING[L][1]] = dimValue;
        iLED[WING[L][2]] = dimValue;
        effect = 0x0101;
    } else if (val == 2) {
        iLED[WING[L][3]] = dimValue;
        iLED[WING[L][4]] = dimValue;
        effect = 0x0102;
    } else if (val == 3) {
        iLED[WING[R][0]] = dimValue;
        iLED[WING[R][1]] = dimValue;
        iLED[WING[R][2]] = dimValue;
        effect = 0x0103;
    } else if (val == 4) {
        iLED[WING[R][3]] = dimValue;
        iLED[WING[R][4]] = dimValue;
        effect = 0x0104;
    }
}

// Main game loop
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
        simonPos = 1;
        simonGameState = BASTET_GAME_START;
    }

    if (BASTET_GAME_START == simonGameState) {
        // TODO start animu + sound ?
        simonGameState = BASTET_GAME_SHOW_PATTERN;
        simonTimer = 0;
    }

    if (BASTET_GAME_SHOW_PATTERN == simonGameState) {
        // assuming 15Hz
        uint8_t pos = simonTimer / (15 - (simonPos>>1));
        if (pos > simonPos) {
            simonGameState = BASTET_GAME_INPUT;
            simonLed(0);
            return 0;
        }
        simonLed(simonState[pos]);
    }

    if (BASTET_GAME_INPUT == simonGameState) {
        uint8_t choice = 0;
        if ((buttonState & 0xf0)&&(buttonState < 0xff)) {
            switch (buttonState&0x0f) {
                case 0b1000: // bottom left
                    choice = BASTET_BOTTOM_LEFT;
                    break;
                case 0b0100: // top left
                    choice = BASTET_TOP_LEFT;
                    break;
                case 0b0010: // top right
                    choice = BASTET_TOP_RIGHT;
                    break;
                case 0b0001: // bottom right
                    choice = BASTET_BOTTOM_RIGHT;
                    break;
            }
        }

        //Button pressed
        if (choice > 0) {
            gameNow = BASTET;
            if (simonProcessed == 0) {
                simonTimer = 0;
                simonLed(choice);
                simonGameState = BASTET_GAME_WAIT_LEDS;

                if (simonState[simonInputPos]+1 == choice) {
                    // TODO correct, next sound (textadv?)
                    simonInputPos++;
                } else {
                    // TODO fail sound (textadv?)
                    simonInputPos = 0;
                    gameNow = TEXT;
                    simonLed(0);
                }

                if (simonInputPos == BASTET_LENGTH) {
                    // TODO win animu + sound ?
                    UpdateState(BASTET_COMPLETED);
                    gameNow = TEXT;
                }
            }
        
        //Button released, next or reset!
        } else {    
            if (simonInputPos == simonPos) {
                simonPos++;
                simonInputPos = 0;
                simonGameState = BASTET_GAME_SHOW_PATTERN;
            }                    
            simonProcessed = 0;
        }
    }

    if (BASTET_GAME_WAIT_LEDS == simonGameState) {
        if (simonTimer >= 7) {
            // on to next state after ±.5 second
            simonLed(0);  // LEDs off
            simonGameState = BASTET_GAME_INPUT;
        }
    }

    ++simonTimer;
    return 0;
}
