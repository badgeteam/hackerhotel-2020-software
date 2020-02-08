/*
 * maze.h
 *
 * Created: 02/02/2020 0:55
 * Author: Badge.team
 */

// Magnetic Maze

#ifndef MAZE_H_
#define MAZE_H_
    #include <stdlib.h>
    #include <avr/io.h>
    #include <avr/interrupt.h>
    #include <stdlib.h>
    #include <literals.h>

    #define MAZE_INACTIVE       119+128
    #define MAZE_COMPLETED      125
    #define HALL_LOW            30*4
    #define HALL_HIGH           75*4
    #define MAZE_MAX_IDLE       10

    // optimizing for size by pre-calculation compare values
    #define NEG_HALL_LOW        HALL_LOW * -1
    #define NEG_HALL_HIGH       HALL_HIGH * -1
    #define HALL_FIELD_0        HALL_LOW/2
    #define HALL_FIELD_1        HALL_LOW
    #define HALL_FIELD_2        HALL_HIGH/2
    #define HALL_FIELD_3        HALL_HIGH*2/3
    #define HALL_FIELD_4        HALL_HIGH
    #define MAZE_LEN            18
    #define HALL_IDLE           2
    #define HALL_N              0
    #define HALL_S              1


    uint8_t MagnetMaze();

#endif /* MAZE_H_ */
