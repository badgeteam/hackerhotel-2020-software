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

static object_model_t currObj;              //Object data for current posistion in game
static uint8_t currDepth = 0xff;            //Depth of position in game, 0xff indicates game data is not loaded from eeprom yet.
static uint16_t route[MAX_OBJ_DEPTH] = {0};
static uint16_t reactStr[3][32] = {{0},{0},{0}};
static uint8_t responseList = 0;

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

//Empty all other strings
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

//Put the addresses of text that has to be sent in the address/length lists.
uint8_t PrepareSending(uint16_t address, uint16_t length, uint8_t type){
    uint8_t x=0;
    if (length){
        
        while (length>255) {
            txAddrList[x] = address;
            txStrLen[x] = 255;
            address += 255;
            length -= 255;
            ++x;
            if (x==(TXLISTLEN-1)) return 1;
        }
        txAddrList[x]=address;
        txStrLen[x]=length%255;
        if (length>255) return 1; 
        txTypeNow = type;
    } else {
        txAddrList[0] = 0;
    }

    //Get rid of old data from previous run
    ClearTxAfter(x);

    txAddrNow = 0;  //Start at first pointer, initiates sending in CheckSend
    return 0;
}

void SetResponse(uint8_t number, uint16_t address, uint16_t length, uint8_t type){
    reactStr[0][number]=address;
    reactStr[1][number]=length;
    reactStr[2][number]=type;
}

uint8_t SetStandardResponse(uint8_t custStrEnd){

    SetResponse(custStrEnd++, A_LF, 2, TEASER);
    SetResponse(custStrEnd++, A_LOCATION, L_LOCATION, TEASER);
    SetResponse(custStrEnd++, A_SPACE, L_SPACE, TEASER);
    reactStr[0][custStrEnd++] = CURR_LOC;
    SetResponse(custStrEnd++, A_LF, 1, TEASER);
    SetResponse(custStrEnd++, A_PROMPT, L_PROMPT, TEASER);
    SetResponse(custStrEnd++, A_SPACE, L_SPACE, TEASER);

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
        ExtEERead(offset, 2, GAME, &data[0]);
        parStr = (data[0]<<8|data[1]);
        object->lenStr[x]= parStr;
        
        //Determine string start location and add length to offset for next field
        offset += 2;
        object->addrStr[x]=offset;
        offset += parStr;
    }    
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

