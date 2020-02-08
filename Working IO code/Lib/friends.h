/*
 * friends.h
 *
 * Created: 21/01/2020 15:42:42
 * Author: Badge.team
 */

// Friends
// Badge to badge "communication"

#ifndef FRIENDS_H_
#define FRIENDS_H_
    #include <stdlib.h>
    #include <avr/io.h>
    #include <avr/interrupt.h>
    #include <stdlib.h>
    #include <literals.h>

    #define DELTA           10

    #define NO_OTHER        0
    #define FIRST_CONTACT   1
    #define SECOND_LOVE     2
    #define THIRD_KISS      3
    #define FOURTH_BASE     4
    #define NEXT            0xf0

    #define SHORT_WAIT      8
    #define LONG_WAIT       250
  
    uint8_t MakeFriends();

#endif /* FRIENDS_H_ */
