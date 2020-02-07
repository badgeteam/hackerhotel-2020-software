/*
 * text_adv.c
 *
 * Created: 06/01/2020 18:26:44
 *  Author: Badge.team
 */ 

#include <text_adv.h>           //Text adventure stuff
#include <main_def.h>
#include <resources.h>
#include <I2C.h>                //Fixed a semi-crappy lib found on internet, replace with interrupt driven one? If so, check hardware errata pdf!

//Keys are md5 hash of 'H@ck3r H0t3l 2020', split in two
const uint8_t xor_key[2][KEY_LENGTH] = {{0x74, 0xbf, 0xfa, 0x54, 0x1c, 0x96, 0xb2, 0x26},{0x1e, 0xeb, 0xd6, 0x8b, 0xc0, 0xc2, 0x0a, 0x61}};
//const unsigned char boiler_plate[]   = "Hacker Hotel 2020 by badge.team "; // boiler_plate must by 32 bytes long, excluding string_end(0)
#define L_BOILER    32

//Serial Tx string glue and send stuff
#define  TXLISTLEN  8
uint16_t txAddrList[TXLISTLEN]  = {0};      //List of external EEPROM addresses of first element of strings to be glued together
uint8_t txStrLen[TXLISTLEN]     = {0};      //List of lengths of strings to be glued together
uint8_t txAddrNow = 0;                      //Number of the string that currently is being sent
uint8_t sendPrompt = 0;
uint8_t txTypeNow = GAME;                   //Type of data that is being sent.
uint8_t txBuffer[TXLEN];                    //Buffer for string data

static object_model_t currObj;                      //Object data for current posistion in game
static uint8_t currDepth = 0xff;                    //Depth of position in game, 0xff indicates game data is not loaded from eeprom yet.
static uint16_t route[MAX_OBJ_DEPTH] = {0};         //
static uint16_t reactStr[3][MAX_REACT] = {{0},{0},{0}};    //
static uint8_t responseList = 0;                    //
static char specialInput[INP_LENGTH] = {0};         //Sometimes a special input is requested by the game. When this is set, input is compared to this string first.
static uint8_t specialPassed = 0;                   //If user input matches specialInput, this is set.
static uint16_t PunishmentTime = 0;

uint8_t Cheat(uint8_t bit, uint16_t checksum){
    const uint16_t chk[MAX_CHEATS] = {0x7D2D, 0x5739, 0x795C, 0x4251, 0x6134, 0x597D, 0x653B, 0x445B}; // }-, W9, y\, BQ, a4, Y~, e;, D[
    uint8_t pos = 0xff;
    uint8_t val;

    //Checksum ok?
    for (uint8_t x=0; x<8; ++x){
        if (checksum == chk[x]) pos = x;
    }

    //Check if cheat position is empty
    if (pos<MAX_CHEATS){
        EERead(CHEATS+pos, &val, 1);
        if (val == 0xff) {
            EEWrite(CHEATS+pos, &bit, 1);
            return 1;
        }
    }
    return 0;
}

//Decrypts data read from I2C EEPROM, max 255 bytes at a time
void DecryptData(uint16_t offset, uint8_t length, uint8_t type, uint8_t *data){
    //offset += L_BOILER;
    while(length){
        *data ^= xor_key[type][(uint8_t)(offset%KEY_LENGTH)];
        ++data;
        ++offset;
        --length;
    }
}

//Un-flips the extra "encrypted" data for answers
void UnflipData(uint8_t length, uint8_t *data){
    for (uint8_t x = 0; x<length; ++x){
        data[x] = (data[x]<<4)|(data[x]>>4);
        data[x] ^= 0x55; 
    }
}

//Game data: Read a number of bytes and decrypt
uint8_t ExtEERead(uint16_t offset, uint8_t length, uint8_t type, uint8_t *data){
    offset &=EXT_EE_MAX;
    uint8_t reg[2] = {(uint8_t)(offset>>8), (uint8_t)(offset&0xff)};
    uint8_t error = (I2C_read_bytes(EE_I2C_ADDR, &reg[0], 2, data, length));
    if (error) return error;
    DecryptData(offset, length, type, data);
    return 0;
}

//Empty all other strings
void ClearTxAfter(uint8_t nr){
    for (uint8_t x=(nr+1); x<TXLISTLEN; ++x) txStrLen[x]=0;
}

//Check if an input string starts with compare string
uint8_t StartsWith(uint8_t *data, char *compare){
    uint8_t x=0;
    while (data[x] && compare[x]){
        if(data[x] != compare[x]) return 0;
        ++x;
    }
    if (compare[x] > 0) return 0; //Not the entire string found
    return 1;
}