//Check if the entered letter corresponds with a name
uint8_t CheckLetter(uint16_t child, uint8_t letter){
    
    uint8_t found = 0;
    uint8_t data[32];

    child += L_BOILER;
    ExtEERead(child+OFF_STRINGFLDS, 2, GAME, &data[0]);
    uint8_t x = data[1]; //Assuming a name is not longer than 255 characters.

    while (x){
        uint8_t max = (x>32)?32:x;
        ExtEERead(child+OFF_STRINGFLDS+1, max, GAME, &data[0]);
        for (uint8_t y=0; y<max; ++y){
            if (found){
                if ((data[y]|0x20) == letter) return 1; else return 0;
            }
            if (data[y] == '[') found = 1;
        }
        child += max;
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

//
uint8_t InpOkChk(uint8_t test){
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
        //Load things from EEPROM
        EERead(0, &gameState[0], 16);   //Load game status bits from EEPROM

        //idSet can be used to detect cheating. After cheating idSet will be 0, with a virgin badge idSet will be 3.
        uint8_t idSet = 0;
        for (uint8_t x=0; x<4; ++x){
            idSet += ReadStatusBit(110+x);
        }

        if (idSet == 1) {
            ;
        } else if (idSet == 3) {
            //TODO: Test all functions program (for badge check during sweatshop), skip if already passed (save result + temperature calibration in EEPROM location)
            Reset();
        } else {
            //Data not as expected, cheated or corrupted memory
            Reset();
        }

        //Start at first location
        PopulateObject(route[0], &currObj);
        currDepth = 0;
    }

    //Play an effect if configured (if )
    if ((effect < 0x0100) && (effect ^ currObj.byteField[EFFECTS])){
        effect = currObj.byteField[EFFECTS];
        auStart = ((effect&0xE0)>0);
    }

    if (serRxDone){
        //Read up to the \0 character and convert to lower case.
        for (uint8_t x=0; serRx[x]!=0; ++x){
            if ((serRx[x]<'A')||(serRx[x]>'Z')) data[x]=serRx[x]; else data[x]=serRx[x]|0x20;
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
            //Put help pointers/lengths in tx list
            SetResponse(0, A_HELP, L_HELP, TEASER);
            responseList = SetStandardResponse(1);
            return 1;
        }

        //Quit text
        if (data[0] == 'q'){
            //Put quit pointers/lengths in tx list
            SetResponse(0, A_QUIT, L_QUIT, TEASER);
            responseList = SetStandardResponse(1);
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
uint8_t ProcessInput(uint8_t *data){
//    enum {NAME, DESC, ACTION_STR1, ACTION_STR2, OPEN_ACL_MSG, ACTION_ACL_MSG, ACTION_MSG};
    static object_model_t actObj1, actObj2;

    CleanInput(&data[0]);
    uint8_t inputLen = CleanInput(&data[0]);

    if (inputLen) {
        if (data[0] == 'x'){

            //Standing in the Lobby?
            if ((route[currDepth] == 0)||(currDepth == 0)){
                SetResponse(0, A_NOTPOSSIBLE, L_NOTPOSSIBLE, TEASER);
                responseList = SetStandardResponse(1);
            
            //Is there a way to go back?
            } else if (CheckState(currObj.byteField[OPEN_ACL])){
                --currDepth;
                PopulateObject(route[currDepth], &currObj);
                responseList = SetStandardResponse(0);
            
            //No way out, print denied message of location
            } else {
                SetResponse(0, currObj.addrStr[OPEN_ACL_MSG], currObj.lenStr[OPEN_ACL_MSG], GAME);               
                responseList = SetStandardResponse(1);
            }   
            
        } else if ((data[0] == 'e')||(data[0] == 'o')) {
                
            //Not possible, too many/little characters
            if (inputLen != 2){
                SetResponse(0, A_NOTPOSSIBLE, L_NOTPOSSIBLE, TEASER);
                responseList = SetStandardResponse(1);
            } else {
                uint8_t canDo = 0;
                route[currDepth+1] = FindChild(route[currDepth], data[1], 0);
                    
                //Child found?
                if (route[currDepth+1]) {
                    PopulateObject(route[currDepth+1], &actObj1);
                    canDo = 1;
                //No child, maybe a step back, letter ok?
                } else if (currDepth) {
                    PopulateObject(route[currDepth-1], &actObj1);
                    if (CheckLetter(actObj1.addrStr[NAME], data[1])) {
                        canDo = 1; 
                    }
                }

                //The candidate is found! Let's check if the action is legit
                if (canDo) {
                    volatile uint8_t bEnter = (data[0] == 'e');
                    if ((actObj1.byteField[ACTION_MASK]&(bEnter?ENTER:OPEN))==0) {
                        SetResponse(0, bEnter?A_CANTENTER:A_CANTOPEN, bEnter?L_CANTENTER:L_CANTOPEN, TEASER);
                        responseList = SetStandardResponse(1);
                        
                    //Action legit, permission granted?
                    } else if (CheckState(actObj1.byteField[OPEN_ACL])) {
                            
                        //Yes! Check if we must move forward or backwards.
                        if (route[currDepth+1]) ++currDepth; else --currDepth;
                        PopulateObject(route[currDepth], &currObj);
                        responseList = SetStandardResponse(0);
                                                  
                    //Not granted!
                    } else {
                        route[currDepth+1] = 0;
                        SetResponse(0, currObj.addrStr[OPEN_ACL_MSG], currObj.lenStr[OPEN_ACL_MSG], GAME);                
                        responseList = SetStandardResponse(1);
                    }

                //No candidate
                } else {
                    SetResponse(0, A_DONTSEE, L_DONTSEE, TEASER);                
                    responseList = SetStandardResponse(1);
                }
            }

        } else if (data[0] == 'l') {
            if (inputLen == 1) {
                uint8_t elements = 0;

                //Show info about this area first
                SetResponse(elements++, currObj.addrStr[DESC], currObj.lenStr[DESC],GAME);
                SetResponse(elements++, A_COMMA, L_COMMA, TEASER);
                SetResponse(elements++, A_SPACE, L_SPACE, TEASER);

                //Check the visible children first
                route[currDepth+1] = 0;
                do{
                    route[currDepth+1] = FindChild(route[currDepth], 0, route[currDepth+1]);
                    if (route[currDepth+1]) {
                        PopulateObject(route[currDepth+1], &actObj1);
                        SetResponse(elements++, actObj1.addrStr[NAME], actObj1.lenStr[NAME],GAME);
                        SetResponse(elements++, A_COMMA, L_COMMA, TEASER);
                        SetResponse(elements++, A_SPACE, L_SPACE, TEASER);
                    }
                }while (route[currDepth+1]);

                //Look back if not on level 0
                if (currDepth) {
                    PopulateObject(route[currDepth-1], &actObj1);
                    SetResponse(elements++, actObj1.addrStr[NAME], actObj1.lenStr[NAME],GAME);
                } else --elements;
                responseList = SetStandardResponse(elements);

            } else {

            }
            
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
        if (data[0] == 'p') {
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
        if (data[0] == 'd') {
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
        if (data[0] == 'i') {
        /*
        
        */
        } else
        if ((data[0] == 't')||(data[0] == 'u')||(data[0] == 'g')) {
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
        
            ;//No clue, no valid input...
                
        }
            
        //Input handled
        data[0] = 0;
        serRxDone = 0;
        RXCNT = 0;
    }
    
    return 0;
}


// Main game loop
uint8_t TextAdventure(){
    static uint8_t serInput[RXLEN];
    
    //Sending data to serial?
    if (CheckSend()) return 1;

    //Not sending? Process next response part to send
    if (CheckResponse()) return 1;        

    //No responses to send, check if there is user input.
    if (CheckInput(&serInput[0])) return 2; //No (valid) input

    //Input found, process and save (changes only)
    ProcessInput(&serInput[0]);                     //Check which response to generate and fill tx arrays
    SaveGameState();                                //Check if things are changed, write to EEPROM

    return 0;
}