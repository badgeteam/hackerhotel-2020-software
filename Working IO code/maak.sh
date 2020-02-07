#!/usr/bin/env bash
#    _____          __           _____.__.__
#   /     \ _____  |  | __ _____/ ____\__|  |   ____   ______
#  /  \ /  \\__  \ |  |/ // __ \   __\|  |  | _/ __ \ /  ___/
# /    Y    \/ __ \|    <\  ___/|  |  |  |  |_\  ___/ \___ \
# \____|__  (____  /__|_ \\___  >__|  |__|____/\___  >____  >
#         \/     \/     \/    \/                   \/     \/
#    _____                  .____
#   /  _  \_______   ____   |    |   _____    _____   ____
#  /  /_\  \_  __ \_/ __ \  |    |   \__  \  /     \_/ __ \
# /    |    \  | \/\  ___/  |    |___ / __ \|  Y Y  \  ___/
# \____|__  /__|    \___  > |_______ (____  /__|_|  /\___  >
#         \/            \/          \/    \/      \/     \/

TARGETS="Lib/I2C Lib/main_def Lib/resources Lib/text_adv Lib/simon Lib/maze Lib/lanyard Lib/friends ISR_IO/main"
OBJECTS=""

if [ "$1" == "schoon" ]; then
  rm Lib/*.o ISR_IO/*.o Lib/*.d ISR_IO/*.d ISR_IO.eep  ISR_IO.elf  ISR_IO.hex  ISR_IO.map ISR_IO.lss ISR_IO.srec
else
  for target in $TARGETS
  do
    avr-gcc -ILib -Iavr/include -Os \
            -x c -funsigned-char -funsigned-bitfields -DDEBUG -Os \
            -ffunction-sections -fdata-sections -fpack-struct -fshort-enums\
            -mrelax -g2 -Wall -Wa,-mgcc-isr\
            -mmcu=attiny1617 -Bdev/attiny1617\
            -c -std=gnu99 -MD -MP\
            -MF $target.d -MT $target.d -MT $target.o -o $target.o $target.c

    OBJECTS="$OBJECTS $target.o"
  done

  avr-gcc -o ISR_IO.elf $OBJECTS -Wl,-Map="ISR_IO.map"\
          -Wl,--start-group -Wl,-lm -Wl,--end-group -Wl,-Lavrxmega3 -Wl,-LLib -Wl,-Lavr/lib -Wl,--gc-sections\
          -mrelax -mmcu=attiny1617 -B dev/attiny1617

  avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures  "ISR_IO.elf" "ISR_IO.hex"

  avr-objcopy -j .eeprom  --set-section-flags=.eeprom=alloc,load\
              --change-section-lma .eeprom=0\
              --no-change-warnings\
              -O ihex "ISR_IO.elf" "ISR_IO.eep"

  avr-objdump -h -S "ISR_IO.elf" > "ISR_IO.lss"

  avr-objcopy -O srec -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures "ISR_IO.elf" "ISR_IO.srec"

  avr-size "ISR_IO.elf"
  if [ "$1" == "flits" ]; then
    echo Flashing
	  python3 ~/Projects/pyupdi/pyupdi.py -d tiny1617 -c /dev/tty.SLAB_USBtoUART -f ISR_IO.hex
  fi
fi
