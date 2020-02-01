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
parser.add_argument("-b", default=0, help="Set the badge type")

args = parser.parse_args()

DEBUG = args.d

badge = int(args.b)

if badge % 4 == 0:
    update_state(110)
elif badge % 4 == 1:
    update_state(111)
elif badge % 4 == 2:
    update_state(112)
elif badge % 4 == 3:
    update_state(113)

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

    if loc == [0,1]:
        print(s(eeprom,'CONGRATS'))
        exit()

    print(s(eeprom,'LOCATION') + "{}".format(read_string_field(eeprom,loc_offset,'name')),end='')

    # The effects should be triggered, so no need to print them I think, unless we want to
    # maybe print the sound effect for those that do not use earplugs ;-)
    if current_effects != 0:
        print(s(eeprom,'SPACE') + "{}".format(effects(current_effects)))
    else:
        print()

    # Start with getting user input
    inp = input(s(eeprom,'PROMPT'))
    print()
    if len(inp) == 0:
        continue

    if inp == 'IDDQD':
        exit()


    inp = inp.lower().split()
    cmd = inp[0][0]


    if cmd == 'h' or cmd == '?':
        show_help(eeprom)


    if cmd == 'a':
        show_alphabet(eeprom)


    if cmd == 'w':
        who_am_i(eeprom)


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


####################
# ^^  to here   ^^ #
####################



    elif cmd == 'q':
        print(s(eeprom,'QUIT'))
        exit()


    elif cmd == 'l':
        if len(inp) == 1:
