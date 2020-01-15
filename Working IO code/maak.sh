#!/usr/bin/env bash 

TARGETS="Lib/I2C Lib/main_def Lib/resources ISR_IO/main"
OBJECTS=""

if [ "$1" == "clean" ]; then
  rm Lib/*.o ISR_IO/*.o Lib/*.d ISR_IO/*.d ISR_IO.elf 
else
  for target in $TARGETS 
  do
    avr-gcc -ILib -Iavr/include -x c -funsigned-char -funsigned-bitfields -DDEBUG -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -mrelax -g2 -Wall -Wa,-mgcc-isr -mmcu=attiny1617 -Bdev/attiny1617  -c -std=gnu99 -MD -MP -MF $target.d -MT $target.d -MT $target.o -o $target.o $target.c 
    OBJECTS="$OBJECTS $target.o"
  done 
  avr-gcc -o ISR_IO.elf $OBJECTS -Wl,-Map="ISR_IO.map" -Wl,--start-group -Wl,-lm -Wl,--end-group -Wl,-LLib -Wl,--gc-sections -mrelax -mmcu=attiny1617 -B dev/attiny1617 
fi
