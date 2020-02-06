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
uint8_t simonGameStateNext = BASTET_BOOT;   // for stuff like LEDs / audio loops etc ??
uint8_t simonTimer = 0;

void setupSimon() {
    for (uint8_t i = 0; i < BASTET_LENGTH; i++) {
        simonState[i] = (lfsr() % 4);
    }
    simonGameState = BASTET_GAME_START;
}

void simonTone(uint8_t val) {
    // TODO some way to set teh audio stuff
}

void simonLed(uint8_t val) {
    for (uint8_t n = 0; n < 5; n++) {
        iLED[WING[L][n]] = 0;
        iLED[WING[R][n]] = 0;
    }
    if (val == 1) {
        iLED[WING[L][0]] = dimValue;
        iLED[WING[L][1]] = dimValue;
    } else if (val == 2) {
        iLED[WING[L][3]] = dimValue;
        iLED[WING[L][4]] = dimValue;
    } else if (val == 3) {
        iLED[WING[R][0]] = dimValue;
        iLED[WING[R][1]] = dimValue;
    } else if (val == 4) {
        iLED[WING[R][3]] = dimValue;
        iLED[WING[R][4]] = dimValue;
    }
    simonTone(val);
}

// Main game loop
uint8_t BastetDictates() {
    if (CheckState(BASTET_COMPLETED))
        return 0;

    if ((TEXT != gameNow) && (BASTET != gameNow))
        return 0;

    if (BASTET_BOOT == simonGameState) {
        setupSimon();
    }

    if (BASTET_GAME_START == simonGameState) {
        // TODO start animu
        simonGameState = BASTET_GAME_SHOW_PATTERN;
        simonTimer = 0;
    }

    if (BASTET_GAME_SHOW_PATTERN == simonGameState) {
        // assuming 15Hz
        uint8_t pos = simonTimer / 15;
        if (pos > simonPos) {
            simonGameState = BASTET_GAME_INPUT;
            simonLed(0);
            return 0;
        }
        simonLed(simonState[pos]+1);
    }

    if (BASTET_GAME_INPUT == simonGameState) {
        uint8_t choice = 0;
        switch (buttonState) {
            case 0b1000: // bottom left
                choice = 1;
                break;
            case 0b0100: // top left
                choice = 2;
                break;
            case 0b0010: // top right
                choice = 3;
                break;
            case 0b0001: // bottom right
                choice = 4;
                break;
        }
        if (choice > 0) {
            simonLed(choice);
            // TODO something timer something 
            if (simonState[simonInputPos]+1 == choice) {
                // TODO correct sound
                simonInputPos++;
            } else {
                // TODO fail sound
                simonInputPos = 0;
            }
            if (simonInputPos == BASTET_LENGTH) {
                // TODO win animu
                UpdateState(BASTET_COMPLETED);
            }
        }
    }
    ++simonTimer;
    return 0;
}
