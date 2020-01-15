#!/usr/local/env bash 

avr-gcc -ILib -Iavr/include -x c -funsigned-char -funsigned-bitfields -DDEBUG  -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -mrelax -g2 -Wall -mmcu=attiny1617 -Bdev/attiny1617  -c -std=gnu99 -MD -MP -MF -mmcu=attiny1617 -o ISR_IO/main.o ISR_IO/main.c 

avr-gcc -ILib -Iavr/include -x c -funsigned-char -funsigned-bitfields -DDEBUG  -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -mrelax -g2 -Wall -mmcu=attiny1617 -Bdev/attiny1617  -c -std=gnu99 -MD -MP -MF -mmcu=attiny1617 -o Lib/I2C.o Lib/I2C.c 

avr-gcc -ILib -Iavr/include -x c -funsigned-char -funsigned-bitfields -DDEBUG  -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -mrelax -g2 -Wall -mmcu=attiny1617 -Bdev/attiny1617  -c -std=gnu99 -MD -MP -MF -mmcu=attiny1617 -o Lib/main_def.o Lib/main_def.c 

avr-gcc -ILib -Iavr/include -x c -funsigned-char -funsigned-bitfields -DDEBUG  -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -mrelax -g2 -Wall -Wa,-mgcc-isr -mmcu=attiny1617 -Bdev/attiny1617  -c -std=gnu99 -MD -MP -MF -mmcu=attiny1617 -o Lib/resources.o Lib/resources.c 


