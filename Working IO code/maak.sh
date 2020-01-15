#!/usr/bin/env bash 

if [ "$1" == "clean" ]; then

rm Lib/*.o ISR_IO/*.o Lib/*.d ISR_IO/*.d ISR_IO.elf 

else 

avr-gcc -ILib -Iavr/include -x c -funsigned-char -funsigned-bitfields -DDEBUG -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -mrelax -g2 -Wall -mmcu=attiny1617 -Bdev/attiny1617  -c -std=gnu99 -MD -MP -MF Lib/I2C.d -o Lib/I2C.o Lib/I2C.c 

avr-gcc -ILib -Iavr/include -x c -funsigned-char -funsigned-bitfields -DDEBUG -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -mrelax -g2 -Wall -mmcu=attiny1617 -Bdev/attiny1617  -c -std=gnu99 -MD -MP -MF Lib/main_def.d -o Lib/main_def.o Lib/main_def.c 

avr-gcc -ILib -Iavr/include -x c -funsigned-char -funsigned-bitfields -DDEBUG -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -mrelax -g2 -Wall -Wa,-mgcc-isr -mmcu=attiny1617 -Bdev/attiny1617  -c -std=gnu99 -MD -MP -MF Lib/resources.d -o Lib/resources.o Lib/resources.c 

avr-gcc -ILib -Iavr/include -x c -funsigned-char -funsigned-bitfields -DDEBUG -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -mrelax -g2 -Wall -mmcu=attiny1617 -Bdev/attiny1617  -c -std=gnu99 -MD -MP -MF ISR_IO/main.d -o ISR_IO/main.o ISR_IO/main.c 

fi
