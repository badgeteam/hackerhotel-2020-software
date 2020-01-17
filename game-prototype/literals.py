from lit_offsets import *
import common

literals = ['Welcome to the "Escape from Hacker Hotel!!!" challenge.\n\
\n\
You are locked in the hotel, just like servants of the pharaohs\n\
were locked in the Pyramids with their masters.\n\
You will need to hack yourself a way out of the hotel before\n\
the end of the weekend (when the hotel will be closed up\n\
forever, WHOAHAHAHAHAHAHA)!\n\
\n\
h or ?      show this help\n\
q           quit the game for now\n\
x           leave a room or enter the elevator\n\
e <room>    enter the given room\n\
l           look around the room\n\
l <o>       look at object <o>\n\
o <o>       open object <o>\n\
p <i>       pickup item <i> (you can only carry 2 items at a time)\n\
d <i>       drop item <i> (you can only carry 2 items at a time)\n\
i           list items you are carrying\n\
t <p>       talk to "person" <p>\n\
u <o>       use object <o>\n\
u <i> <o>   use item <i> on object <o>\n\
\n\
Every object has a single letter identifier enclosed in brackets. So \n\
when you see "The [R]eception" in the game, you can use "e r" to enter\n\
the Reception.',
'string2',
'string3'
]

def s(eeprom,lit):
    if lit in lit_offsets:
        offset,length = lit_offsets[lit]
        return common.read_range(eeprom,offset,length,0)
    else:
        print("ERROR: {} not listed in literals.py".format(lit))
