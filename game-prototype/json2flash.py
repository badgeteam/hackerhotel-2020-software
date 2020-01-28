#!/usr/bin/env python3

#################################################################
### The code in this file does not need to be converted to FW ###
### It is just for creating the flash content                 ###
#################################################################

### Flash memory layout
###
### ADDR   CONTENT                          XOR_KEY
### 0x0000 boilerplate (32 bytes)           xor_key_teaser
### 0x0020 Game data                        xor_key_game
### 0xXXXX Empty space with random data     xor_key_teaser
### 0xXXXX Literals                         xor_key_teaser

import json
import pprint 
from random import *

from common import *
import literals

items        = ["" for i in range(max_items)]
state_desc   = ["" for i in range(status_bits*2)]
action_state = [[] for i in range(status_bits*2)]
visible_acl  = [[] for i in range(status_bits*2)]
open_acl     = [[] for i in range(status_bits*2)]
action_acl   = [[] for i in range(status_bits*2)]

def show_eeprom(data):
    offset = 0
    level = 0
    cache = []
    objects = 0

    while offset<len(data):
        objects += 1
        ptr = read_ptr(data,offset)
        if ptr == 0x0000:
            break
        print("\n0x{0:04X} {2:} ->0x{1:04X} Pointer to next object at this level".format(offset+32,ptr,"  "*level))
        cache.append(ptr)
        offset += 2

        ptr = read_ptr(data,offset)
        print("0x{0:04X} {2:} ->0x{1:04X} Pointer to next level".format(offset+32,ptr,"  "*level))
        offset += 2

        for i in range(len(byte_fields)):
            print("0x{0:04X} {4:} Level {1:}, {2:}={3:}".format(offset+32,level,byte_fields[i],read_byte(data,offset),"  "*level))
            offset += 1

        for i in range(len(string_fields)):
            start_offset = offset
            b = bytearray(0)
            l = read_ptr(data,offset)
            b = b + read_range(data,offset+2,l)
            offset = offset + 2 + l
            if i == string_fields.index('action_str2'):
                b = xor_nibble_swap(b)
            if i >= string_fields.index('open_acl_msg'):
                if len(b) > 0:
                    s = effects(b[0]) + b[1:].decode()
            else:
                s = b.decode()

            print("0x{0:04X} {4:} Level {1:}, {2:}={3:}".format(start_offset+32,level,string_fields[i],s,"  "*level))

        if offset == cache[-1]:
            # next object at same level
            del cache[-1]
            while len(cache)>0 and offset == cache[-1]:
                # Go to previous level
                del cache[-1]
                level -= 1
            continue

        level += 1
    return objects
### end of show_eeprom()