#            print(read_string_field(eeprom,loc_offset,'desc'))
#            print(s(eeprom,'LOOK'),end='')
#            sep = ""
#            if loc_parent[1] != 0xffff and object_visible(eeprom,loc_parent[1]):
#                name = read_string_field(eeprom,loc_parent[1],'name')
#                print("{}".format(name),end='')
#                sep = s(eeprom,'COMMA')
#            for i in range(len(loc_children)):
#                if object_visible(eeprom,loc_children[i][1]):
#                    item = read_byte_field(eeprom,loc_children[i][1],'item_nr')
#                    in_inventory = False
#                    if item != 0:
#                        for inv in range(len(inventory)):
#                            if item == inventory[inv][0]:
#                                in_inventory = True
#                                break
#                    if  not in_inventory:
#                        name = read_string_field(eeprom,loc_children[i][1],'name')
#                        print("{}{}".format(sep,name),end='')
#                        sep = s(eeprom,'COMMA')
#            print()
            look_around(eeprom, loc_offset, loc_parent, loc_children,inventory)

        else:
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

            
    elif cmd == 'x':
        exit_allowed = False
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


    elif cmd == 'e' or cmd == 'o':
        if len(inp) != 2:
            invalid(eeprom)
        else:
            enter_offset = 0xffff
            for i in range(len(loc_children)):
                if object_visible(eeprom,loc_children[i][1]):
                    if inp[1][0] == loc_children[i][0]:
                        enter_offset = loc_children[i][1]
                        break
            else:
                if object_visible(eeprom,loc_parent[1]):
                    if inp[1][0] == loc_parent[0]:
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
                    look_around(eeprom, loc_offset, loc_parent, loc_children,inventory)
                    continue
            else:
                print(s(eeprom,'DONTSEE'))


    elif cmd == 't' or cmd == 'u' or cmd == 'g' or cmd == 'r':
        if len(inp) < 2 or len(inp)>3:
            invalid(eeprom)
        else:
            item = 0
            if len(inp) == 2:
                obj  = inp[1][0]
            elif len(inp) == 3:
                obj  = inp[2][0]
                for i in range(len(inventory)):
                    if "["+inp[1][0]+"]" in inventory[i][1].lower():
                        item = inventory[i][0]
                        break
                if item == 0:
                    print(s(eeprom,'NOTCARRYING'))
                    continue

            obj_offset = 0xffff
            for i in range(len(loc_children)):
                if object_visible(eeprom,loc_children[i][1]):
                    if obj == loc_children[i][0]:
                        obj_offset = loc_children[i][1]
                        break
            else:
                if object_visible(eeprom,loc_parent[1]):
                    if obj == loc_parent[0]:
                        obj_offset = loc_parent[1]

            if obj_offset == 0xffff:
                if cmd == 'u' or cmd == 'r':
                    print(s(eeprom,'NOSUCHOBJECT'))
                else:
                    print(s(eeprom,'NOSUCHPERSON'))
                continue
            else:
                obj_action_mask = read_byte_field(eeprom,obj_offset,'action_mask')
                msg = check_action_permission(eeprom,obj_offset)
                request = read_string_field(eeprom,obj_offset,'action_str1')
                if cmd == 't' and (obj_action_mask & A_TALK == 0):
                    print(s(eeprom,'WHYTALK') + "{}".format(read_string_field(eeprom,obj_offset,'name')))
                    continue
                elif cmd == 'u' and (obj_action_mask & A_USE == 0):
                    print(s(eeprom,'CANTUSE'))
                    continue
                elif cmd == 'r' and (obj_action_mask & A_READ == 0):
                    print(s(eeprom,'CANTREAD'))
                    continue
                elif msg != "":
                    print(msg)
                    continue
                if len(request) == 1:
                    if request == '1':
                        # Offer an item by placing it on the altar
                        # print("Special game/challenge 1")
                        if item == 0: 
                            print(s(eeprom,'PLEASEOFFER'))
                            continue
                        if item == 31 or item == 32 or item == 33 or item == 34:
                            offering = item - 31
                            print(s(eeprom,'PRIEST'))
                            response = input(s(eeprom,'RESPONSE'))
                            inp = response.lower().split()
                            if len(inp) != 2:
                                offering = -1
                                print(s(eeprom,'BADOFFERING'))
                                continue
                            kneelings = ord(inp[0][0]) - 49
                            if kneelings < 0 or kneelings > 3:
                                offering = -1
                                print(s(eeprom,'BADOFFERING'))
                                continue
                            if inp[1][0] not in elements:
                                offering = -1
                                print(s(eeprom,'BADOFFERING'))
                                continue
                            element = elements.index(inp[1][0])
                            if get_state(110):
                                person = 0
                            elif get_state(111):
                                person = 1
                            elif get_state(112):
                                person = 2
                            elif get_state(113):
                                person = 3
                            else:
                                print(s(eeprom,'ERROR'))
                                continue

                            answer = ((offering  & 2) << 19) + ((offering  & 1) << 8) + \
                                     ((element   & 2) << 15) + ((element   & 1) << 4) + \
                                     ((kneelings & 2) << 11) + ((kneelings & 1)) 
                            answer = answer << (3-person)
                            print(s(eeprom,'YOURPART') + "{}".format(answer))
                            update_state(read_byte_field(eeprom,obj_offset,'action_state'))
                            continue
                        else:
                            print(s(eeprom,'CANTUSEITEM'))
                            continue
                    elif request == '2':
                        print("Special game/challenge 2")
                    else:
                        print("Undefined challenge!")
                        continue
                elif cmd == 'u' and item != 0 and read_byte_field(eeprom,obj_offset,'action_item') != item:
                    print(s(eeprom,'CANTUSEITEM'))
                    continue
                elif cmd == 'g' and item != 0 and read_byte_field(eeprom,obj_offset,'action_item') != item:
                    print(s(eeprom,'CANTGIVE'))
                    continue
                else:
                    if len(request) > 1:
                        print("{}".format(request))
                        response = input(s(eeprom,'RESPONSE'))
                        if unify(read_string_field(eeprom,obj_offset,'action_str2')) != unify(response):
                            print(s(eeprom,'INCORRECT'))
                            continue
                    update_state(read_byte_field(eeprom,obj_offset,'action_state'))
                    print("{}".format(read_string_field(eeprom,obj_offset,'action_msg')))

        
    elif cmd == 'p':
        if len(inventory) >= 2:
            print(s(eeprom,'CARRYTWO'))
            
        elif len(inp) != 2:
            invalid(eeprom)

        else:
            obj_offset = 0xffff
            for i in range(len(loc_children)):
                if object_visible(eeprom,loc_children[i][1]):
                    if inp[1] == loc_children[i][0]:
                        obj_offset = loc_children[i][1]
                        break

            if obj_offset == 0xffff:
                print(s(eeprom,'NOSUCHOBJECT'))
                continue

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
                    update_state(read_byte_field(eeprom,obj_offset,'action_state'))
                    print("{}".format(read_string_field(eeprom,obj_offset,'action_msg')))
                    print(s(eeprom,'NOWCARRING') + "{}".format(obj_name))
                    inventory.append([obj_id,obj_name])
            else:
                invalid(eeprom)
                    

    elif cmd == 'd':
        if len(inventory) == 0:
            print(s(eeprom,'EMPTYHANDS'))
            
        elif len(inp) < 2:
            invalid(eeprom)

        else:
            for i in range(len(inventory)):
                if "["+inp[1][0]+"]" in inventory[i][1].lower():
                    print(s(eeprom,'DROPPING') + "{}".format(inventory[i][1]))
                    print(s(eeprom,'RETURNING'))
                    del inventory[i]
                    break
            else:
                print(s(eeprom,'NOTCARRYING'))


    ### end of command options
