# these definitions are used for importing the JSON data. I also
# use them as constants in game.py, but in the FW source-code, they
# can just be DEFINEs in combination with a define for the length 
# of both of these lists
byte_fields   = ['visible_acl',     # status bit needed to show object or look at it
                 'open_acl',        # status bit needed to open/enter object
                 'action_acl',      # first status bit needed to perform an action on the object
                 'action_mask',     # which actions can be performed on this object
                 'action_item',     # item number that can be used on this object
                 'action_state',    # status bit that will change after the action
                 'item_nr']         # This object has item number X

string_fields = ['name',            # name to show in lists
                 'desc',            # text to show when looking at the object
                 'open_acl_msg',    # text to show when the open_acl prevents opening/entering the object
                 'action_acl_msg',  # text to show when the action_acl prevents an action on the object
                 'action_str1',     # string1 used for an action, like the challenge for a challenge/response action
                 'action_str2',     # string2 used for an action, like the response for a challenge/response action
                 'action_msg']      # text to show when an action on the object was successful

# Action constants, defining the bits in 'action_mask'
A_ENTER =  1   # allowed to enter an object (room, elevator, etc)          NB: ENTER and OPEN are mutually exclusive !!!
A_OPEN  =  2   # allowed to open an object (closet, drawer, etc)           NB: ENTER and OPEN are mutually exclusive !!!
A_LOOK  =  4   # allowed to look at an object from the parent
A_TALK  =  8   # allowed to talk to an object (barman, receptionist, etc)  NB: TALK and USE are mutually exclusive !!!
A_USE   = 16   # allowed to use an object (like computer, knife, etc)      NB: TALK and USE are mutually exclusive !!!

max_object_depth = 32
max_items        = 64
status_bits      = 128


# Keys are md5 hash of 'H@ck3r H0t3l 2020', split in two
xor_key_game   = b'\x74\xbf\xfa\x54\x1c\x96\xb2\x26'
xor_key_teaser = b'\x1e\xeb\xd6\x8b\xc0\xc2\x0a\x61'
flash_size     = 32768
boiler_plate   = b'Hacker Hotel 2020 by badge.team ' # boiler_plate must by 32 bytes long


def read_byte(data,offset):
    return (data[offset+32] ^ xor_key_game[offset % len(xor_key_game)])

def read_range(data,offset,length):
    result = bytearray(0)
    for i in range(length):
        result.append(read_byte(data,offset+i))
    return result

def read_ptr(data,offset):
    return 256 * int(read_byte(data,offset)) + int(read_byte(data,offset+1))


def read_children(data,offset):
    children = []
    nxt    = read_ptr(data,offset)
    offset = read_ptr(data,offset+2)
    if offset >= nxt:
        return []
    while True:
        children.append(offset)
        offset = read_ptr(data,offset)
        if offset >= nxt:
            return children
    

def loc2offset(data,loc):
    offset = 0

    if loc != []:
        for l in range(len(loc)):
            nxt    = read_ptr(data,offset)
            offset = read_ptr(data,offset+2)
            if offset >= nxt:
                return [0xffff,None,None]
            for i in range(loc[l]):
                offset = read_ptr(data,offset)
                #print("{} {} 0x{:04X} 0x{:04x}".format(l,i,offset,nxt))
                if offset >= nxt:
                    return [0xffff,None,None]

    mask     = read_byte_field(data,offset,'action_mask')
    children = read_children(data,offset)
    return [offset,mask,children]
### end of loc2offset()


def name2loc(data,loc,loc_offset,name):
    if loc_offset == 0:
        offset,d1,d2 = loc2offset(data,loc)
    i = 0
    while True:
        offset,d1,d2 = loc2offset(data,loc+[i])
        if offset == 0xffff:
            break
        loc_name = read_string_field(data,offset,'name')
        if name in loc_name.lower():
                break
        i += 1
        # failsafe
        if i >= max_object_depth:
            print("ERROR: out-of-band-error")
            break
    if offset == 0xffff:
        return [None,None]
    return [loc+[i],offset]
### end of name2offset()

    
def read_byte_field(data,offset,field):
    if field in byte_fields:
        if offset != 0xffff:
            return read_byte(data,offset + 4 + byte_fields.index(field))
    return -1


def read_string_field(data,offset,field):
    if field in string_fields:
        if offset != 0xffff:
            offset = offset + 4 + len(byte_fields)
            for i in range(string_fields.index(field)):
                offset = offset + read_byte(data,offset) + 1
            return read_range(data,offset+1,read_byte(data,offset)).decode()
    return "N/A"