//Put the addresses of text that has to be sent in the address/length lists.
uint8_t PrepareSending(uint16_t address, uint16_t length, uint8_t type){
    uint8_t x=0;
    if (length){
        
        while (length>0xff) {
            txAddrList[x] = address;
            txStrLen[x] = 0xff;
            address += 0xff;
            length -= 0xff;
            ++x;
            if (x==(TXLISTLEN-1)) return 1;
        }
        txAddrList[x]=address;
        txStrLen[x]=length%0xff;
        if (length>0xff) return 1; 
        txTypeNow = type;
    } else {
        txAddrList[0] = 0;
    }

    //Get rid of old data from previous run
    ClearTxAfter(x);

    txAddrNow = 0;  //Start at first pointer, initiates sending in CheckSend
    return 0;
}

//
void SetResponse(uint8_t number, uint16_t address, uint16_t length, uint8_t type){
    reactStr[0][number]=address;
    reactStr[1][number]=length;
    reactStr[2][number]=type;
}

//
uint8_t SetStandardResponse(uint8_t custStrEnd){

    SetResponse(custStrEnd++, A_LF, 2, TEASER);
    SetResponse(custStrEnd++, A_LOCATION, L_LOCATION, TEASER);
    //SetResponse(custStrEnd++, A_SPACE, L_SPACE, TEASER);
    reactStr[0][custStrEnd++] = CURR_LOC;
    SetResponse(custStrEnd++, A_LF, 2, TEASER);
    SetResponse(custStrEnd++, A_PROMPT, L_PROMPT, TEASER);
    //SetResponse(custStrEnd++, A_SPACE, L_SPACE, TEASER);

    return custStrEnd;
}

//Get all the relevant data and string addresses of an object
void PopulateObject(uint16_t offset, object_model_t *object){
    uint16_t parStr;
    offset += L_BOILER;

    //Fill things with fixed distance to offset
    uint8_t data[OFF_STRINGFLDS];
    ExtEERead(offset, OFF_STRINGFLDS, GAME, &data[0]);
    object->addrNextObj = (data[OFF_NEXTOBJ]<<8|data[OFF_NEXTOBJ+1])+L_BOILER;
    object->addrNextLvl = (data[OFF_NEXTLVL]<<8|data[OFF_NEXTLVL+1])+L_BOILER;
    for (uint8_t x=0; x<BYTE_FIELDS_LEN; ++x){
        object->byteField[x]=data[x+OFF_BYTEFLDS];
    }

    //Find out where all of the strings begin and how long they are
    offset += OFF_STRINGFLDS;
    for(uint8_t x=0; x<STRING_FIELDS_LEN; ++x){
        
        //Determine length
        ExtEERead(offset, 3, GAME, &data[0]);
        parStr = (data[0]<<8|data[1]);
        offset += 2;
        if (x >= OPEN_ACL_MSG){
            object->lenStr[x]= parStr-1;
            object->addrStr[x]=offset+1;
            object->effect[x-OPEN_ACL_MSG] = data[2];
        } else {
            object->lenStr[x]= parStr;
            object->addrStr[x]=offset;
        }
        
        //Determine string start location for next field
        offset += parStr;
    }    
}

//Check if the entered letter corresponds with a name
uint8_t CheckLetter(uint16_t object, uint8_t letter){
    
    uint8_t found = 0;
    uint8_t data[32];

    object += L_BOILER;
    ExtEERead(object+OFF_STRINGFLDS, 2, GAME, &data[0]);
    uint8_t x = data[1]; //Assuming a name is not longer than 255 characters.

    while (x){
        uint8_t max;
        if (x>32) max = 32; else max = x;
        ExtEERead(object+OFF_STRINGFLDS+2, max, GAME, &data[0]);
        for (uint8_t y=0; y<max; ++y){
            if (found){
                if ((data[y]|0x20) == letter) return 1; else return 0;
            }
            if (data[y] == '[') found = 1;
        }
        object += max;
        x -= max;
    }
    return 0;
}

//Returns the child's address if the child is visible and the search letter matches
uint16_t FindChild(uint16_t parent, uint8_t letter, uint16_t start){
    
    uint16_t child = parent;
    uint8_t data[4];

    ExtEERead(child+L_BOILER, 4, GAME, &data[0]);
    parent = (data[0]<<8|data[1]);    //Next object on parent level
    child =  (data[2]<<8|data[3]);    //First object on child level

    //As long as the child is within the parent's range
    while (parent>child){

        //If child's address is higher than the start address, perform search for letter
        if (child>start){
            ExtEERead(child+OFF_BYTEFLDS+VISIBLE_ACL+L_BOILER, 1, GAME, &data[0]);
            if ((data[0] == 0)||(CheckState(data[0]))) {
                if ((letter == 0)||(CheckLetter(child, letter))) return child;
            }
        }

        //Not visible or name not right
        ExtEERead(child+L_BOILER, 2, GAME, &data[0]);
        child = (data[0]<<8|data[1]);    //Next object on child level
        
    } return 0;
}

