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

    uint8_t MagnetMaze();

#endif /* MAZE_H_ */
