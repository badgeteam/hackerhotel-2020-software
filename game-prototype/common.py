import literals
from lit_offsets import *

# these definitions are used for importing the JSON data. I also
# use them as constants in game.py, but in the FW source-code, they
# can just be DEFINEs in combination with a define for the length 
# of both of these lists
byte_fields   = ['effects',         # Sound/LED effects to play when entering/opening this room/object
                 'visible_acl',     # status bit needed to show object or look at it
                 'open_acl',        # status bit needed to open/enter object
                 'action_acl',      # first status bit needed to perform an action on the object
                 'action_mask',     # which actions can be performed on this object
                 'action_item',     # item number that can be used on this object
                 'action_state',    # status bit that will change after the action
                 'item_nr']         # This object has item number X

string_fields = ['name',            # name to show in lists
                 'desc',            # text to show when looking at the object
                 'action_str1',     # string1 used for an action, like the challenge for a challenge/response action
                 'action_str2',     # string2 used for an action, like the response for a challenge/response action
                 'open_acl_msg',    # text to show when the open_acl prevents opening/entering the object
                 'action_acl_msg',  # text to show when the action_acl prevents an action on the object
                 'action_msg']      # text to show when an action on the object was successful

# For the string fields '*_msg', sound/led effects can be added by adding an
# object <fieldname>_effects to the JSON file, this will prepend the string with one
# byte containing 0bSSSLLLLL where S is the sound effect and LLL the LED show to play.
# No sound effects for these fields show prepend a 0x00 byte to the string.
# When entering a new room, the sound effect in 'effects' should be used.
# Any time one of the '*_msg' fields is shown, the sound/led effects in the first
# byte of the field value should be activated.

####################
# not needed in FW #
# vv from here  vv #
####################
sound_effects = ['<none>',
                 'bad (buzzer)',            # can be used when a bad answer is given
                 'good (bell)',             # can be used when a good answer is given
                 'wind',                    # 
                 'footsteps',
                 'knocking',
                 'screaming',
                 '<free>']

led_effects   = ['<none>',
                 'flashing red eyes',       # can be used when a bad answer is given
                 'flashing green eyes',     # can be used when a good answer is given
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>',
                 '<free>']
####################
# ^^  to here   ^^ #
####################

global current_effects
def effects(e):
    sound = e>>5
    led   = e&31
    return "[" + sound_effects[sound] + "," + led_effects[led] + "]"


# Action constants, defining the bits in 'action_mask'
A_ENTER =  1   # allowed to enter an object (room, elevator, etc)          NB: ENTER and OPEN are mutually exclusive !!!
A_OPEN  =  2   # allowed to open an object (closet, drawer, etc)           NB: ENTER and OPEN are mutually exclusive !!!
A_LOOK  =  4   # allowed to look at an object from the parent
A_TALK  =  8   # allowed to talk to an object (barman, receptionist, etc)  NB: TALK and USE are mutually exclusive !!!
A_USE   = 16   # allowed to use an object (like computer, knife, etc)      NB: TALK and USE are mutually exclusive !!!

max_object_depth = 32
max_items        = 64
status_bits      = 128

exclude_words    = ('on','to','in','with','at','from')

# Keys are md5 hash of 'H@ck3r H0t3l 2020', split in two
xor_key_game   = b'\x74\xbf\xfa\x54\x1c\x96\xb2\x26'
xor_key_teaser = b'\x1e\xeb\xd6\x8b\xc0\xc2\x0a\x61'
#xor_key_game   = b'\x00' * 8
#xor_key_teaser = b'\x00' * 8
flash_size     = 32768
boiler_plate   = b'Hacker Hotel 2020 by badge.team ' # boiler_plate must by 32 bytes long


def read_byte(eeprom,offset,key=1):
    if key == 1:
        return (eeprom[offset+32] ^ xor_key_game[offset % len(xor_key_game)])
    else:
        return (eeprom[offset] ^ xor_key_teaser[offset % len(xor_key_teaser)])


def read_range(eeprom,offset,length,key=1):
    result = bytearray(0)
    for i in range(length):
        result.append(read_byte(eeprom,offset+i,key))
    return result


def read_ptr(eeprom,offset):
    return 256 * int(read_byte(eeprom,offset)) + int(read_byte(eeprom,offset+1))


def read_handle(eeprom,offset):
    offset = offset + 4 + len(byte_fields)
    l = read_byte(eeprom,offset)
    while l>0:
        offset += 1
        c = chr(read_byte(eeprom,offset)).lower()
        if c == '[':
            c = chr(read_byte(eeprom,offset+1)).lower()
            return c
        l -= 1
    return ' '


def read_children(eeprom,offset):
    children = []
    current = offset
    nxt    = read_ptr(eeprom,offset)
    offset = read_ptr(eeprom,offset+2)
    if offset >= nxt:
        return []
    while True:
        handle = read_handle(eeprom,offset)
        children.append([handle,offset])
        offset = read_ptr(eeprom,offset)
        if offset >= nxt:
            return children
    

