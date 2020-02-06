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
uint8_t simonPos = 0;
uint8_t simonInputPos = 0;
uint8_t simonLen = 1;
uint8_t simonGameState = BASTET_BOOT;

void setupSimon() {
    for (uint8_t i = 0; i < BASTET_LENGTH; i++) {
        simonState[i] = (lfsr() % 4);
    }
    simonGameState = BASTET_GAME_START;
}

void simonTone(uint8_t val) {
    // if (val == 1) {
    //   tone(PIN_AUDIO, 200);
    // } else if (val == 2) {
    //   tone(PIN_AUDIO, 400);
    // } else if (val == 3) {
    //   tone(PIN_AUDIO, 600);
    // } else if (val == 4) {
    //   tone(PIN_AUDIO, 800);
    // } else {
    //   noTone(PIN_AUDIO);
    // }
}

void simonLed(uint8_t val) {
    simonTone(val);
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
        // TODO Animu
        simonGameState = BASTET_GAME_SHOW_PATTERN;
    } else if (BASTET_GAME_SHOW_PATTERN == simonGameState) {

    }

    return 0;
}
