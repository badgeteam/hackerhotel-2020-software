/*
 * text_adv.h
 *
 * Contains all the crap that is needed for the text adventure 
 *
 * Created: 15/01/2020 18:25:19
 *  Author: Badge.team
 */ 

 // Game loop:
 // Check if still sending data, true -> end loop
 // Check if input present, false -> end loop
 // Process input, put response in tx variables for sending to user
 // :End of loop

 // Playing of game audio and LED effects are done outside this library

#ifndef TEXT_ADV_H_
#define TEXT_ADV_H_
    #include <avr/io.h>
    #include <avr/interrupt.h>
    #include <stdlib.h>
    #include <literals.h>

    //Object model offsets
    enum{OFF_NEXTOBJ = 0, OFF_NEXTLVL = 2, OFF_BYTEFLDS = 4};

    //Byte fields
    enum {EFFECTS, VISIBLE_ACL, OPEN_ACL, ACTION_ACL, ACTION_MASK, ACTION_ITEM, ACTION_STATE, ITEM_NR};
    #define BYTE_FIELDS_LEN     ITEM_NR+1

    //string fields
    enum {NAME, DESC, ACTION_STR1, ACTION_STR2, OPEN_ACL_MSG, ACTION_ACL_MSG, ACTION_MSG};
    #define STRING_FIELDS_LEN   ACTION_MSG+1
    #define OFF_STRINGFLDS      OFF_BYTEFLDS+BYTE_FIELDS_LEN

    //Object model
    typedef struct {
        uint16_t addrNextObj;
        uint16_t addrNextLvl;
        uint8_t  byteField[BYTE_FIELDS_LEN];
        uint16_t addrStr[STRING_FIELDS_LEN];
        uint16_t lenStr[STRING_FIELDS_LEN];
    } object_model_t;

    //Action constants
    enum {ENTER = 1, OPEN = 2, LOOK = 4, TALK = 8, USE = 16};

    //Extra text additions constants
    enum {PROMPT = 1, SPACE, CR_1, CR_2, LOCATION};

    //
    #define MAX_OBJ_DEPTH   32
    #define MAX_ITEMS       64
    #define STATUS_BITS     sizeof(gameState)*8
    
    #define EXT_EE_MAX      32767

    //Keys are md5 hash of 'H@ck3r H0t3l 2020', split in two
    #define KEY_LENGTH      8
    enum {GAME = 0, TEASER = 1};
    extern const uint8_t xor_key[2][KEY_LENGTH];

    uint8_t TextAdventure();

#endif /* TEXT_ADV_H_ */