#def get_byte_field(data,loc,field):
#    if field in byte_fields:
#        offset,d1,d2 = loc2offset(data,loc)
#        if offset != 0xffff:
#            return read_byte(data,offset + 4 + byte_fields.index(field))
#    return -1
#
#
#def get_string_field(data,loc,field,offset=0):
#    if field in string_fields:
#        offset,d1,d2 = loc2offset(data,loc)
#        if offset != 0xffff:
#            offset = offset + 4 + len(byte_fields)
#            for i in range(string_fields.index(field)):
#                offset = offset + read_byte(data,offset) + 1
#
#            return read_range(data,offset+1,read_byte(data,offset)).decode()
#    return "N/A"


def invalid():
    print("Sorry, that is not possible")


def show_help(data):
    offset = read_ptr(data,0)
    length = read_ptr(data,offset)
    print(read_range(data,offset+2,length).decode())


###################################
###   State related functions   ###
###################################

# status bits that will be used for the game logic
# bit 0 is not used for code/memory efficiency, status of bit 0 is considered True
game_state = [0 for i in range(status_bits//8)]
inventory  = []

# get_state() returns True if the status bit <num> is set to 1.
def get_state(num):
    if num > 0 and num < status_bits:
        byte = num >> 3
        bit = 1 << (num & 7)
        return((game_state[byte]&bit) != 0)
    else:
        return True

# set_state() sets status bit <num> to 1
def set_state(num):
    if num > 0 and num < status_bits:
        byte = num >> 3
        bit = 1 << (num & 7)
        game_state[byte] |= bit
    return

# clear_state() sets status bit <num> to 0
def clear_state(num):
    if num > 0 and num < status_bits:
        byte = num >> 3
        bit = 1 << (num & 7)
        game_state[byte] &= (255 - bit)
    return

# toggle_state() toggles status bit <num>
def toggle_state(num):
    if num > 0 and num < status_bits:
        byte = num >> 3
        bit = 1 << (num & 7)
        game_state[byte] ^= bit
    return


def check_open_permission(data,offset):
    state_num = read_byte_field(data,offset,'open_acl')
    if get_state(state_num) == True:
        return ""
    else:
        return read_string_field(data,offset,'open_acl_msg')
### end of check_open_permission()


def check_action_permission(data,offset):
    state_num = read_byte_field(data,offset,'action_acl')
    if get_state(state_num) == True:
        return ""
    else:
        return read_string_field(data,offset,'action_acl_msg')
### end of check_action_permission()


def object_visible(data,offset):
    state_num = read_byte_field(data,offset,'visible_acl')
    return get_state(state_num)
### end of checkpermission()

##########################################
###   End of State related functions   ###
##########################################


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

def print_summary(data,loc,current_loc,offset):
    c = color.NORMAL

    if loc == current_loc:
        print(color.BOLD,end='')
    print("{:20s} ".format(str(loc)),end='')

    tree_str  = ""
    tree_str += "{}+- ".format("|  "*len(loc))
    item = read_byte_field(data,offset,'item_nr')
    if item != 0:
        tree_str += color.BLUE + "({})".format(item)
    else:
        tree_str += color.BLACK
    print("{:50s} ".format(tree_str + read_string_field(data,offset,'name')),end='')

    logic_str = ""
    mask = read_byte_field(data,offset,'action_mask')
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

    acl = read_byte_field(data,offset,'visible_acl')
    if acl != 0:
        if get_state(acl):
            c = color.GREEN
        else:
            c = color.RED
        logic_str += " {}V:{:3d}{}".format(c,acl,color.BLACK)
    else:
        logic_str += " "*6

    acl = read_byte_field(data,offset,'open_acl')
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
    acl = read_byte_field(data,offset,'action_acl')
    if acl != 0:
        if get_state(acl):
            c = color.GREEN
        else:
            c = color.RED
        action_str += "{}".format(c,acl,color.BLACK)
        sep = ","
    item = read_byte_field(data,offset,'action_item')
    if item != 0:
        for i in range(len(inventory)):
            if item == inventory[i][0]:
                c = color.GREEN
                break
        else:
            c = color.BLUE
        action_str += "{}{}I{}{}".format(sep,c,item,color.BLACK)
    state = read_byte_field(data,offset,'action_state')
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

def print_tree(data,loc,current_loc,offset):
    print_summary(data,loc,current_loc,offset)
    children = read_children(data,offset)
    for i in range(len(children)):
        print_tree(data,loc+[i],current_loc,children[i])

##########################################
###   Debugging functions              ###
##########################################