//Allow only a(A) to z(Z) and 0 to 9 as input
uint8_t InpOkChk(uint8_t test){
    test |= 0x20;
    if ((test>='a')&&(test<='z')) return 1;
    if ((test>='0')&&(test<='9')) return 1;
    return 0;
}

//Cleans input of garbage and returns length of cleaned input "o->d2  !\0" becomes "od2\0"
uint8_t CleanInput(uint8_t *data){
    uint8_t cnt = 0;
    for (uint8_t x=0; data[x]!=0; ++x){
        data[cnt] = data[x];
        if (InpOkChk(data[x])) ++cnt;
    }
    data[cnt] = 0;
    return cnt;
}

//Send routine, optimized for low memory usage
uint8_t CheckSend(){
    static uint8_t txPart;
    uint8_t EEreadLength=0;

    //Play effects if available and not already playing
    if (effect == 0){
        effect = currObj.byteField[EFFECTS];
        auStart = ((effect&0xE0)>0);
    }

    //Check if more string(part)s have to be sent to the serial output if previous send operation is completed
    if ((txAddrNow < TXLISTLEN) && serTxDone){
        if (txStrLen[txAddrNow] == 0){
            txPart = 0;
            txAddrNow = TXLISTLEN;
        } else if (txPart < txStrLen[txAddrNow]){
            EEreadLength = txStrLen[txAddrNow]-txPart;
            if (EEreadLength>=TXLEN) EEreadLength = TXLEN-1;
            ExtEERead(txAddrList[txAddrNow]+txPart, EEreadLength, txTypeNow, &txBuffer[0]);
            txPart += EEreadLength;
            txBuffer[EEreadLength] = 0; //Add string terminator after piece to send to plug memory leak
            if ((txBuffer[0] == 0) && (EEreadLength)) txBuffer[0] = 0xDB; //Block character when data wrong.
            SerSend(&txBuffer[0]);
        } else {
            txPart = 0;
            ++txAddrNow;
        }
    } else if (serTxDone) return 0; //All is sent!
    return 1; //Still sending, do not change the data in the tx variables
}

//Send another part of the response and play the effect afterwards
uint8_t CheckResponse(){
    static uint8_t number = 0;
    if (responseList){
        --responseList;
        if (reactStr[0][number] == CURR_LOC) {
            PrepareSending(currObj.addrStr[NAME], currObj.lenStr[NAME], GAME);
        } else {
            PrepareSending(reactStr[0][number], reactStr[1][number], reactStr[2][number]);
        }
        ++number;

        if (responseList == 0) {
            effect = currObj.byteField[EFFECTS];
            RXCNT = 0;
            serRxDone = 0;
            number = 0;
            return 0;
        }
        return 1;
    }
    return 0;
}

