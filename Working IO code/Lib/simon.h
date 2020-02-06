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

    #define BASTET_COMPLETED             122
    #define BASTET_LENGTH                16
    #define BASTET_BOOT                  0
    #define BASTET_GAME_START            1
    #define BASTET_GAME_WAIT_LED_ON      2
    #define BASTET_GAME_WAIT_LED_OFF     3
    #define BASTET_GAME_WAIT_FOR_INPUT   4
    #define BASTET_GAME_WAIT_AFTER_INPUT 5
    #define BASTET_GAME_SHOW_PATTERN     6
    #define BASTET_GAME_INPUT            7

    uint8_t BastetDictates();

    void setupSimon();
    void simonTone(uint8_t val);
    uint8_t getSimonValue(uint8_t pos);
    void setSimonValue(uint8_t pos, uint8_t val);
    void simonLed(uint8_t val);

#endif /* SIMON_H_ */
