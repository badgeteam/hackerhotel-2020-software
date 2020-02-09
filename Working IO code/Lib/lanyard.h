/*
 * lanyard.h
 *
 * Created: 02/02/2020 0:55
 * Author: Badge.team
 */

// Lanyard puzzle

#ifndef LANYARD_H_
#define LANYARD_H_
    #include <stdlib.h>
    #include <avr/io.h>
    #include <avr/interrupt.h>
    #include <stdlib.h>
    #include <literals.h>

    #define LANYARD_COMPLETED   123
    #define LANYARD_MAX_IDLE    10
//    #define LANYARD_LEN         24
    #define LANYARD_LEN         12

    #define LANYARD_GOOD        1
    #define LANYARD_MISTAKE     2
    #define LANYARD_GAMEOVER    3

    uint8_t LanyardCode();

#endif /* LANYARD_H_ */
