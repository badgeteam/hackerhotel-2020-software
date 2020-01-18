#!/usr/bin/env python3

from common import *
from literals import *
import argparse

global inventory
global current_effects

####################
# not needed in FW #
# vv from here  vv #
####################
parser = argparse.ArgumentParser(description='The Hacker Hotel 2020 badge adventure')
parser.add_argument("-d", default=False, action="store_true", help="Enable Debugging Output")
parser.add_argument("-t", default=False, action="store_true", help="Print game tree and logic")

args = parser.parse_args()

DEBUG = args.d


### Read the EEPROM data
with open('hotel.bin', 'rb') as f:
    eeprom = f.read()
####################
# ^^  to here   ^^ #
####################

### Start of the game
loc             = []
loc_offset,loc_action_mask,loc_children,loc_parent = loc2offset(eeprom,loc)
current_effects = read_byte_field(eeprom,loc_offset,'effects')
inventory = []

####################
# not needed in FW #
# vv from here  vv #
####################
if args.t:
    print_tree(eeprom,[],[-1],0,inventory)

while True:
    if DEBUG:
        print("\nloc = {}".format(loc))
        print("effects = {}".format(current_effects))
        print("state = {}".format(game_state))
        print("action_mask = {}".format(loc_action_mask))
        print("inventory = {}".format(inventory))
        print("offset = 0x{:04X}".format(loc_offset))
        print("children = {}".format(loc_children))
        print()
####################
# ^^  to here   ^^ #
####################


    print(s(eeprom,'\nLocation: ') + "{}".format(read_string_field(eeprom,loc_offset,'name')),end='')

    # The effects should be triggered, so no need to print them I think, unless we want to
    # maybe print the sound effect for those that do not use earplugs ;-)
    if current_effects != 0:
        print(s(eeprom,' ') + "{}".format(effects(current_effects)))
    else:
        print()

    # Start with getting user input
    inp = input(s(eeprom,'? '))
    print()
    if len(inp) == 0:
        continue

    inp = inp.lower().split()
    cmd = inp[0][0]


    if cmd == 'h' or cmd == '?':
        show_help(eeprom)


