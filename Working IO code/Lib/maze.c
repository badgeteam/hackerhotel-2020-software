/*
 * maze.c
 *
 * Created: 02/02/2020 0:56
 * Author: Badge.team
 * \                           /
 *  \                         /
 *   \                       /
 *    ]                     [    ,'|
 *    ]                     [   /  |
 *    ]___               ___[ ,'   |
 *    ]  ]\             /[  [ |:   |
 *    ]  ] \           / [  [ |:   |
 *    ]  ]  ]         [  [  [ |:   |
 *    ]  ]  ]__     __[  [  [ |:   |
 *    ]  ]  ] ]\ _ /[ [  [  [ |:   |
 *    ]  ]  ] ] (#) [ [  [  [ :===='
 *    ]  ]  ]_].nHn.[_[  [  [
 *    ]  ]  ]  HHHHH. [  [  [
 *    ]  ] /   `HH("N  \ [  [
 *    ]__]/     HHH  "  \[__[  O!o
 *    ]         NNN         [
 *    ]         N/"         [
 *    ]         N H         [
 *   /          N            \
 *  /           q,            \
 * /                           \
 */ 

#include <maze.h>
#include <main_def.h>
#include <resources.h>
#include <I2C.h>

uint8_t         curHallState = 0;
uint8_t         newHallState = 0;

// optimizing for size, also need to put the bits in reverse
//const uint8_t   mazeCode[] = {1,1,2, 1,2,2, 1,2,2, 1,2,2, 2,2,2, 1,1,2};
const uint8_t   mazeCode[] = {0b10110100,0b01111101,0b10};
uint8_t         mazePos = 0;
uint8_t         mazeHckrPos = 0;
uint8_t         mazeCnt = 0;
uint8_t         mazeState = TRUE;
uint8_t         inverted  = FALSE;
uint16_t        mazeLastActive = 0;

void initMaze() {
    mazeHckrPos = 0;
    mazePos = 0;
    mazeCnt = 0;
    mazeState = TRUE;
    inverted  = FALSE;
    effect = 0;
}

void showFieldStrength(int16_t val) {
    int16_t field;

    field = abs(val);

    if ( field > HALL_FIELD_0 )
        gameNow = MAZE;

    if (gameNow == MAZE) {
        if ( field < HALL_FIELD_0 )
            WingBar(0,0);
        else if ( field < HALL_FIELD_1 )
            WingBar(1,1);
        else if ( field < HALL_FIELD_2 )
            WingBar(2,2);
        else if ( field < HALL_FIELD_3 )
            WingBar(3,3);
        else if ( field < HALL_FIELD_4 )
            WingBar(4,4);
        else
            WingBar(5,5);
    }
}

// Main game loop
uint8_t MagnetMaze(){
    if (gameNow == MAZE && idleTimeout(mazeLastActive,MAZE_MAX_IDLE)) {
        /* clean up maze game and go back to text game */
        initMaze();
        gameNow = TEXT;
        return 0;
    }
        
    if (CheckState(MAZE_INACTIVE))
        return 0;

    if (CheckState(MAZE_COMPLETED))
        return 0;

    if ( (gameNow != TEXT) && (gameNow != MAZE) )
        return 0;

    if (calHall == 0)
        calHall = adcHall;

    int16_t valHall = adcHall - calHall;
    showFieldStrength(valHall);

    switch (curHallState) {
        case HALL_IDLE:
            if ( valHall < NEG_HALL_HIGH ) {
                newHallState = HALL_N;
            } else if ( valHall > HALL_HIGH ) {
                newHallState = HALL_S;
            } else {
                newHallState = HALL_IDLE;
            }
            break;
        
        case HALL_N:
            if ( valHall > HALL_HIGH ) {
                newHallState = HALL_S;
            } else if ( valHall > NEG_HALL_LOW ) {
                newHallState = HALL_IDLE;
            } else {
                newHallState = HALL_N;
            }
            break;
        
        case HALL_S:
            if ( valHall < NEG_HALL_HIGH ) {
                newHallState = HALL_N;
            } else if ( valHall < HALL_LOW ) {
                newHallState = HALL_IDLE;
            } else {
                newHallState = HALL_S;
            }
            break;
    }

    /* Indicate that the magnet was read succesfully */
    iLED[CAT] = (newHallState == HALL_IDLE ? 0 : dimValue);
    effect = 0x19f;

    if (newHallState != curHallState) {
        /* keep track of time to enable an idle timeout to exit the game */
        mazeLastActive = getClock();

        /* Make the maze work regardless of badge orientation */
        if ((mazePos == 0) && (newHallState != 0))
            inverted = (newHallState != (mazeCode[0]&1)) ? TRUE : FALSE;

        curHallState = newHallState;
        
        if (curHallState != 0) {
            if (gameNow == TEXT)
                initMaze();
            gameNow = MAZE;

            if ( (inverted ? curHallState^1 : curHallState) == (mazeCode[mazePos>>3]&(mazePos&7)) ) {
                mazeState &= TRUE;
                iLED[EYE[R][L]] = 0;
                iLED[EYE[R][R]] = 0;
            } else {
                mazeState = FALSE;
            }
            mazePos++;
            mazeCnt++;            
            if (mazeCnt >= 3) {
                mazeCnt = 0;
                if (mazeState == TRUE) {
                    iLED[HCKR[G][mazeHckrPos]] = dimValue;
                    mazeHckrPos++;
                    if (mazePos == MAZE_LEN) {
                        UpdateState(MAZE_COMPLETED);
                        iLED[CAT]       = 0;
                        effect = 0x42;
                        /*TODO state = STATE_MUSIC;*/
                    }
                } else {
                    initMaze();
                    gameNow   = TEXT;
                    effect = 0x21;
                }
            }
        }
    }
    return 0;
}
