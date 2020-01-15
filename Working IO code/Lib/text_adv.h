/*
 * text_adv.h
 *
 * Contains all the crap that is needed for the text adventure 
 *
 * Created: 15/01/2020 18:25:19
 *  Author: Badge.team
 */ 


#ifndef TEXT_ADV_H_
#define TEXT_ADV_H_
    #include <main_def.h>
    #include <avr/io.h>
    #include <avr/interrupt.h>
    #include <util/delay.h>
    #include <stdlib.h>

    //Byte fields
    enum {EFFECTS, VISIBLE_ACL, OPEN_ACL, ACTION_ACL, ACTION_MASK, ACTION_ITEM, ACTION_STATE, ITEM_NR};
    #define BYTE_FIELDS_LEN     ITEM_NR+1

    //string fields
    enum {NAME, DESC, ACTION_STR1, ACTION_STR2, OPEN_ACL_MSG, ACTION_ACL_MSG, ACTION_MSG};
    #define STRING_FIELDS_LEN   ACTION_MSG+1

    //Keys are md5 hash of 'H@ck3r H0t3l 2020', split in two
    //const uint8_t xor_key_game[8]  = {0x74, 0xbf, 0xfa, 0x54, 0x1c, 0x96, 0xb2, 0x26};
    //const uint8_t xor_key_teaser[8] = {0x1e, 0xeb, 0xd6, 0x8b, 0xc0, 0xc2, 0x0a, 0x61};
    //const unsigned char boiler_plate[]   = "Hacker Hotel 2020 by badge.team "; // boiler_plate must by 32 bytes long, excluding string_end(0)

#endif /* RESOURCES_H_ */