//User input check (and validation)
uint8_t CheckInput(uint8_t *data){
    
    //Load game data after reboot
    if (currDepth == 0xff) {
        //Start at first location
        PopulateObject(route[0], &currObj);
        currDepth = 0;
    }

    if (serRxDone){

        //Special input requested from user by game
        if (specialInput[0]){
            specialPassed = 0;
            data[0] = 'a';
            
            //Normal code challenge
            if (StartsWith((uint8_t *)&serRx[0], &specialInput[0])) {
                specialPassed = 1;
                //specialInput[0] = 0;
                //data[1] = 0;

            //Special challenge 1
            } else if ((specialInput[0] == '1')&&(specialInput[2] == 0)) {
                uint8_t inputLen = CleanInput((uint8_t *)&serRx[0]);
                specialPassed = 2;
                data[1] = 0;

                if (inputLen >= 2) {
                    if ((serRx[0] == '1')||(serRx[0] == '2')||(serRx[0] == '3')||(serRx[0] == '4')) {
                        serRx[1] |= 0x20;
                        if ((serRx[1] == 'a')||(serRx[1] == 'e')||(serRx[1] == 'f')||(serRx[1] == 'w')) {
                            data[1] = specialInput[1]+0x11;
                            data[2] = serRx[0];
                            data[3] = serRx[1];
                            data[4] = 0;
                        }
                    }
                }
            }
            //Wrong answer
            //} else {
                //specialInput[0] = 0;
                //data[1] = 0;
            //}
        
        //Normal input
        } else {

            //Change status bit
            if ((serRx[0] == '#')&&(RXCNT == 6)){
                uint8_t bitNr = 0;

                for (uint8_t x=1; x<4; ++x) {
                    serRx[x] -= '0';
                    bitNr *= 10;
                    if (serRx[1] > 2) break;
                    if (serRx[x] < 10) {
                        bitNr += serRx[x];
                        continue;
                    }
                    bitNr = 0;
                    break;
                }
                if ((bitNr)&&(bitNr!=128)) {
                    if (Cheat(0xff-bitNr, serRx[4]<<8|serRx[5])) UpdateState(bitNr);
                }
                responseList = SetStandardResponse(0);
                return 1;            
            }
            
            //Read up to the \0 character and convert to lower case.
            for (uint8_t x=0; x<RXLEN; ++x){
                if ((serRx[x]<'A')||(serRx[x]>'Z')) data[x]=serRx[x]; else data[x]=serRx[x]|0x20;
                if (serRx[x] == 0) {
                    data[x] = 0;
                    break;
                }
            }

            //No text
            if (serRx[0] == 0){
                data[0] = 0;
                RXCNT = 0;
                serRxDone = 0;
                return 1;
            }

            //Help text
            if ((data[0] == '?')||(data[0] == 'h')){
                SetResponse(0, A_LF, 4, TEASER);
                SetResponse(1, A_HELP, L_HELP, TEASER);
                responseList = SetStandardResponse(2);
                return 1;
            }        
        
            //Alphabet text
            if (data[0] == 'a'){
                SetResponse(0, A_LF, 4, TEASER);
                SetResponse(1, A_ALPHABET, L_ALPHABET, TEASER);
                responseList = SetStandardResponse(2);
                return 1;
            }

            //Whoami text
            if (data[0] == 'w'){
                SetResponse(0, A_LF, 4, TEASER);
                SetResponse(1, A_HELLO, L_HELLO, TEASER);
                if (whoami == 1) {
                    SetResponse(2, A_ANUBIS, L_ANUBIS, TEASER);
                } else if (whoami == 2) {
                    SetResponse(2, A_BES, L_BES, TEASER);
                } else if (whoami == 3) {
                    SetResponse(2, A_KHONSU, L_KHONSU, TEASER);
                } else if (whoami == 4) {
                    SetResponse(2, A_THOTH, L_THOTH, TEASER);
                } else {
                    SetResponse(2, A_ERROR, L_ERROR, TEASER);
                }
                SetResponse(3, A_PLEASED, L_PLEASED, TEASER);
                responseList = SetStandardResponse(4);
                return 1;
            }

            //Quit text
            if (data[0] == 'q'){
                SetResponse(0, A_LF, 4, TEASER);
                SetResponse(1, A_QUIT, L_QUIT, TEASER);
                responseList = SetStandardResponse(2);
                return 1;
            }

            //Fake cheat = reset badge!
            if (StartsWith(&data[0], "iddqd")){
            
                //Reset game data by wiping the UUID bits
                for (uint8_t x=0; x<4; ++x){
                    WriteStatusBit(110+x, 0);
                }
                SaveGameState();

                uint8_t cheat[] = "Cheater! Contact badge team... ";
                SerSpeed(60);
                while(1){
                    if (serTxDone) SerSend(&cheat[0]);
                }
            }
            
            //Cheat reset (Quote from: Donald E. Westlake)
            if (StartsWith(&data[0], "the trouble with real life is, there's no reset button")){
                
                //Reset cheat data by resetting the EEPROM bytes
                uint8_t empty=0xff;
                for (uint8_t x=0; x<MAX_CHEATS; ++x){
                    EEWrite(CHEATS+x, &empty, 1);
                }
                //Reset game data by wiping the UUID bits
                for (uint8_t x=0; x<4; ++x){
                    WriteStatusBit(110+x, 0);
                }
                
                SaveGameState();

                uint8_t cheat[] = "Reset please! ";
                SerSpeed(60);
                while(1){
                    if (serTxDone) SerSend(&cheat[0]);
                }
            }
            if (StartsWith(&data[0], "cheat")){
                int8_t n;
                uint8_t bit, digit[3];

                for (uint8_t x=0; x<MAX_CHEATS; ++x){
                    
                    //Set up sending out number
                    EERead(CHEATS+x, &bit, 1);
                    bit = 0xff-bit;
                    for (n=2; n>=0; --n) {
                        digit[n] = bit % 10;
                        bit /= 10;
                    }

                    for (n=0; n<3; ++n) {
                        SetResponse(x*4+n, A_DIGITS+digit[n], 1, TEASER);
                    }                
                    SetResponse(x*4+3, A_COMMA, L_COMMA, TEASER);
                }
                SetResponse((4*MAX_CHEATS)-1, A_LF, 4, TEASER);
                responseList = 32;
                return 1;
            }
        } 
        //Data received, but not any of the commands above
        return 0;
    }

    //Serial input not available yet
    return 1;
}

