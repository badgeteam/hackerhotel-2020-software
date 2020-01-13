#!/usr/bin/env python3

from common import *
import argparse

parser = argparse.ArgumentParser(description='The Hacker Hotel 2020 badge adventure')
parser.add_argument("-d", default=False, action="store_true", help="Enable Debugging Output")

args = parser.parse_args()

DEBUG = args.d

### Read the EEPROM data
with open('hotel.bin', 'rb') as f:
    eeprom = f.read()


### Start of the game
loc             = []
loc_offset,loc_action_mask,loc_children = loc2offset(eeprom,loc)
#loc_action_mask = read_byte_field(eeprom,loc_offset,'action_mask')
inventory       = []

while True:
    if DEBUG:
        print("\nloc = {}".format(loc))
        print("offset = 0x{:04X}".format(loc_offset))
        print("children = {}".format(loc_children))
        print("state = {}".format(game_state))
        print("action_mask = {}".format(loc_action_mask))
        print("inventory = {}".format(inventory))

    print("\nYou are in {}".format(read_string_field(eeprom,loc_offset,'name')))

    inp = input("? ")
    print()
    if len(inp) == 0:
        continue

    inp = inp.lower().split()
    cmd = inp[0][0]


    if cmd == 'h' or cmd == '?':
        show_help(eeprom)


    elif cmd == 'q':
        print("Thank you for staying, you can check out any time you want, but you may never leave!")
        break


    elif cmd == 'l':
        if len(inp) == 1:
            print(read_string_field(eeprom,loc_offset,'desc'))
            if len(loc_children) > 0:
                print("\nI see the following: ",end='')
                sep = ""
                for i in range(len(loc_children)):
                    name = read_string_field(eeprom,loc_children[i],'name')
                    if object_visible(eeprom,loc_children[i]):
                        print("{}{}".format(sep,name),end='')
                        sep = ", "

        elif len(inp) == 2:
            for i in range(len(loc_children)):
                if object_visible(eeprom,loc_children[i]):
                    name = read_string_field(eeprom,loc_children[i],'name')
                    if inp[1] in name.lower():
                        if (read_byte_field(eeprom,loc_children[i],'action_mask') & A_LOOK == 0):
                            print("You can't look inside {} from here.".format(name))
                            break
                        else:
                            desc = read_string_field(eeprom,loc_children[i],'desc')
                            print(desc)
                            break
        else:
            invalid()

            
    elif cmd == 'x':
        exit_allowed = False
        if len(loc) == 0:
            invalid()
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
            loc_offset,loc_action_mask,loc_children = loc2offset(eeprom,loc)


    elif cmd == 'e' or cmd == 'o':
        if len(inp) < 2:
            invalid()
        else:
            for i in range(len(loc_children)):
                if object_visible(eeprom,loc_children[i]):
                    name = read_string_field(eeprom,loc_children[i],'name')
                    if inp[1] in name.lower():
                        if cmd == 'e' and (loc_action_mask & A_ENTER == 0):
                            print("You can't enter {}".format(name))
                            break
                        elif cmd == 'o' and (loc_action_mask & A_OPEN == 0):
                            print("You can't open {}".format(name))
                            break
                        msg = check_open_permission(eeprom,loc_children[i])
                        if msg != "":
                            print(msg)
                            break
                        else:
                            loc.append(i)
                            loc_offset,loc_action_mask,loc_children = loc2offset(eeprom,loc)
                            break


    elif cmd == 't' or cmd == 'u':
        if len(inp) < 2:
            invalid()

        else:
            item = 0
            if len(inp) == 2:
                obj  = inp[1]
            elif len(inp) == 3:
                obj  = inp[2]
                for i in range(len(inventory)):
                    if inp[1] in inventory[i][1].lower():
                        print("Found item {}".format(inventory[i][1]))
                        item = inventory[i][0]
                        break
                if item == 0:
                    print("You are not carrying that item.")
                    continue

            obj_loc,obj_offset = name2loc(eeprom,loc,loc_offset,obj)
            if obj_loc is None:
                print("No such object here.")
                continue
            else:
                obj_action_mask = read_byte_field(eeprom,obj_offset,'action_mask')
                if cmd == 't' and (obj_action_mask & A_TALK == 0):
                    print("Why are you trying to talk to {}".format(read_string_field(eeprom,obj_offset,'name')))
                    continue
                elif cmd == 'u' and (obj_action_mask & A_USE == 0):
                    print("You can't use this object.")
                    continue
                elif cmd == 'u' and item != 0 and read_byte_field(eeprom,obj_offset,'action_item') != item:
                    print("You can't use this item on this object.")
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
                        response = input("(your response) ? ")
                        if read_string_field(eeprom,obj_offset,'action_str2').lower() != response.lower():
                            print("That is incorrect!")
                            continue
                    set_state(read_byte_field(eeprom,obj_offset,'action_state'))
                    print("{}".format(read_string_field(eeprom,obj_offset,'action_msg')))

        
    elif cmd == 'p':
        if len(inventory) >= 2:
            print("You can only carry 2 objects, please drop another object\nif you really need this object.")
            
        elif len(inp) < 2:
            invalid()

        else:
            obj_loc,obj_offset = name2loc(eeprom,loc,loc_offset,inp[1])
            if obj_loc is None:
                invalid()
            else:
                obj_id   = read_byte_field(eeprom,obj_offset,'item_nr')
                if obj_id != 0:
                    obj_name = read_string_field(eeprom,obj_offset,'name')
                    if [obj_id,obj_name] in inventory:
                        print("You are already carrying that object.")
                    else:
                        print("You are now carrying {}".format(obj_name))
                        inventory.append([obj_id,obj_name])
                else:
                    invalid()
                    

    elif cmd == 'd':
        if len(inventory) == 0:
            print("Uhmmm... you're not carrying anything!")
            
        elif len(inp) < 2:
            invalid()

        else:
            for i in range(len(inventory)):
                if inp[1] in inventory[i][1].lower():
                    print("Dropping object {}".format(inventory[i][1]))
                    print("How nice, a hotel clerk picked it up and brought it \nback to it's original location.")
                    break
            if i == len(inventory):
                print("You are not carrying that object.")
            else:
                del inventory[i]


    elif cmd == 'i':
        if DEBUG:
            if len(inp) == 3:
                inventory = [[int(inp[1]),inp[2]]]
        print("Inventory is now {}".format(inventory))


    elif cmd == 's':
        if DEBUG:
            if len(inp) > 1:
                toggle_state(int(inp[1]))
            print("The game state is now:")
            for i in range(status_bits//8):
                print("0x{:02X}:{:08b}".format(i,game_state[i]))


    ### end of command options

    if loc == [] and get_state(status_bits-1):
        print("CONGRATULATIONS!!!")
        print("The spell has been broken and you found your way out of the hotel.")
        exit()

    print()
