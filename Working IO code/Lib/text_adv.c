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

object_model_t currObj;
object_model_t actObj;

uint16_t reactStr[3][8] = {{0},{0},{0}};
uint8_t actionList = 0;


//Load saved data from internal EEPROM
uint8_t LoadGameState(){
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

//Game data: Read a number of bytes and decrypt
uint8_t ExtEERead(uint16_t offset, uint8_t length, uint8_t type, uint8_t *data){
    offset &=EXT_EE_MAX;
    uint8_t reg[2] = {(uint8_t)(offset>>8), (uint8_t)(offset&0xff)};
    uint8_t error = (I2C_read_bytes(EE_I2C_ADDR, &reg[0], 2, data, length));
    if (error) return error;
    DecryptData(offset, length, type, data);
    return 0;
}

//Reads an address from I2C EEPROM
uint16_t ReadAddr (uint16_t offset, uint8_t type){
    uint8_t data[2] = {0,0};
    if (ExtEERead(offset, 2, type, &data[0])) return 0xffff; else return (data[0]<<8|data[1]);
}

//Empty all other string
void ClearTxAfter(uint8_t nr){
    for (uint8_t x=(nr+1); x<TXLISTLEN; ++x) txStrLen[x]=0;
}

//Check if a string starts with compare value
uint8_t StartsWith(uint8_t *data, char *compare){
    uint8_t x=0;
    while (data[x] && compare[x]){
        if(data[x] != compare[x]) return 0;
        ++x;
        if ((x>=(sizeof(*compare)-1))||(x>=(sizeof(*data)))) break;
    }
    return 1;
}

//Put the addresses
uint8_t PrepareSending(uint16_t address, uint16_t length, uint8_t type, uint8_t prompt){
    uint8_t x=0;
    while (length>255) {
        txAddrList[x] = address;
        txStrLen[x] = 255;
        address += 255;
        length -= 255;
        ++x;
        if (x==(TXLISTLEN-1));
    }
    txAddrList[x]=address;
    txStrLen[x]=length%255;
    if (length>255) return 1; 
    txTypeNow = type;
    ClearTxAfter(x);
    sendPrompt = prompt;
    txAddrNow = 0;  //Start at first pointer, initiates sending in CheckSend
    return 0;
}

//Get all the relevant data and string addresses of an object
void PopulateObject(uint16_t offset, object_model_t *object){
    uint16_t addrStart;
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
    object->addrStr[0]=offset+1;
    for(uint8_t x=1; x<STRING_FIELDS_LEN; ++x){
        addrStart = offset;
        do {
            ExtEERead(offset, 1, GAME, &data[0]);
            offset += (uint16_t)(data[0])+1;
            if (offset>EXT_EE_MAX) break;
        } while (data[0]==255);
        object->lenStr[x-1]=(offset-addrStart-1)&EXT_EE_MAX;
        object->addrStr[x]=offset&EXT_EE_MAX;
    }
    object->lenStr[STRING_FIELDS_LEN-1]=(offset-addrStart)&EXT_EE_MAX;
}

//Sends numCR LFs and a literal, total
void SendLiteral(uint16_t address, uint16_t length, uint8_t numCR){
    numCR %= (TXLEN-1);
    length %= (TXLEN-numCR-1);
    for (uint8_t x=0; x<numCR; ++x) txBuffer[x]='\n';
    if (length) ExtEERead(address, length, TEASER, &txBuffer[numCR]);
    txBuffer[numCR+length]='\0';
    SerSend(&txBuffer[0]);
}

//Send routine, optimized for low memory usage
uint8_t CheckSend(){
    static uint8_t txPart;
    uint8_t EEreadLength=0;

    //Check if more string(part)s have to be sent to the serial output if previous send operation is completed
    if ((txAddrNow < TXLISTLEN) && serTxDone){
        if (txPart < txStrLen[txAddrNow]){
            EEreadLength = txStrLen[txAddrNow]-txPart;
            if (EEreadLength>=TXLEN) EEreadLength = TXLEN-1;
            ExtEERead(txAddrList[txAddrNow]+txPart, EEreadLength, txTypeNow, &txBuffer[0]);
            txPart += EEreadLength;
            txBuffer[EEreadLength] = 0; //Add string terminator after piece to send to plug memory leak
            SerSend(&txBuffer[0]);
        } else {
            txPart = 0;
            ++txAddrNow;
        }
    
    //Check if prompt needs to be sent afterward or something else
    } else if (serTxDone&&sendPrompt) {
        if (sendPrompt == PROMPT){
            SendLiteral(A_PROMPT, L_PROMPT, 2);
        } else if (sendPrompt == SPACE){
            SendLiteral(A_SPACE, L_SPACE, 0);
        } else if (sendPrompt == CR_2){
            SendLiteral(0, 0, 2);
        }
        sendPrompt=0;
    } else if (serTxDone) return 0; //All is sent!
    return 1; //Still sending, do not change the data in the tx variables
}

//User input check (and validation)
uint8_t CheckInput(uint8_t *data){
    if (serRxDone){
        //Read up to the \0 character and convert to lower case.
        for (uint8_t x=0; serRx[x]!=0; ++x){
            if ((serRx[x]<'A')||(serRx[x]>'Z')) data[x]=serRx[x]; else data[x]=serRx[x]|0x20;
        }

        //Reset serial input pointer and accept new input
        RXCNT = 0;
        serRxDone = 0;        
        
        //Help text
        if ((data[0] == '?')||(data[0] == 'h')){
            //Put help pointers/lengths in tx list
            PrepareSending(A_HELP, L_HELP, TEASER, PROMPT);
            return 1;
        }

        //Quit text
        if (data[0] == 'q'){
            //Put quit pointers/lengths in tx list
            PrepareSending(A_QUIT, L_QUIT, TEASER, PROMPT);
            return 1;
        }

        //Cheat = reset badge!
        if (StartsWith(&data[0], "iddqd")){
            //Reset game(s) data (TODO)

            uint8_t cheat[] = "Cheater! ";
            SerSpeed(255);
            while(1){
                if (serTxDone) SerSend(&cheat[0]);
            }
        }

        //Data received, but not any of the commands above
        return 0;
    }

    //Serial input not available yet
    return 1;
}

//The game logic!
void ProcessInput(uint8_t *data){
//    enum {NAME, DESC, ACTION_STR1, ACTION_STR2, OPEN_ACL_MSG, ACTION_ACL_MSG, ACTION_MSG};
    static uint8_t toSend = 0;

    PopulateObject(0, &currObj);
    
    if (actionList){
        PrepareSending(reactStr[0][toSend], reactStr[1][toSend], GAME, reactStr[2][toSend]);
        ++toSend;
        --actionList;
    } else {
        toSend = 0;

        if (data[0] == 'x'){
            reactStr[0][0]=currObj.addrStr[NAME];
            reactStr[1][0]=currObj.lenStr[NAME];
            reactStr[2][0]=PROMPT;
            actionList = 1;
        
        } else
        if ((data[0] == 'e')||(data[0] == 'o')){
            PrepareSending(currObj.addrStr[DESC],currObj.lenStr[DESC], GAME, PROMPT);
        
        } else
        if (data[0] == 'l'){
            PrepareSending(currObj.addrStr[ACTION_STR1],currObj.lenStr[ACTION_STR1], GAME, PROMPT);
        } else
        if (data[0] == 'p'){
            PrepareSending(currObj.addrStr[ACTION_STR2],currObj.lenStr[ACTION_STR2], GAME, PROMPT);
        
        } else
        if (data[0] == 'd'){
            PrepareSending(currObj.addrStr[OPEN_ACL_MSG],currObj.lenStr[OPEN_ACL_MSG], GAME, PROMPT);
        
        } else
        if (data[0] == 'i'){
            PrepareSending(currObj.addrStr[ACTION_ACL_MSG],currObj.lenStr[ACTION_ACL_MSG], GAME, PROMPT);
        
        } else
        if ((data[0] == 't')||(data[0] == 'u')){
            PrepareSending(currObj.addrStr[ACTION_MSG],currObj.lenStr[ACTION_MSG], GAME, PROMPT);
        
        } 
    }
}


// Main game loop
uint8_t TextAdventure(){
    static uint8_t serInput[RXLEN];
    if (CheckSend()) return 1;                      //Still sending data to serial, return 1
    if (CheckInput(&serInput[0])) return 2;         //No valid input to process, return 2
    ProcessInput(&serInput[0]);                     //Check which response to generate and fill tx arrays
    //CheckChanges();                               //Check if things are changed, write to EEPROM
    return 0;
}