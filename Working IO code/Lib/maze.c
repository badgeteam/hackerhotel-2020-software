/*
 * maze.c
 *
 * Created: 02/02/2020 0:56
 * Author: Badge.team
 */ 

#include <maze.h>
#include <main_def.h>
#include <resources.h>
#include <I2C.h>

uint8_t         curHallState = 0;
uint8_t         newHallState = 0;

const uint8_t   mazeCode[] = {1,1,2,1,2,2,1,2,2,1,2,2,2,2,2,1,1,2};
uint8_t         mazePos = 0;
uint8_t         mazeCnt = 0;
uint8_t         mazeState = TRUE;
uint8_t         inverted  = FALSE;

// Main game loop
uint8_t MagnetMaze(){
    if (CheckState(MAZE_INACTIVE))
        return 0;

    if (CheckState(MAZE_COMPLETED))
        return 0;

    if ( gameNow != TEXT && gameNow != MAZE )
        return 0;

    if (calHall == 0)
        calHall = adcHall;

    int16_t valHall = adcHall - calHall;
    switch (curHallState) {
        case 0: {
            if ( valHall + HALL_HIGH < 0 ) {
                newHallState = 1;
            } else if ( valHall - HALL_HIGH > 0 ) {
                newHallState = 2;
            } else {
                newHallState = 0;
            }
            break;
        }
        
        case 1: {
            if ( valHall - HALL_HIGH > 0 ) {
                newHallState = 2;
            } else if ( valHall + HALL_LOW > 0 ) {
                newHallState = 0;
            } else {
                newHallState = 1;
            }
            break;
        }
        
        case 2: {
            if ( valHall + HALL_HIGH < 0 ) {
                newHallState = 1;
            } else if ( valHall - HALL_LOW < 0 ) {
                newHallState = 0;
            } else {
                newHallState = 2;
            }
            break;
        }
    }

    /* activate led for hallstate */
    switch (newHallState) {
        case 0: {
            iLED[GEM[G]]    = 0;
            break;
        }

        default: {
            iLED[GEM[G]]    = 255;
            break;
        }
    }


    /* Make the maze work regardless of badge orientation */
    if (newHallState != 0 && mazePos == 0 )
        inverted = (newHallState != mazeCode[0]) ? TRUE : FALSE;

    if (newHallState != curHallState) {
        curHallState = newHallState;
        
        if (curHallState != 0) {
            gameNow = MAZE;
            if ( (inverted ? curHallState^3 : curHallState) == mazeCode[mazePos]) {
                iLED[EYE[R][L]] = 0;
                iLED[EYE[R][R]] = 0;
                mazePos++;
                mazeCnt++;            
                /* play tone GOOD */
                if (mazeCnt >= 3) {
                    /* setHackerScore((mazePos)/3); */
                    mazeCnt = 0;
                }
            } else {
                mazeState = FALSE;
                /* play tone BAD */
                gameNow   = TEXT;
                mazePos   = 0;
                iLED[GEM[G]]    = 0;
                iLED[EYE[G][L]] = 0;
                iLED[EYE[G][R]] = 0;
                iLED[EYE[R][L]] = 255;
                iLED[EYE[R][R]] = 255;
                /*
                setHackerScore(0);
                */
            }
        } 
        if (mazePos == sizeof(mazeCode)) {
            UpdateState(MAZE_COMPLETED);
            iLED[GEM[G]]    = 0;
            iLED[EYE[R][L]] = 0;
            iLED[EYE[R][R]] = 0;
            iLED[EYE[G][L]] = 255;
            iLED[EYE[G][R]] = 255;
            /*state = STATE_MUSIC;*/
        }
    }
    return 0;
}