####################
# not needed in FW #
# vv from here  vv #
####################

    elif inp[0] == "tree":
        print_tree(eeprom,[],loc,0,inventory)


    elif inp[0] == "debug":
        DEBUG = not DEBUG
        print("Debugging: {}".format(DEBUG))

    elif cmd == 's':
        if DEBUG:
            if len(inp) > 1:
                update_state(int(inp[1]))
            print("The game state is now:")
            for i in range(status_bits//8):
                print("0x{:02X}:{:08b}".format(i,game_state[i]))


    elif cmd == 'i':
        if DEBUG:
            if len(inp) == 3:
                inventory = [[int(inp[1]),inp[2]]]
        print("Inventory is now {}".format(inventory))


    elif inp[0] == 'quit':
        exit()

####################
# ^^  to here   ^^ #
####################


    elif inp[0] == 'IDDQD':
        exit()


    elif cmd == 'q':
        print(s(eeprom,'Thank you for staying, you can check out any time you want, but you may never leave!'))
        exit()


    elif cmd == 'l':
        if len(inp) == 1:
            print(read_string_field(eeprom,loc_offset,'desc'))
            print(s(eeprom,'\nI see the following: '),end='')
            sep = ""
            if loc_parent[1] != 0xffff and object_visible(eeprom,loc_parent[1]):
                name = read_string_field(eeprom,loc_parent[1],'name')
                print("{}".format(name),end='')
                sep = s(eeprom,', ')
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
                        sep = s(eeprom,', ')
            print()

        else:
            if len(inp) > 2 and inp[1] in exclude_words:
                del(inp[1])
            if len(inp) != 2:
                invalid()
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
                        print(s(eeprom,'I don\'t see that!'))
                        continue
            if look_offset != 0xffff:
                if (read_byte_field(eeprom,look_offset,'action_mask') & A_LOOK == 0):
                    print(s(eeprom,'You can\'t look inside ') + "{}".format(name))
                    continue
                else:
                    desc = read_string_field(eeprom,look_offset,'desc')
                    print(desc)
                    continue
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
            loc_offset,loc_action_mask,loc_children,loc_parent = loc2offset(eeprom,loc)
            current_effects = read_byte_field(eeprom,loc_offset,'effects')


    elif cmd == 'e' or cmd == 'o':
        if len(inp) > 2 and inp[1] in exclude_words:
            del(inp[1])
        if len(inp) != 2:
            invalid()
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
                    print(s(eeprom,'You can\'t enter that location.'))
                    continue
                elif cmd == 'o' and (enter_action_mask & A_OPEN == 0):
                    print(s(eeprom,'You can\'t open that object.'))
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
                print(s(eeprom,'I don\'t see that location.'))


    elif cmd == 't' or cmd == 'u':
        if len(inp) > 2 and inp[1] in exclude_words:
            del(inp[1])
        elif cmd == 'u' and len(inp) > 2 and inp[2] in exclude_words:
            del(inp[2])

        if len(inp) < 2:
            invalid()
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
                    print(s(eeprom,'You are not carrying that item.'))
                    continue

            obj_loc,obj_offset,obj_parent = name2loc(eeprom,loc,loc_offset,obj)
            if obj_loc is None:
                print(s(eeprom,'No such object here.'))
                continue
            else:
                obj_action_mask = read_byte_field(eeprom,obj_offset,'action_mask')
                if cmd == 't' and (obj_action_mask & A_TALK == 0):
                    print(s(eeprom,'Why are you trying to talk to ') + "{}".format(read_string_field(eeprom,obj_offset,'name')))
                    continue
                elif cmd == 'u' and (obj_action_mask & A_USE == 0):
                    print(s(eeprom,'You can\'t use this object.'))
                    continue
                elif cmd == 'u' and item != 0 and read_byte_field(eeprom,obj_offset,'action_item') != item:
                    print(s(eeprom,'You can\'t use this item on this object.'))
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
                        response = input(s(eeprom,'(your response) ? '))
                        if unify(read_string_field(eeprom,obj_offset,'action_str2')) != unify(response):
                            print(s(eeprom,'That is incorrect!'))
                            continue
                    update_state(read_byte_field(eeprom,obj_offset,'action_state'))
                    print("{}".format(read_string_field(eeprom,obj_offset,'action_msg')))

        
    elif cmd == 'p':
        if len(inventory) >= 2:
            print(s(eeprom,'You can only carry 2 objects, please drop another object\nif you really need this object.'))
            
        elif len(inp) < 2:
            invalid()

        else:
            obj_loc,obj_offset,obj_parent = name2loc(eeprom,loc,loc_offset,inp[1])
            if obj_loc is None:
                invalid()
            else:
                obj_id   = read_byte_field(eeprom,obj_offset,'item_nr')
                if obj_id != 0:
                    obj_name = read_string_field(eeprom,obj_offset,'name')
                    if [obj_id,obj_name] in inventory:
                        print(s(eeprom,'You are already carrying that object.'))
                    else:
                        msg = check_action_permission(eeprom,obj_offset)
                        if msg != "":
                            print(msg)
                            continue
                        print(s(eeprom,'You are now carrying: ') + "{}".format(obj_name))
                        inventory.append([obj_id,obj_name])
                else:
                    invalid()
                    

    elif cmd == 'd':
        if len(inventory) == 0:
            print(s(eeprom,'Uhmmm... you\'re not carrying anything!'))
            
        elif len(inp) < 2:
            invalid()

        else:
            for i in range(len(inventory)):
                if inp[1] in inventory[i][1].lower():
                    print(s(eeprom,'Dropping object ') + "{}".format(inventory[i][1]))
                    print(s(eeprom,'A hotel clerk brought the item back to its original location.'))
                    del inventory[i]
                    break
            else:
                print(s(eeprom,'You are not carrying that object.'))


    ### end of command options

    if loc == [] and get_state(status_bits-1):
        print(s(eeprom,'CONGRATULATIONS!!!\nThe spell has been broken and you found your way out of the hotel.'))
        exit()