def convert_json_to_eeprom(objects,offset=0):
    result = bytearray(0)

    for name in objects:
        l = 0
        nextlevel = 0
        binary = bytearray(0)

        obj = objects[name]
        for f in byte_fields:
            if f in obj:
                binary.append(obj[f])
            else:
                binary.append(0x00)
            l = l + 1

        # Add name of the object
        binary.append(len(name)//256)
        binary.append(len(name)%256)
        binary.extend(name.encode('utf8'))
        l = l + 2 + len(name)

        for i in range(1,len(string_fields)):   # skip first string field (name), as it has been processed already
            f = string_fields[i]
            b = bytearray(0)
            if f in obj:
                # If needed, start with sound/LED effects in the string
                if i >= string_fields.index('open_acl_msg'):
                    e = string_fields[i] + "_effects"
                    if e in obj:
                        b.append(obj[e])
                    else:
                        b.append(0)
                b.extend(obj[f].encode('utf8'))
                if i == string_fields.index('action_str2'):
                    b = xor_nibble_swap(b)
                binary.append(len(b)//256)
                binary.append(len(b)%256)
                binary.extend(b)
                l = l + 2 + len(b)
            else:
                binary.append(0x00)
                binary.append(0x00)
                l = l + 2

        if 'item_nr' in obj:
            if obj['item_nr'] != 0:
                items[obj['item_nr']] = name

        if 'action_state' in obj:
            if obj['action_state'] != 0:
                if 'state_desc' in obj:
                    if obj['state_desc'] != "":
                        state_desc[obj['action_state']] = obj['state_desc']
                action_state[obj['action_state']].append(name)

        if 'visible_acl' in obj:
            if obj['visible_acl'] != 0:
                visible_acl[obj['visible_acl']].append(name)

        if 'open_acl' in obj:
            if obj['open_acl'] != 0:
                open_acl[obj['open_acl']].append(name)

        if 'action_acl' in obj:
            if obj['action_acl'] != 0:
                action_acl[obj['action_acl']].append(name)


        nextlevel = offset+l+4

        if 'o' in obj:
            c = convert_json_to_eeprom(obj['o'],nextlevel)
            l = l + len(c)
            binary = binary + c

        offset = offset+l+4

        result = result + \
                 bytearray([(offset) // 256, (offset)  % 256]) + \
                 bytearray([(nextlevel) // 256, (nextlevel)  % 256])+ \
                 binary

    return result
### end of convert_json_to_eeprom()


### Read and convert the JSON file
with open('hotel.json', 'r') as f:
    hotel = json.load(f)
game_data = convert_json_to_eeprom(hotel)
game_data.append(0x00)
game_data.append(0x00)

f1 = open('literals.h', 'w')
f2 = open('lit_offsets.py', 'w')
f2.write("lit_offsets = {\n")

literal_data   = bytearray(0)
literal_offset = flash_size
for short in literals.literals:
    l = literals.literals[short].encode('utf8')
    literal_data = l + literal_data
    literal_offset = literal_offset - len(l)
    f1.write("#define A_{:20s}   {:5d}\n".format(short,literal_offset))
    f1.write("#define L_{:20s}   {:5d}\n".format(short,len(l)))
    f2.write("    '{}': [{},{}], # '{}'\n".format(short,literal_offset,len(l),repr(l)[:40]))
f2.write("}\n")
f2.close()
f1.close()


total_length = 32 + len(game_data) + len(literal_data)
if total_length > flash_size:
    print("The game is currently using {} bytes of EEPROM space...".format(total_length))
    print("... but only {} bytes of flash available".format(flash_size))
    exit()


xor_data = bytearray(0)
offset = 0
for i in range(len(boiler_plate)):
    xor_data.append(boiler_plate[i] ^ xor_key_teaser[offset % len(xor_key_teaser)])
    offset += 1

for i in range(len(game_data)):
    xor_data.append(game_data[i] ^ xor_key_game[offset % len(xor_key_game)])
    offset += 1

for i in range(flash_size - len(xor_data) - len(literal_data)):
    xor_data.append(randrange(0,256) ^ xor_key_teaser[offset % len(xor_key_teaser)])
    offset += 1

for i in range(len(literal_data)):
    xor_data.append(literal_data[i] ^ xor_key_teaser[offset % len(xor_key_teaser)])
    offset += 1

eeprom = open('hotel.bin', 'wb')
eeprom.write(xor_data)
eeprom.close

count = show_eeprom(xor_data)

print("Items")
for i in range(max_items):
    if items[i] != "":
        print("{:3d} : {}".format(i,items[i]))

print("\nStatus logic")
for i in range(status_bits):
    used = False
    txt = []
    if state_desc[i] != "":
        txt.append("D:" + state_desc[i])
        used = True
    if action_state[i] != []:
        txt.append("C:" + ",".join(action_state[i]))
        used = True
    if visible_acl[i] != []:
        txt.append("V:" + ",".join(visible_acl[i]))
        used = True
    if open_acl[i] != []:
        txt.append("O:" + ",".join(open_acl[i]))
        used = True
    if action_acl[i] != []:
        txt.append("A:" + ",".join(action_acl[i]))
        used = True

    if used:
        print("{:3d} : {}".format(i," | ".join(txt)))


print()
print("The game is currently using {} bytes of EEPROM space".format(total_length))
print("There are currently {} objects in the game,using {} bytes ({:d} bytes per object)".format(count,len(game_data),len(game_data)//count))
print("The literal strings are using {} bytes.".format(len(literal_data)))
print()