def loc2offset(eeprom,loc):
    offset = 0
    parent = 0xffff
    handle = ' '

    if loc != []:
        for l in range(len(loc)):
            parent = offset
            handle = read_handle(eeprom,offset)
            nxt    = read_ptr(eeprom,offset)
            offset = read_ptr(eeprom,offset+2)
            if offset >= nxt:
                return [0xffff,None,None,None]
            for i in range(loc[l]):
                offset = read_ptr(eeprom,offset)
                #print("{} {} 0x{:04X} 0x{:04x}".format(l,i,offset,nxt))
                if offset >= nxt:
                    return [0xffff,None,None,None]

    mask     = read_byte_field(eeprom,offset,'action_mask')
    children = read_children(eeprom,offset)
    return [offset,mask,children,[handle,parent]]
### end of loc2offset()


def name2loc(eeprom,loc,loc_offset,handle):
    if loc_offset == 0:
        offset,d1,children,parent = loc2offset(eeprom,loc)
    # XXX change the following code to use read_children
    i = 0
    while True:
        offset,d1,d2,parent = loc2offset(eeprom,loc+[i])
        if offset == 0xffff:
            break
        if handle == parent[0]:
                break
        i += 1
        # failsafe
        if i >= max_object_depth:
            print("ERROR: out-of-band-error")
            break
    if offset == 0xffff:
        return [None,None,None]
    return [loc+[i],offset,parent]
### end of name2offset()

    
def read_byte_field(eeprom,offset,field):
    if field in byte_fields:
        if offset != 0xffff:
            return read_byte(eeprom,offset + 4 + byte_fields.index(field))
    return -1


def read_string_field(eeprom,offset,field):
    global current_effects

    if field in string_fields:
        if offset != 0xffff:
            offset = offset + 4 + len(byte_fields)
            for i in range(string_fields.index(field)):
                while True:
                    l = read_byte(eeprom,offset)
                    offset = offset + l + 1
                    if l != 255:
                        break
            s = ""
            while True:
                l = read_byte(eeprom,offset)
                s += read_range(eeprom,offset+1,read_byte(eeprom,offset)).decode()
                offset = offset + 1 + l
                if l != 255:
                    break

            if string_fields.index(field) >= string_fields.index('open_acl_msg'):
                if len(s) > 0:
                    current_effects = ord(s[0])
                    s = effects(current_effects) + s[1:]
            return s
    return "N/A"


def unify(s):
    exclude = '!"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}~ \t'
    s = ''.join(ch for ch in s if ch not in exclude)
    return s.lower()


def s(eeprom,lit):
    if lit in lit_offsets:
        offset,length = lit_offsets[lit]
        return read_range(eeprom,offset,length,0).decode()
    else:
        print("ERROR: {} not listed in literals.py".format(lit))
        exit()


def show_help(eeprom):
    offset,length = lit_offsets['help']
    print(read_range(eeprom,offset,length,0).decode())


def who_am_i(eeprom):
    if check_state(110):
        name_offset,name_length = lit_offsets['Anubis']
    elif check_state(111):
        name_offset,name_length = lit_offsets['Bes']
    elif check_state(112):
        name_offset,name_length = lit_offsets['Khonsu']
    elif check_state(113):
        name_offset,name_length = lit_offsets['Thoth']
    else:
        printf(s(eeprom,'ERROR'))
        return

    print(s(eeprom,'HELLO'),end='')
    print(read_range(eeprom,name_offset,name_length,0).decode(),end='')
    print(s(eeprom,'PLEASED'))


def invalid(eeprom):
    print(s(eeprom,'NOTPOSSIBLE'))


###################################
###   State related functions   ###
###################################

