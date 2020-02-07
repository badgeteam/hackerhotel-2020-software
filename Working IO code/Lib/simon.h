/*
 * simon.c
 *
 * Created: 21/01/2020 15:42:42
 * Author: Badge.team
 */

// Bastet Dictates
// Simon clone

#ifndef SIMON_H_
#define SIMON_H_
    #include <stdlib.h>
    #include <avr/io.h>
    #include <avr/interrupt.h>
    #include <stdlib.h>
    #include <literals.h>

    #define BASTET_COMPLETED            122
    #define BASTET_LENGTH               12
    #define BASTET_BOOT                 0
    #define BASTET_GAME_START           1
    #define BASTET_GAME_INTRO           2
    #define BASTET_GAME_SHOW_PATTERN    3
    #define BASTET_GAME_WAIT_LEDS       4
    #define BASTET_GAME_INPUT           5
    #define BASTET_GAME_OUTRO           6
    #define BASTET_GAME_OVER            7

    void simonLed(uint8_t val);
    uint8_t BastetDictates();

#endif /* SIMON_H_ */
