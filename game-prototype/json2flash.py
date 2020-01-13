#!/usr/bin/env python3

import json
import pprint 

from common import *

items        = ["" for i in range(max_items)]
state_desc   = ["" for i in range(status_bits)]
action_state = [[] for i in range(status_bits)]
visible_acl  = [[] for i in range(status_bits)]
open_acl     = [[] for i in range(status_bits)]
action_acl   = [[] for i in range(status_bits)]

def show_eeprom(data):
    offset = 0
    level = 0
    cache = []

    while offset<len(data):
        if level==0 and offset != 0:
            # We're at the help file
            l = read_ptr(data,offset)
            print("0x{0:04X} help={1:}".format(offset,read_range(data,offset+2,l).decode()))
            offset += 2 + l
            return

        ptr = read_ptr(data,offset)
        if len(cache) == 0:
            print("0x{0:04X} ->0x{1:04X} Pointer to help".format(offset,ptr))
        else:
            print("\n0x{0:04X} {2:} ->0x{1:04X} Pointer to next object at this level".format(offset,ptr,"  "*level))
        cache.append(ptr)
        offset += 2

        ptr = read_ptr(data,offset)
        print("0x{0:04X} {2:} ->0x{1:04X} Pointer to next level".format(offset,ptr,"  "*level))
        offset += 2

        for i in range(len(byte_fields)):
            print("0x{0:04X} {4:} Level {1:}, {2:}={3:}".format(offset,level,byte_fields[i],read_byte(data,offset),"  "*level))
            offset += 1

        for i in range(len(string_fields)):
            l = read_byte(data,offset)
            print("0x{0:04X} {4:} Level {1:}, {2:}={3:}".format(offset,level,string_fields[i],read_range(data,offset+1,l).decode(),"  "*level))
            offset = offset + 1 + l

        if offset == cache[-1]:
            # next object at same level
            del cache[-1]
            while len(cache)>0 and offset == cache[-1]:
                # Go to previous level
                del cache[-1]
                level -= 1
            continue

        level += 1
### end of show_eeprom()


def convert_json_to_eeprom(obj,offset=0):
    nextlevel = 0
    binary    = bytearray(0)
    l = 0
    for f in byte_fields:
        if f in obj:
            binary.append(obj[f])
        else:
            binary.append(0x00)
        l = l + 1

    for f in string_fields:
        if f in obj:
            binary.append(len(obj[f]))
            binary.extend(obj[f].encode('ascii'))
            l = l + 1 + len(obj[f])
        else:
            binary.append(0x00)
            l = l + 1

    if 'item_nr' in obj:
        items[obj['item_nr']] = obj['name']

    if 'action_state' in obj:
        if 'state_desc' in obj:
            state_desc[obj['action_state']] = obj['state_desc']
        action_state[obj['action_state']].append(obj['name'])

    if 'visible_acl' in obj:
        visible_acl[obj['visible_acl']].append(obj['name'])

    if 'open_acl' in obj:
        open_acl[obj['open_acl']].append(obj['name'])

    if 'action_acl' in obj:
        action_acl[obj['action_acl']].append(obj['name'])


    nextlevel = l

    if 'o' in obj:
        for o in obj['o']:
            c = convert_json_to_eeprom(o,offset+l+4)
            l = l + len(c)
            binary = binary + c

    binary = bytearray([(offset+l+4) // 256, (offset+l+4)  % 256]) + \
             bytearray([(offset+nextlevel+4) // 256, (offset+nextlevel+4)  % 256])+ \
             binary

    return binary
### end of convert_json_to_eeprom()


### Read and convert the JSON file
with open('hotel.json', 'r') as f:
    hotel = json.load(f)
data = convert_json_to_eeprom(hotel['game'])
l = len(hotel['help'])
data = data + bytearray([l // 256, l  % 256])
data.extend(hotel['help'].encode('ascii'))


if len(data) + 64 > flash_size:
    print("The game is currently using {} bytes of EEPROM space (including boilerplate and footer).".format(len(data)+64))
    print("... but only {} bytes of flash available".format(flash_size))
    exit()


xor_data = bytearray(0)
offset = 0
for i in range(len(boiler_plate)):
    xor_data.append(boiler_plate[i] ^ xor_key_teaser[offset % len(xor_key_teaser)])
    offset += 1

for i in range(len(data)):
    xor_data.append(data[i] ^ xor_key_game[offset % len(xor_key_game)])
    offset += 1

for i in range(flash_size - len(xor_data)):
    xor_data.append(xor_key_teaser[offset % len(xor_key_teaser)])
    offset += 1

eeprom = open('hotel.bin', 'wb')
eeprom.write(xor_data)
eeprom.close

show_eeprom(xor_data)

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
print("The game is currently using {} bytes of EEPROM space (including boilerplate and footer).".format(len(data)+64))
print()