# status bits that will be used for the game logic
# bit 0 is not used for code/memory efficiency, status of bit 0 is considered True
game_state = [0 for i in range(status_bits//8)]

# Some state bits are used from outside the game, these are:
# 110   set to 1 by FW if badge UUID mod 4 == 0
# 111   set to 1 by FW if badge UUID mod 4 == 1
# 112   set to 1 by FW if badge UUID mod 4 == 2
# 113   set to 1 by FW if badge UUID mod 4 == 3
# 114   <free>
# 115   <free>
# 116   set to 1 by FW if badge is in the dark
# 117   set to 1 by FW after one normal-hot-normal cycle of the temp sensor
# 118   set to 1 by FW after a second normal-hot-normal cycle of the temp sensor
# 119   read by FW to enable the magnetic maze game (only allow the game when this bit is 1)
# 120   set to 1 by FW after the maze game is succesfully finished
# 121   <free>
# 122   H turns green, set to 1 by FW after reaching level 6 of Bastet dictates
# 123   A turns green, set to 1 by FW after Lanyard code has been entered correctly
# 124   C turns green, set to 1 by FW after Connected to other 3 badge types / personas (UUID MOD 4) 
# 125   K turns green, set to 1 by game after Reached first floor in hotel (hotel guests only)
# 126   E turns green, set to 1 by game after Reached second floor in hotel (VIPs only)
# 127   R turns green, set to 1 by game Was able to leave the hotel (need to solve puzzles in secret room to get antidote)


# get_state() returns True if the status bit <num> is set to 1.
def get_state(num):
    if num > 0 and num < status_bits:
        byte = num >> 3
        bit = 1 << (num & 7)
        return((game_state[byte]&bit) != 0)
    else:
        return True

# update_state() updates a status bit
# The higher order bit is used to either set (0) or reset (1) the
# status bit. The other bits determine the state number.
def update_state(num):
    new_state = num & status_bits
    num = num & (status_bits-1)
    byte = num >> 3
    bit = 1 << (num & 7)

    # MSB set means reset the status bit
    # Less confusing as most state changes are setting bits
    if new_state == 0:
        game_state[byte] |= bit
    else:
        game_state[byte] &= (255 - bit)
    return

def check_state(state_num):
    if state_num & status_bits == 0:
        needed = True
    else:
        needed = False
    return get_state(state_num & (status_bits - 1)) == needed

def check_open_permission(eeprom,offset):
    if check_state(read_byte_field(eeprom,offset,'open_acl')):
        return ""
    else:
        return read_string_field(eeprom,offset,'open_acl_msg')
### end of check_open_permission()


def check_action_permission(eeprom,offset):
    if check_state(read_byte_field(eeprom,offset,'action_acl')):
        return ""
    else:
        return read_string_field(eeprom,offset,'action_acl_msg')
### end of check_action_permission()


def object_visible(eeprom,offset):
    state_num = read_byte_field(eeprom,offset,'visible_acl')
    return get_state(state_num)
### end of checkpermission()

##########################################
###   End of State related functions   ###
##########################################


####################
# not needed in FW #
# vv from here  vv #
####################
##########################################
###   Debugging functions              ###
##########################################
class color:
    NORMAL     = '\033[0m'
    BOLD       = '\033[1m'
    UNDERLINE  = '\033[4m'
    BLACK      = '\033[30m'
    RED        = '\033[31m'
    GREEN      = '\033[32m'
    YELLOW     = '\033[33m'
    BLUE       = '\033[34m'
    MAGENTA    = '\033[35m'
    CYAN       = '\033[36m'
    WHITE      = '\033[37m'

def print_summary(eeprom,loc,current_loc,offset,inventory):
    c = color.NORMAL

    if loc == current_loc:
        print(color.BOLD,end='')
    print("{:20s} ".format(str(loc)),end='')

    tree_str  = ""
    tree_str += "{}+- ".format("|  "*len(loc))
    item = read_byte_field(eeprom,offset,'item_nr')
    if item != 0:
        tree_str += color.BLUE + "({})".format(item)
    else:
        tree_str += color.BLACK
    print("{:50s} ".format(tree_str + read_string_field(eeprom,offset,'name')),end='')

    logic_str = ""
    mask = read_byte_field(eeprom,offset,'action_mask')
    if mask & A_USE:
        logic_str += "U"
    else:
        logic_str += "."
    if mask & A_TALK:
        logic_str += "T"
    else:
        logic_str += "."
    if mask & A_LOOK:
        logic_str += "L"
    else:
        logic_str += "."
    if mask & A_OPEN:
        logic_str += "O"
    else:
        logic_str += "."
    if mask & A_ENTER:
        logic_str += "E"
    else:
        logic_str += "."

    acl = read_byte_field(eeprom,offset,'visible_acl')
    if acl != 0:
        if get_state(acl):
            c = color.GREEN
        else:
            c = color.RED
        logic_str += " {}V:{:3d}{}".format(c,acl,color.BLACK)
    else:
        logic_str += " "*6

    acl = read_byte_field(eeprom,offset,'open_acl')
    if acl != 0:
        if get_state(acl):
            c = color.GREEN
        else:
            c = color.RED
        logic_str += " {}O:{:3d}{}".format(c,acl,color.BLACK)
    else:
        logic_str += " "*6

    sep = ""
    action_str = "ACT:"
    acl = read_byte_field(eeprom,offset,'action_acl')
    if acl != 0:
        if get_state(acl):
            c = color.GREEN
        else:
            c = color.RED
        action_str += "{}{}{}".format(c,acl,color.BLACK)
        sep = ","
    item = read_byte_field(eeprom,offset,'action_item')
    if item != 0:
        for i in range(len(inventory)):
            if item == inventory[i][0]:
                c = color.GREEN
                break
        else:
            c = color.BLUE
        action_str += "{}{}I{}{}".format(sep,c,item,color.BLACK)
    state = read_byte_field(eeprom,offset,'action_state')
    if state != 0:
        if get_state(state):
            c = color.GREEN
        else:
            c = color.RED
        action_str += "=>{}{}{}".format(c,state,color.BLACK)
    if len(action_str) > 4:
        logic_str += " {}".format(action_str)

    print("{}{}".format(logic_str,color.NORMAL),end='')
    print()

def print_tree(eeprom,loc,current_loc,offset,inventory):
    print_summary(eeprom,loc,current_loc,offset,inventory)
    children = read_children(eeprom,offset)
    for i in range(len(children)):
        print_tree(eeprom,loc+[i],current_loc,children[i][1],inventory)

##########################################
###   Debugging functions              ###
##########################################

####################
# ^^  to here   ^^ #
####################

