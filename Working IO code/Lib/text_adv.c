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

//Serial Tx string glue and send stuff
#define  TXLISTLEN  8
uint16_t txAddrList[TXLISTLEN]  = {5, 0};   //List of external EEPROM addresses of first element of strings to be glued together
uint8_t txStrLen[TXLISTLEN]     = {5, 0};   //List of lengths of strings to be glued together
uint8_t txAddrNow = 0;                      //Number of the string that currently is being sent
uint8_t txBuffer[TXLEN];                    //Buffer for string data

//Decrypts data read from I2C EEPROM, max 255 bytes at a time
void DecryptData(uint16_t offset, uint8_t length, uint8_t type, uint8_t *data){
    while(length){
        offset-=32;
        *data ^= xor_key[(uint8_t)(offset%KEY_LENGTH)][type];
        ++data;
        --length;
    }
}

//Game data: Read a number of bytes and decrypt
uint8_t ExtEERead(uint16_t offset, uint8_t length, uint8_t type, uint8_t *data){
    offset = (offset+32)&0x7fff;
    uint8_t reg[2] = {(uint8_t)(offset>>8), (uint8_t)(offset&0xff)};
    uint8_t error = (I2C_read_bytes(EE_I2C_ADDR, &reg[0], 2, data, length));
    if (error) return error;
    DecryptData(offset, length, type, data);
    return 0;
}

//Reads a pointer from I2C EEPROM
uint16_t ReadPtr (uint16_t offset){
    uint8_t data[2] = {0,0};
    if (ExtEERead(offset, 2, GAME, &data[0])) return 0xffff; else return (data[0]<<8|data[1]);
}

//Send routine, optimized for low memory usage
uint8_t CheckSend(){
    static uint8_t txPart;
    uint8_t EEreadLength=0;

    //Check if more string(part)s have to be sent to the serial output if previous send operation is completed
    if ((txAddrNow < TXLISTLEN) && serTxDone){
        if (txPart < txStrLen[txAddrNow]){
            EEreadLength = (txStrLen[txAddrNow]-txPart)%(TXLEN-1);
            ExtEERead(txAddrList[txAddrNow]+txPart, EEreadLength, GAME, &txBuffer[0]);
            txPart += EEreadLength;
        } else {
            txPart = 0;
            ++txAddrNow;
        }
        txBuffer[EEreadLength] = 0; //Add string terminator after piece to send to plug memory leak
        SerSend(&txBuffer[0]);
    } else if (serTxDone) return 0; //All is sent!
    return 1; //Still sending, do not change the data in the tx variables
}

//User input check (and validation?)
uint8_t CheckInput(){
    return 0;
}

//The game logic!
void ProcessInput(){
    
}


// Main game loop
uint8_t TextAdventure(){
    if (CheckSend()) return 1;          //Still sending data to serial, return 1
    if (CheckInput() == 0) return 2;    //No input to process, return 2
    ProcessInput();
    return 0;
}