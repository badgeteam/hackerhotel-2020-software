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
        object->addrStr[x]=(offset-1)&EXT_EE_MAX;
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

//Update game state: num -> vBBBBbbb v=value(0 is set!), BBBB=Byte number, bbb=bit number
void UpdateState(uint8_t num){
    uint8_t clearBit = num & STATUS_BITS;
    num &= (STATUS_BITS-1);
    if (clearBit) {
        WriteStatusBit(num, 0);
    } else {
        WriteStatusBit(num, 1);
    }
}

//Returns the state of the bit on position "num"
uint8_t GetState(uint8_t num){
    if ((num)&&(num<STATUS_BITS)){
        return ReadStatusBit(num);
    }
    return 1;
}

//Checks if state of bit BBBBbbb matches with v (inverted) bit
uint8_t CheckState(uint8_t num){
    return (GetState(num&(STATUS_BITS-1)) == (1-(num&STATUS_BITS))); 
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

        //Help text
        if ((data[0] == '?')||(data[0] == 'h')){
            //Put help pointers/lengths in tx list
            PrepareSending(A_HELP, L_HELP, TEASER, PROMPT);
            RXCNT = 0;
            serRxDone = 0;
            return 1;
        }

        //Quit text
        if (data[0] == 'q'){
            //Put quit pointers/lengths in tx list
            PrepareSending(A_QUIT, L_QUIT, TEASER, PROMPT);
            RXCNT = 0;
            serRxDone = 0;
            return 1;
        }

        //Cheat = reset badge!
        if (StartsWith(&data[0], "iddqd")){
            
            //Reset game data by wiping the UUID bits
            for (uint8_t x=0; x<4; ++x){
                WriteStatusBit(110+x, 0);
            }
            SaveGameState();

            uint8_t cheat[] = "Cheater! ";
            SerSpeed(60);
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
    static object_model_t currObj, actObj1, actObj2;
    static uint16_t route[MAX_OBJ_DEPTH] = {0};
    static uint8_t currDepth = 0;
    
    static uint16_t reactStr[3][8] = {{0},{0},{0}};
    static uint8_t actionList = 0;
    static uint8_t toSend = 0;

    PopulateObject(route[currDepth], &currObj);
    
    //Responses to send after something has tried by the user
    if (actionList){
        PrepareSending(reactStr[0][toSend], reactStr[1][toSend], GAME, reactStr[2][toSend]);
        ++toSend;
        --actionList;
        if (actionList == 0) {
            RXCNT = 0;
            serRxDone = 0;
        }

    //No responses yet and data available? Figure out what to say!
    } else if (data[0]){
        toSend = 0;

        if (data[0] == 'x'){

            //Trash later
            reactStr[0][0]=currObj.addrStr[NAME];
            reactStr[1][0]=currObj.lenStr[NAME];
            reactStr[2][0]=PROMPT;
            actionList = 1;

            //Standing outside?
            if (route[currDepth] == 0){
                PrepareSending(A_NOTPOSSIBLE, L_NOTPOSSIBLE, TEASER, PROMPT);
            } else {
                //if 
            }
        /*
            if len(loc) == 0:
            invalid(eeprom)
            elif len(loc) == 1:
            msg = check_open_permission(eeprom,0)
            if msg != "":
            print(msg)
            else:
            exit_allowed = True
            else:
            exit_allowed = True

            if exit_allowed:
            del loc[-1]
            loc_offset,loc_action_mask,loc_children,loc_parent = loc2offset(eeprom,loc)
            current_effects = read_byte_field(eeprom,loc_offset,'effects')
        */


        } else
        if ((data[0] == 'e')||(data[0] == 'o')){
        /*
            if len(inp) > 2 and inp[1] in exclude_words:
            del(inp[1])
            if len(inp) != 2:
            invalid(eeprom)
            else:
            enter_offset = 0xffff
            for i in range(len(loc_children)):
            if object_visible(eeprom,loc_children[i][1]):
            if inp[1] == loc_children[i][0]:
            enter_offset = loc_children[i][1]
            break
            else:
            if object_visible(eeprom,loc_parent[1]):
            if inp[1] == loc_parent[0]:
            enter_offset = loc_parent[1]

            if enter_offset != 0xffff:
            enter_action_mask = read_byte_field(eeprom,enter_offset,'action_mask')
            if cmd == 'e' and (enter_action_mask & A_ENTER == 0):
            print(s(eeprom,'CANTENTER'))
            continue
            elif cmd == 'o' and (enter_action_mask & A_OPEN == 0):
            print(s(eeprom,'CANTOPEN'))
            continue
            msg = check_open_permission(eeprom,enter_offset)
            if msg != "":
            print(msg)
            continue
            else:
            if enter_offset != loc_parent[1]:
            loc.append(i)
            else:
            del(loc[-1])
            loc_offset,loc_action_mask,loc_children,loc_parent = loc2offset(eeprom,loc)
            current_effects = read_byte_field(eeprom,loc_offset,'effects')
            continue
            else:
            print(s(eeprom,'DONTSEE'))
        */
        
        } else
        if (data[0] == 'l'){
        /*
            if len(inp) == 1:
            print(read_string_field(eeprom,loc_offset,'desc'))
            print(s(eeprom,'LOOK'),end='')
            sep = ""
            if loc_parent[1] != 0xffff and object_visible(eeprom,loc_parent[1]):
            name = read_string_field(eeprom,loc_parent[1],'name')
            print("{}".format(name),end='')
            sep = s(eeprom,'COMMA')
            for i in range(len(loc_children)):
            if object_visible(eeprom,loc_children[i][1]):
            item = read_byte_field(eeprom,loc_children[i][1],'item_nr')
            in_inventory = False
            if item != 0:
            for inv in range(len(inventory)):
            if item == inventory[inv][0]:
            in_inventory = True
            break
            if  not in_inventory:
            name = read_string_field(eeprom,loc_children[i][1],'name')
            print("{}{}".format(sep,name),end='')
            sep = s(eeprom,'COMMA')
            print()

            else:
            if len(inp) > 2 and inp[1] in exclude_words:
            del(inp[1])
            if len(inp) != 2:
            invalid(eeprom)
            continue

            look_offset = 0xffff
            for i in range(len(loc_children)):
            if object_visible(eeprom,loc_children[i][1]):
            if inp[1] == loc_children[i][0]:
            look_offset = loc_children[i][1]
            break
            else:
            if object_visible(eeprom,loc_parent[1]):
            if inp[1] == loc_parent[0]:
            look_offset = loc_parent[1]
            else:
            print(s(eeprom,'DONTSEE'))
            continue
            if look_offset != 0xffff:
            if (read_byte_field(eeprom,look_offset,'action_mask') & A_LOOK == 0):
            name = read_string_field(eeprom,look_offset,'name')
            print(s(eeprom,'CANTLOOK') + "{}".format(name))
            continue
            else:
            desc = read_string_field(eeprom,look_offset,'desc')
            print(desc)
            continue
            else:
            invalid(eeprom)        
        */
        } else
        if (data[0] == 'p'){
        /*
             if len(inventory) >= 2:
             print(s(eeprom,'CARRYTWO'))
         
             elif len(inp) < 2:
             invalid(eeprom)

             else:
             obj_loc,obj_offset,obj_parent = name2loc(eeprom,loc,loc_offset,inp[1])
             if obj_loc is None:
             invalid(eeprom)
             else:
             obj_id   = read_byte_field(eeprom,obj_offset,'item_nr')
             if obj_id != 0:
             obj_name = read_string_field(eeprom,obj_offset,'name')
             if [obj_id,obj_name] in inventory:
             print(s(eeprom,'ALREADYCARRYING'))
             else:
             msg = check_action_permission(eeprom,obj_offset)
             if msg != "":
             print(msg)
             continue
             print(s(eeprom,'NOWCARRING') + "{}".format(obj_name))
             inventory.append([obj_id,obj_name])
             else:
             invalid(eeprom)
        */
        } else
        if (data[0] == 'd'){
        /*
            if len(inventory) == 0:
            print(s(eeprom,'EMPTYHANDS'))
        
            elif len(inp) < 2:
            invalid(eeprom)

            else:
            for i in range(len(inventory)):
            if inp[1] in inventory[i][1].lower():
            print(s(eeprom,'DROPPING') + "{}".format(inventory[i][1]))
            print(s(eeprom,'RETURNING'))
            del inventory[i]
            break
            else:
            print(s(eeprom,'NOTCARRYING'))
        */
        } else
        if (data[0] == 'i'){
        /*
        
        */
        } else
        if ((data[0] == 't')||(data[0] == 'u')||(data[0] == 'g')){
        /*
            if len(inp) > 2 and inp[1] in exclude_words:
            del(inp[1])
            elif cmd == 'u' and len(inp) > 2 and inp[2] in exclude_words:
            del(inp[2])

            if len(inp) < 2:
            invalid(eeprom)
            else:
            item = 0
            if len(inp) == 2:
            obj  = inp[1]
            elif len(inp) >= 3:
            obj  = inp[2]
            for i in range(len(inventory)):
            if inp[1] in inventory[i][1].lower():
            item = inventory[i][0]
            break
            if item == 0:
            print(s(eeprom,'NOTCARRYING'))
            continue

            obj_loc,obj_offset,obj_parent = name2loc(eeprom,loc,loc_offset,obj)
            if obj_loc is None:
            print(s(eeprom,'NOSUCHOBJECT'))
            continue
            else:
            obj_action_mask = read_byte_field(eeprom,obj_offset,'action_mask')
            if cmd == 't' and (obj_action_mask & A_TALK == 0):
            print(s(eeprom,'WHYTALK') + "{}".format(read_string_field(eeprom,obj_offset,'name')))
            continue
            elif cmd == 'u' and (obj_action_mask & A_USE == 0):
            print(s(eeprom,'CANTUSE'))
            continue
            elif cmd == 'u' and item != 0 and read_byte_field(eeprom,obj_offset,'action_item') != item:
            print(s(eeprom,'CANTUSEITEM'))
            continue
            elif cmd == 'g' and item != 0 and read_byte_field(eeprom,obj_offset,'action_item') != item:
            print(s(eeprom,'CANTGIVE'))
            continue
            else:
            msg = check_action_permission(eeprom,obj_offset)
            if msg != "":
            print(msg)
            continue

            request = read_string_field(eeprom,obj_offset,'action_str1')
            if len(request) == 1:
            if request == '1':
            print("Special game/challenge 1")
            elif request == '2':
            print("Special game/challenge 2")
            else:
            print("Undefined challenge!")
            continue
            elif len(request) > 1:
            print("{}".format(request))
            response = input(s(eeprom,'RESPONSE'))
            if unify(read_string_field(eeprom,obj_offset,'action_str2')) != unify(response):
            print(s(eeprom,'INCORRECT'))
            continue
            update_state(read_byte_field(eeprom,obj_offset,'action_state'))
            print("{}".format(read_string_field(eeprom,obj_offset,'action_msg')))          
        */
        } else {
            //No clue...
            data[0] = 0;
        }
    }
}


// Main game loop
uint8_t TextAdventure(){
    static uint8_t serInput[RXLEN];
    if (CheckSend()) return 1;                      //Still sending data to serial, return 1
    if (CheckInput(&serInput[0])) return 2;         //No valid input to process, return 2
    ProcessInput(&serInput[0]);                     //Check which response to generate and fill tx arrays
    SaveGameState();                                //Check if things are changed, write to EEPROM
    return 0;
}