//The game logic!
uint8_t ProcessInput(uint8_t *data){
    static object_model_t actObj1, actObj2;
    uint8_t elements = 1;

    CleanInput(&data[0]);
    uint8_t inputLen = CleanInput(&data[0]);

    if (inputLen) {

        //eXit to previous location
        if (data[0] == 'x'){

            //Standing in the Lobby?
            if ((route[currDepth] == 0)||(currDepth == 0)){
                SetResponse(elements++, A_NOTPOSSIBLE, L_NOTPOSSIBLE, TEASER);
            
            //Is there a way to go back?
            } else if (CheckState(currObj.byteField[OPEN_ACL])){
                --currDepth;
                PopulateObject(route[currDepth], &currObj);
                effect = currObj.byteField[EFFECTS];
            
            //No way out, print denied message of location
            } else {
                SetResponse(elements++, currObj.addrStr[OPEN_ACL_MSG], currObj.lenStr[OPEN_ACL_MSG], GAME);
                effect = currObj.effect[0];               
            }   
        
        //Enter locations or Open objects    
        } else if ((data[0] == 'e')||(data[0] == 'o')) {
                
            //Not possible, too many/little characters
            if (inputLen != 2){
                SetResponse(elements++, A_NOTPOSSIBLE, L_NOTPOSSIBLE, TEASER);
            } else {
                uint8_t canDo = 0;
                route[currDepth+1] = FindChild(route[currDepth], data[1], 0);
                    
                //Child found?
                if (route[currDepth+1]) {
                    PopulateObject(route[currDepth+1], &actObj1);
                    canDo = 1;
                //No child, maybe a step back, letter ok?
                } else if (currDepth) {
                    if (CheckLetter(route[currDepth-1], data[1])) {
                        PopulateObject(route[currDepth-1], &actObj1);
                        canDo = 1; 
                    }
                }

                //The candidate is found! Let's check if the action is legit
                if (canDo) {
                    if ((data[0] == 'e') && ((actObj1.byteField[ACTION_MASK]&ENTER)==0)) {
                        SetResponse(elements++, A_CANTENTER, L_CANTENTER, TEASER);
                    } else if ((data[0] == 'o') && ((actObj1.byteField[ACTION_MASK]&OPEN)==0)) {
                        SetResponse(elements++, A_CANTOPEN, L_CANTOPEN, TEASER);
                    
                    //Action legit, permission granted?
                    } else if (CheckState(actObj1.byteField[OPEN_ACL])) {
                            
                        //Yes! Check if we must move forward or backwards.
                        if (route[currDepth+1]) ++currDepth; else --currDepth;
                        PopulateObject(route[currDepth], &currObj);
                        SetResponse(elements++, currObj.addrStr[DESC], currObj.lenStr[DESC],GAME);
                        SetResponse(elements++, A_LF, 2, TEASER);
                        effect = currObj.byteField[EFFECTS];
                                                  
                    //Not granted!
                    } else {
                        route[currDepth+1] = 0;
                        SetResponse(elements++, actObj1.addrStr[OPEN_ACL_MSG], actObj1.lenStr[OPEN_ACL_MSG], GAME);
                        effect = actObj1.effect[0];              
                    }

                //No candidate
                } else {
                    SetResponse(elements++, A_DONTSEE, L_DONTSEE, TEASER);                
                }
            }

        //Look around or at objects
        } else if (data[0] == 'l') {
            if (inputLen == 1) {

                //Show info about this area first
                SetResponse(elements++, currObj.addrStr[DESC], currObj.lenStr[DESC],GAME);
                SetResponse(elements++, A_LF, 2, TEASER);
                SetResponse(elements++, A_LOOK, L_LOOK, TEASER);

                //Check the visible children first
                route[currDepth+1] = 0;
                do{
                    route[currDepth+1] = FindChild(route[currDepth], 0, route[currDepth+1]);
                    if (route[currDepth+1]) {
                        if ((route[currDepth+1] != inventory[0])&&(route[currDepth+1] != inventory[1])) {
                            PopulateObject(route[currDepth+1], &actObj1);
                            SetResponse(elements++, actObj1.addrStr[NAME], actObj1.lenStr[NAME],GAME);
                            SetResponse(elements++, A_COMMA, L_COMMA, TEASER);
                        }
                    }
                }while (route[currDepth+1]);

                //Look back if not on level 0
                if (currDepth) {
                    PopulateObject(route[currDepth-1], &actObj1);
                    SetResponse(elements++, actObj1.addrStr[NAME], actObj1.lenStr[NAME],GAME);
                } else elements-=1;

            } else {
                route[currDepth+1] = FindChild(route[currDepth], data[1], 0);
                if (route[currDepth+1]) {
                    PopulateObject(route[currDepth+1], &actObj1);
//TODO: CHECKSTATE -> A_CANTLOOK item
                    SetResponse(elements++, actObj1.addrStr[DESC], actObj1.lenStr[DESC],GAME);   
                } else if (currDepth) {
                    if (CheckLetter(route[currDepth-1], data[1])) {
                        PopulateObject(route[currDepth-1], &actObj1);
                        SetResponse(elements++, actObj1.addrStr[DESC], actObj1.lenStr[DESC],GAME);
                    }
                } else {
                    SetResponse(elements++, A_DONTSEE, L_DONTSEE, TEASER);
                }
            }
        
        //Pick up an object
        } else if (data[0] == 'p') {
            if (inventory[0]&&inventory[1]) {
                SetResponse(elements++, A_CARRYTWO, L_CARRYTWO, TEASER);
            } else if (inputLen != 2) {
                SetResponse(elements++, A_NOTPOSSIBLE, L_NOTPOSSIBLE, TEASER);
            } else {
                route[currDepth+1] = FindChild(route[currDepth], data[1], 0);
                if (route[currDepth+1]) {
                    if ((route[currDepth+1] == inventory[0])||(route[currDepth+1] == inventory[1])) {
                        SetResponse(elements++, A_ALREADYCARRYING, L_ALREADYCARRYING, TEASER);
                        route[currDepth+1] = 0;
                    } else {
                        //Put item in the inventory if possible
                        PopulateObject(route[currDepth+1], &actObj1);  
                        if (CheckState(actObj1.byteField[ACTION_ACL])) {
                            if (actObj1.byteField[ITEM_NR]) {
                                if (inventory[0]) {
                                    inventory[1] = route[currDepth+1];
                                } else {
                                    inventory[0] = route[currDepth+1];
                                }
                                SetResponse(elements++, A_NOWCARRING, L_NOWCARRING, TEASER);
                                SetResponse(elements++, actObj1.addrStr[NAME], actObj1.lenStr[NAME], GAME);
                            } else {
                                SetResponse(elements++, A_NOTPOSSIBLE, L_NOTPOSSIBLE, TEASER);
                            }
                        } else {
                            SetResponse(elements++, actObj1.addrStr[ACTION_ACL_MSG], actObj1.lenStr[ACTION_ACL_MSG], GAME);
                            effect = actObj1.effect[1];
                        }
                    }
                } else SetResponse(elements++, A_NOSUCHOBJECT, L_NOSUCHOBJECT, TEASER);
            }
        } else

        //Drop item if in inventory
        if (data[0] == 'd') {
            if ((inventory[0] == 0)&&(inventory[1] == 0)){
                SetResponse(elements++, A_EMPTYHANDS, L_EMPTYHANDS, TEASER);
            } else if (inputLen != 2) {
                SetResponse(elements++, A_NOTPOSSIBLE, L_NOTPOSSIBLE, TEASER);
            } else {
                for (uint8_t x=0; x<2; ++x) {
                    if (inventory[x]) {
                        if (CheckLetter(inventory[x], data[1])) {
                            PopulateObject(inventory[x], &actObj1);
                            SetResponse(elements++, A_DROPPING, L_DROPPING, TEASER);
                            SetResponse(elements++, actObj1.addrStr[NAME], actObj1.lenStr[NAME], GAME);
                            SetResponse(elements++, A_LF, 2, TEASER);
                            SetResponse(elements++, A_RETURNING, L_RETURNING, TEASER);
                            inventory[x] = 0;
                            break;
                        }
                    }
                    if (x) SetResponse(elements++, A_NOTCARRYING, L_NOTCARRYING, TEASER);
                }
            }

        //Inventory list
        } else if (data[0] == 'i') {
            if ((inventory[0] == 0)&&(inventory[1] == 0)){
                SetResponse(elements++, A_EMPTYHANDS, L_EMPTYHANDS, TEASER);
            } else {
                SetResponse(elements++, A_NOWCARRING, L_NOWCARRING, TEASER);
                //SetResponse(elements++, A_SPACE, L_SPACE, TEASER);
                
                for (uint8_t x=0; x<2; ++x) {
                    if (inventory[x]) {
                        PopulateObject(inventory[x], &actObj1);
                        SetResponse(elements++, actObj1.addrStr[NAME], actObj1.lenStr[NAME], GAME);
                        SetResponse(elements++, A_COMMA, L_COMMA, TEASER);
                        //SetResponse(elements++, A_SPACE, L_SPACE, TEASER);
                    }
                }
                elements -= 1;
            }            
        
        //Talk, use, give, read
        } else if ((data[0] == 't')||(data[0] == 'u')||(data[0] == 'g')||(data[0] == 'r')) {
            if ((inputLen<2)||(inputLen>3)) {
                SetResponse(elements++, A_NOTPOSSIBLE, L_NOTPOSSIBLE, TEASER);                
            
            //Check for visible person or object first
            } else {
                route[currDepth+1] = FindChild(route[currDepth], data[inputLen-1], 0);
                if (route[currDepth+1]) {
                    
                    //Give item to person / use item on object 
                    if ((inputLen == 3)&&((data[0] == 'u')||(data[0] == 'g'))) {
                        for (uint8_t x=0; x<2; x++) {
                            if (inventory[x]) { 
                                if (CheckLetter(inventory[x], data[1])) {
                                    PopulateObject(inventory[x], &actObj2);
                                    x = 2;
                                }
                            }
                            if (x == 1) { 
                                SetResponse(elements++, A_NOTCARRYING, L_NOTCARRYING, TEASER);
                                data[0] = 0;
                            }
                        }

                        //Both the item and person/object are found, check if action is legit
                        if (data[0]){
                            PopulateObject(route[currDepth+1], &actObj1);

                            //Special game
                            if (actObj1.lenStr[ACTION_STR1] == 1) {
                                ExtEERead(actObj1.addrStr[ACTION_STR1], 1, GAME, &data[2]);
                                if (data[2] == '1') {
                                    uint8_t item = actObj2.byteField[ITEM_NR];
                                    if ((item < 31)||(item > 34)) {
                                        SetResponse(elements++, A_CANTUSE, L_CANTUSE, TEASER);
                                    } else {
                                        SetResponse(elements++, A_PRIEST, L_PRIEST, TEASER);
                                        SetResponse(elements++, A_LF, 2, TEASER);
                                        SetResponse(elements++, A_RESPONSE, L_RESPONSE, TEASER);
                                        specialInput[0] = '1';
                                        specialInput[1] = item;
                                        specialInput[2] = 0;
                                    }

                                //There should not be another special
                                } else {
                                    SetResponse(elements++, A_ERROR, L_ERROR, TEASER);
                                }

                            //Normal "use ... on ..." or "give ... to ..." action
                            } else if (actObj1.byteField[ACTION_ITEM] == actObj2.byteField[ITEM_NR]) {
                                SetResponse(elements++, actObj1.addrStr[ACTION_MSG], actObj1.lenStr[ACTION_MSG], GAME);
                                effect = actObj1.effect[2];
                                UpdateState(actObj1.byteField[ACTION_STATE]);
                                
                            } else {
                                if (data[0] == 'u') {
                                    SetResponse(elements++, A_CANTUSE, L_CANTUSE, TEASER);
                                } else if (data[0] == 'g') {
                                    SetResponse(elements++, A_CANTGIVE, L_CANTGIVE, TEASER);                                    
                                }
                            }
                        }
                    
                    //Talk to person, read or use object (without item)                          
                    } else {
                        PopulateObject(route[currDepth+1], &actObj1);
                        if ((data[0] == 't')&&((actObj1.byteField[ACTION_MASK]&TALK) == 0)) {
                            SetResponse(elements++, A_WHYTALK, L_WHYTALK, TEASER);
                            SetResponse(elements++, actObj1.addrStr[NAME], actObj1.lenStr[NAME], GAME);
                        } else if ((data[0] == 'u')&&((actObj1.byteField[ACTION_MASK]&USE) == 0)) {
                            SetResponse(elements++, A_CANTUSE, L_CANTUSE, TEASER);
                        } else if ((data[0] == 'r')&&((actObj1.byteField[ACTION_MASK]&READ) == 0)) {
                            SetResponse(elements++, A_CANTREAD, L_CANTREAD, TEASER);
                        } else if (data[0] == 'g'){ 
                            SetResponse(elements++, A_NOTPOSSIBLE, L_NOTPOSSIBLE, TEASER);  
                        } else {

                            //Special game, not enough characters
                            if (actObj1.lenStr[ACTION_STR1] == 1) {
                                ExtEERead(actObj1.addrStr[ACTION_STR1], 1, GAME, &data[2]);
                                if (data[2] == '1') {
                                    SetResponse(elements++, A_PLEASEOFFER, L_PLEASEOFFER, TEASER);
                                }

                            //General request
                            } else if (actObj1.lenStr[ACTION_STR1]) {
                                SetResponse(elements++, actObj1.addrStr[ACTION_STR1], actObj1.lenStr[ACTION_STR1], GAME);
                                SetResponse(elements++, A_LF, 2, TEASER);
                                SetResponse(elements++, A_RESPONSE, L_RESPONSE, TEASER);
                                if (actObj1.lenStr[ACTION_STR2]>(INP_LENGTH-1)) actObj1.lenStr[ACTION_STR2] = (INP_LENGTH-1);
                                ExtEERead(actObj1.addrStr[ACTION_STR2], actObj1.lenStr[ACTION_STR2], GAME, (uint8_t *)&specialInput[0]);
                                UnflipData(actObj1.lenStr[ACTION_STR2], (uint8_t *)&specialInput[0]);
                                specialInput[actObj1.lenStr[ACTION_STR2]] = 0;
                            
                            //Normal action    
                            } else if (CheckState(actObj1.byteField[ACTION_ACL])){
                                SetResponse(elements++, actObj1.addrStr[ACTION_MSG], actObj1.lenStr[ACTION_MSG], GAME);
                                effect = actObj1.effect[2];
                                UpdateState(actObj1.byteField[ACTION_STATE]);
                            } else {
                                SetResponse(elements++, actObj1.addrStr[ACTION_ACL_MSG], actObj1.lenStr[ACTION_ACL_MSG], GAME);
                                effect = actObj1.effect[1];
                            }
                        }
                    }

                //Person or object not found
                } else {
                    if ((data[0] == 't')||(data[0] == 'g')){
                        SetResponse(elements++, A_NOSUCHPERSON, L_NOSUCHPERSON, TEASER);
                    } else {
                        SetResponse(elements++, A_NOSUCHOBJECT, L_NOSUCHOBJECT, TEASER);
                    }
                }
            }
        
        //Special answer given
        } else if (data[0] == 'a'){
            
            //Priest offerings
            if (specialPassed >= 2) {
                if (data[1] > 0) {

                    uint8_t  digit[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
                    uint32_t answer = 0;
                    uint8_t  n = 0;

                    /* CALCULATION                    
                        data[x]: x=1->offering x=2->kneelings x=3->element whoami->person
                    
                        answer = ((offering  & 2) << 19) + ((offering  & 1) << 8) + \
                        ((element   & 2) << 15) + ((element   & 1) << 4) + \
                        ((kneelings & 2) << 11) + ((kneelings & 1))
                        answer = answer << (3-person)
                    */
                    data[1]-='1';
                    data[2]-='1';
                    if (data[3] == 'a') data[3] = 1;
                    else if (data[3] == 'e') data[3] = 0;
                    else if (data[3] == 'f') data[3] = 3;
                    else data[3] = 2;

                    if (data[1] & 2) answer += (1UL << 20);
                    if (data[1] & 1) answer += (1 << 8);
                    if (data[3] & 2) answer += (1UL << 16);
                    if (data[3] & 1) answer += (1 << 4);
                    if (data[2] & 2) answer += (1 << 12);
                    if (data[2] & 1) answer += 1;
                    answer <<= (4 - whoami);            

                    SetResponse(elements++, A_YOURPART, L_YOURPART, TEASER);
                    
                    //Set up sending out number
                    for (n=9; n>=0; --n) {
                        digit[n] = answer % 10;
                        answer /= 10;
                        if (answer == 0) break;
                    }
                    for (; n<10; ++n) {
                        SetResponse(elements++, A_DIGITS+digit[n], 1, TEASER);
                        UpdateState(actObj1.byteField[ACTION_STATE]);
                    }

                } else {
                    SetResponse(elements++, A_BADOFFERING, L_BADOFFERING, TEASER);
                }
            
            //Other questions    
            } else if (specialPassed == 1) {
                PopulateObject(route[currDepth+1], &actObj1);
                if (CheckState(actObj1.byteField[ACTION_ACL])){
                    SetResponse(elements++, actObj1.addrStr[ACTION_MSG], actObj1.lenStr[ACTION_MSG], GAME);
                    effect = actObj1.effect[2];
                    UpdateState(actObj1.byteField[ACTION_STATE]);
                } else {
                    SetResponse(elements++, actObj1.addrStr[ACTION_ACL_MSG], actObj1.lenStr[ACTION_ACL_MSG], GAME);
                    effect = actObj1.effect[1];
                }

            } else {
                PopulateObject(route[currDepth+1], &actObj1);
                SetResponse(elements++, A_INCORRECT, L_INCORRECT, TEASER);
                PunishmentTime = getClock();
            }
            specialInput[0] = 0;

        //Faulty input
        } else {
               
        }
            
        //Input handled
        SetResponse(0, A_LF, 2, TEASER);
        if (specialInput[0]) responseList = elements; else responseList = SetStandardResponse(elements);

    } else {
        SetResponse(0, A_LF, 2, TEASER);
        responseList = SetStandardResponse(1);
    }

    data[0] = 0;
    serRxDone = 0;
    RXCNT = 0;    
    return 0;
}


//MAIN GAME STRUCTURE
uint8_t TextAdventure(){
    static uint8_t serInput[RXLEN];
    uint16_t PunishmentCount;
    
    /* Count down PunishmentTime on WING LEDs */
    if (PunishmentTime != 0) {
        PunishmentCount = getClock();
        if (PunishmentCount < PunishmentTime)
            PunishmentCount += 256 * 60;
        PunishmentCount -= PunishmentTime;
        if (PunishmentCount > 10) {
            PunishmentTime = 0;
            PunishmentCount = 10;
        }
        if (gameNow == TEXT ) {
            WingBar(10-PunishmentCount,5-PunishmentCount);
        }
    }

    //Still sending data to serial?
    if (CheckSend()) return 1;

    //Not sending? Process next response part to send, if any.
    if (CheckResponse()) return 1;        

    if (PunishmentTime == 0) {
        //No responses to send, check if there is user input.
        if (CheckInput(&serInput[0])) return 2; 

        //Input found, process and save (changes only)
        ProcessInput(&serInput[0]);
    }

    return 0;
}
