/*
 * resources.c
 *
 * Created: 06/01/2020 18:26:44
 *  Author: Badge.team
 *       _______ '______   _____ '_______  __ . __  _______ '______ ______   _____
 *  . ' /____  / /_____/ /_____/ / ____ / / /' / / /____  / /_____//_____/ /_____/
 *   * _____/ / _____ + ____ '. / / +/ / / / ./ / _____/ / ___ '. _____   ____ +
 *  ' / .  __/ / ___/  .\__ \. / /'./ / / /+ / / / .  __/ / / .  / ___/   \__ \ `.
 *   / / \ \  / /___  ____) / / /__/ / / /__/ / / / \ \  / /___ / /___  ____) / .
 *  /_/ ' \_\/_____/ /_____/ /______/ /______/ /_/ . \_\/_____//_____/ /_____/   .
 */

#include <resources.h>

volatile uint16_t tmp16bit;    
volatile uint8_t mask[8] = {0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};
static uint16_t lfsrSeed;

uint8_t HeartCount = 0;
uint8_t LedCount = 0;

void Setup(){
    cli();

    //Set up clock at 10MHz
    CCP = CCP_IOREG_gc;
    CLKCTRL_MCLKCTRLB = 0x01;

    //Set I/O direction registers
    PORTA_DIR = 0b01001010;
    PORTB_DIR = 0b01111100;
    PORTC_DIR = 0b00111111;

#ifndef PURIST_BADGE
    //Invert some pins for correcting LED reversal error (with bodge wire mod or PFET mod)
    PORTC_PIN3CTRL |= 0x80;
    PORTC_PIN4CTRL |= 0x80;
    PORTC_PIN5CTRL |= 0x80;
    PORTB_PIN3CTRL |= 0x80;
    PORTB_PIN4CTRL |= 0x80;
    PORTB_PIN5CTRL |= 0x80;
#endif

#ifdef PFET_MOD //(benadski has the only one left at the moment of writing, but you can mod the badge too!)
    //PFET mod drive pins
    PORTC_PIN0CTRL |= 0x80;
    PORTC_PIN1CTRL |= 0x80;
    PORTC_PIN2CTRL |= 0x80;
    PORTB_PIN2CTRL |= 0x80;
    PORTB_PIN6CTRL |= 0x80;
#endif

    //UART (Alternative pins PA1=TxD, PA2=RxD, baudrate 115200, 8n1, RX and Buffer empty interrupts on)
    PORTMUX_CTRLB = 0x01;
    PORTA_OUTSET = 0x02;
    USART0_BAUD = (uint16_t)USART0_BAUD_RATE(115200);
    USART0_CTRLA = 0xA0; //Interrupts on
    USART0_CTRLB = 0xC0; //RX and TX on
    USART0_CTRLC = 0x03; //8 bits data, no parity, 1 stop bit
     
    //GPIO registers 0..3 can be used for global variables used in ISR routines (ASM capability: IN, OUT, SBI, CBI, SBIS, SBIC)
    L_COL = 0;     //Used in LED array to hold currently driven column value
    RXCNT = 0;     //Used as UART receive counter
    GPIOR3 = 0;    //

    //Init TCA (split mode, f_CLK/16, PWM on 6 alternative pins, underflow int enabled, synced and started)
    TCA0_SPLIT_CTRLD = 1;
    TCA0_SPLIT_CTRLA = (0x4)<<1;
    PORTMUX_CTRLC = 0x3F;
    TCA0_SPLIT_CTRLB = 0x77;
    TCA0_SPLIT_INTCTRL = 0x1;
    TCA0_SPLIT_CTRLESET = ((0x2)<<2)|0x3;
    TCA0_SPLIT_CTRLA |= 1;
     
    //Init TCB0 (compare value sets serial Tx pause between characters, uses TCA0 prescaler, enabled, interrupt disabled)
    TCB0_CTRLA = ((0x02)<<1)|0x01;
    TCB0_CTRLB = 0;
    TCB0_CCMP = 0x01FF;
    TCB0_INTCTRL = 0x00;     
     
    //Init TCB1 (compare value sets audio sample rate (0x038B = 11025sps), enabled, interrupt enabled)
    TCB1_CTRLA = 0x01;
    TCB1_CTRLB = 0;
    TCB1_CCMP = 0x038B;
    TCB1_INTCTRL = 0x01;

    //Init I2C (About 400kHz, but EEPROM might work on FM+ as well (MBAUD minimum is 3, so fmax would be 625kHz))
    TWI0.MBAUD = 8;		    // Rate = 10MHz/(2*MBAUD+10) => 385kHz, close enough;
    TWI0.MCTRLA = 0xC3;     // SMEN (0x02) also enabled
    TWI0.MSTATUS |= 0x01;	// Put bus in idle
    TWI0.MSTATUS |= 0xC4;	// Clear errors if any

    //VREF (DAC, ADC's)
    VREF_CTRLA   = 0x12;    //0x22 for audio in/out (2.5V), 0x12 for temperature in, audio out (1.1V/2.5V)
    VREF_CTRLC   = 0x20;    //ADC1 reference at 2.5V
    VREF_CTRLB   = 0x01;    //DAC0 ref forced enabled

    //Init ADC0 (audio in and internal temperature)
    ADC0_CTRLA   = 0x02;    //10 bit resolution, free running
    ADC0_CTRLC   = 0x44;    //Reduced sample capacitor, internal reference, clock/32 => 24038sps
    ADC0_MUXPOS  = 0x1E;    //Audio in: AIN7 at (0x07), Temperature: Internal sensor at (0x1E)
    ADC0_INTCTRL = 0x01;    //Result ready interrupt enabled
    ADC0_CTRLA  |= 0x01;    //ADC0 enabled
    ADC0_COMMAND = 0x01;    //Start first conversion, after that it will run periodically.

    //Init ADC1 (buttons and sensors: CH0=Photo transistor, CH1=Hall, CH4=Buttons)
    ADC1_CTRLA   = 0;
    ADC1_CTRLB   = 0x02;    //4 results automatically accumulated
    ADC1_CTRLC   = 0x52;    //Reduced sample capacitor, VDD as reference, clock/8
    ADC1_SAMPCTRL= 0x04;    //Extend sampling time with 4 cycles (6 total)
    ADC1_MUXPOS  = 0x04;    //Start with Buttons, for sensors, activate sensor power first.
    ADC1_INTCTRL = 0x01;    //Result ready interrupt enabled
    ADC1_CTRLA  |= 0x01;    //ADC0 enabled
    ADC1_COMMAND = 0x01;    //Start single conversion

    //Init DAC0
    DAC0_CTRLA = 0x40;     //Enable output buffer (connect to pin)
    DAC0_DATA = 0x80;      //Data value at half point (about 1.25V)
    DAC0_CTRLA |= 0x01;    //Enable DAC
     
    //PIT and RTC interrupts
    while(RTC_STATUS & RTC_CTRLABUSY_bm);
    RTC_CTRLA          = (0x6)<<3;                         //Prescaler: /64
    RTC_CLKSEL         = 0;                                //Clock source: internal 32768Hz clock
    while(RTC_STATUS & RTC_PERBUSY_bm);
    RTC_PER            = 512*60;                           //60 second period (for clocking how long the device has been running?)
    tmp16bit = (RTC_CNT + BTN_TMR);
    if (tmp16bit > RTC_PER) tmp16bit -= RTC_PER;
    while(RTC_STATUS & RTC_CMPBUSY_bm);
    RTC_CMP            = tmp16bit;                         //Button timing
    RTC_INTCTRL        = 0x03;                             //RTC overflow interrupt enabled
    RTC_PITINTCTRL     = 0x01;                             //PIT interrupt enabled
    RTC_PITCTRLA       = (0x1)<<3;                         //Interrupt fires after 4 RTC clock cycles Rate=(RTCCLK/(PRESC*PERIOD)) 32768/256=128Hz
    while(RTC_STATUS & RTC_CTRLABUSY_bm);      
    RTC_CTRLA          |= 0x01;                            //Enable RTC
    RTC_PITCTRLA       |= 0x01;                            //Enable PIT
     
    //Other inits
    serRx[0] = 0;           // Empty Rx buffer (first char is enough)
    serTxAddr = &serRx[0];  // Point to first address of the Rx buffer
    adc0Chg = 255;          // Wait 10ms before taking samples from ADC0 (jack input temperature sensor)

    sei();
}

// TCA0 is used for driving the LED matrix at 488Hz (* 5 columns = 2440Hz). The lower 8 bit underflow interrupt is used to load new values and shift between columns.
ISR(TCA0_LUNF_vect){
    //Turn off all columns
    PORTC_OUTCLR = 0x07;
    PORTB_OUTCLR = 0x44;

    //Write all of the compare registers of TCA0 with PWM values in array (rows)
    asm(
    "ldi r25, 6        \n"
    "ld r24, %a[arr]+  \n"
    "st %a[wo]+, r24   \n"
    "dec r25           \n"
    "brne .-8          \n"
    :: [wo] "e" (&TCA0_SPLIT_LCMP0), [arr] "e" (&iLED[(L_COL<<3)]) : "r25", "r24", "cc", "memory");

    TCA0_SPLIT_CTRLESET = ((0x2)<<2)|0x3; //Sync timers: Moved this below the PWM value loading to fix the data shifting in asm above.

    //Turn on the right column
    if (L_COL<3) {
        asm(
        "in r24, %[io0]  \n"
        "ldi r25, 1      \n"
        "rjmp .+2        \n"
        "add r25, r25    \n"
        "dec r24         \n"
        "brpl .-6        \n"
        "out %[vpc], r25 \n"
        "in r24, %[io0]  \n"
        "inc r24         \n"
        "out %[io0], r24 \n"
        :: [io0] "I" (&L_COL), [vpc] "I" (&VPORTC_OUT) : "r24", "r25", "cc");
        } else {
        asm(
        "in r24, %[io0]  \n"
        "cpi r24, 3      \n"
        "brne .+8        \n"
        "sbi %[vpb], 2   \n"    
        "inc r24         \n"
        "out %[io0], r24 \n"
        "rjmp .+6        \n"
        "sbi %[vpb], 6   \n"   
        "clr r24         \n"
        "out %[io0], r24 \n"
        :: [io0] "I" (&L_COL), [vpb] "I" (&VPORTB_OUT) : "r24", "cc");
    }
    
    TCA0_SPLIT_INTFLAGS = 0xFF;
}

// TCB0 is used for (slowly) sending serial characters. A 0x00 character code is needed to terminate sending or it will leak memory to serial. It's a feature, honest! 
ISR(TCB0_INT_vect){
    if (*serTxAddr) {
        USART0_TXDATAL = *serTxAddr;
        ++serTxAddr;
        USART0_CTRLA |= USART_DREIE_bm;
    } else {
        serTxDone = 1;
        TCB0_INTCTRL = 0x00;
    }
    TCB0_INTFLAGS = TCB_CAPT_bm;
}

// TCB1 is used for audio generation. Keeps playing "data" until 0 is reached, can leak memory too. Audio sample data can contain 0x01 to 0xFF, centered around 0x80
ISR(TCB1_INT_vect){
    int16_t volCtrl;
    if (*auSmpAddr == 0) auSmpAddr = auRepAddr;
    if (*auSmpAddr) {
        volCtrl = ((((int16_t)(*auSmpAddr) - 0x7f) * auVolume) >> 8) + 0x80;
        DAC0_DATA = (uint8_t)(volCtrl);
        ++auSmpAddr;
    } else {
        auVolume = 0xff;
        DAC0_DATA = 0x80;
    }
    TCB1_INTFLAGS = TCB_CAPT_bm;
}

//I2C Interrupt (DOES work now! Tidied up.)
ISR(TWI0_TWIM_vect){

    //The initial numbers of register/data bytes has to be one higher than written/read for switching the TWI0_MADDR to read and to send the NACK/STOP sequence.
    if (bytesLeftI2C) --bytesLeftI2C;
   
    //Reading is done here
    if (TWI0_MSTATUS & 0x80) {                 
        if (bytesLeftI2C) {
            *addrDataI2C++ = TWI0_MDATA; 
            TWI0.MCTRLB = 0;
        } else TWI0.MCTRLB = 7;
            
    //Write part at (7 bit I2C address)<<1 (data: 2 byte internal address of external EEPROM)
    } else {

        //Error, will never get here unless there's some hacker poking around on the PCB, slim chance of happening at HackerHotel... ;-)
        if (TWI0_MSTATUS & 0x0C) {
            bytesLeftI2C = 0;   //No neat exit, we'll see what happens next...

        //Good, good!
        } else { 
            if (bytesLeftI2C == 2) {
                TWI0_MDATA = addrDataI2C[0];
            } else if (bytesLeftI2C == 1) {
                TWI0_MDATA = addrDataI2C[1];
            } else {
                TWI0_MADDR |= 1; //Read: (7 bit I2C address)<<1 + 1
            }
        }
    }
}

// Reads up to RXLEN characters until LF is found, LF sets the serRxDone flag and writes 0 instead of LF.
ISR(USART0_RXC_vect){
    if (serRxDone == 0){
        serRx[RXCNT] = USART0.RXDATAL;
        USART0_TXDATAL = serRx[RXCNT];
        if ((serRx[RXCNT] == 0x0A)||(serRx[RXCNT] == 0x0D)){
            serRx[RXCNT] = 0;
            serRxDone = 1;
        } else if (((serRx[RXCNT] == 0x08)||(serRx[RXCNT] == 0x7F))&&(RXCNT > 0)) {
            serRx[RXCNT] = 0;
            --RXCNT;
        } else if (RXCNT < (RXLEN-1)) ++RXCNT;
    }
    USART0_STATUS = USART_RXCIF_bm;
};

// Trigger new data write and turn off DRE interrupt.
ISR(USART0_DRE_vect){
    TCB0_CNT = 0;
    TCB0_INTCTRL = 0x01;
    USART0_CTRLA &= ~(USART_DREIE_bm);
};

// ADC used for audio input and temperature sensor.
ISR(ADC0_RESRDY_vect){
    //If just switched reference, discard first few samples
    if (adc0Chg == 0){
        if (ADC0_MUXPOS == 0x1E) adcTemp = ADC0_RES; else auIn=ADC0_RESL;
    } else --adc0Chg;
    ADC0_INTFLAGS = ADC_RESRDY_bm;
}

// Reads out sensors at 8Hz and buttons at 42Hz (timed by PIT), power for the sensors is on 1/8th of the time to save power.
ISR(ADC1_RESRDY_vect){
    if (ADC1_MUXPOS == 0) {
        adcPhot = ADC1_RES;
        ADC1_MUXPOS = 0x01;             //Select Hall sensor
    } else if (ADC1_MUXPOS == 1){
        PORTA_OUTCLR = 0b00001000;      //Turn off sensor power
        adcHall = ADC1_RES;
        ADC1_MUXPOS = 0x04;             //select buttons
    } else {
        adcBtns = ADC1_RES;
        if (A1CNT == 15){
            ADC1_MUXPOS = 0x00;         //Select photo transistor
            PORTA_OUTSET = 0b00001000;  //Turn on sensor power
        }
    }

    A1CNT=(A1CNT+1)%16;        
    ADC1_INTFLAGS = ADC_RESRDY_bm;
}

// RTC compare interrupt, triggers at 512/BTN_TMR rate, also RTC overflow interrupt, triggers once a minute
ISR(RTC_CNT_vect) {
    if (RTC_INTFLAGS & RTC_CMP_bm){
        if (buttonMark<0xff) ++buttonMark;  // For button timing purposes
        tmp16bit = (RTC_CNT + BTN_TMR);
        while (tmp16bit > RTC_PER) tmp16bit -= RTC_PER;
        while(RTC_STATUS & RTC_CMPBUSY_bm);
        RTC_CMP = tmp16bit;                 // Button timing: next interrupt set
        RTC_INTFLAGS = RTC_CMP_bm;		    // clear interrupt flag
    } else {
        ++minuteMark;                       // For very slow timing purposes, overflows to 0
        RTC_INTFLAGS = RTC_OVF_bm;		    // clear interrupt flag
    }
}

// PIT interrupt (timing of ADC1: sensor values @ 8192 sps)
ISR(RTC_PIT_vect) {						// PIT interrupt handling code
    ADC1_COMMAND = 0x01;
    fastTicker++;
    RTC_PITINTFLAGS = RTC_PI_bm;		// clear interrupt flag
}

/*
    ---------- END OF INTERRUPT ROUTINES ----------
*/

// I2C read data from external EEPROM
uint8_t I2C_read_bytes(uint8_t slave_addr, uint8_t *reg_ptr, uint8_t reg_len, uint8_t *dat_ptr, uint8_t dat_len){

    //Error, I2C not available somehow...
    if ((TWI0_MSTATUS & 0x03) == 0) return 1;

    addrDataI2C = reg_ptr;
    TWI0_MADDR = (EE_I2C_ADDR<<1);          //Write stupid shifted! I2C address, looking over this for hours...
    bytesLeftI2C = reg_len+1;               //Yeah, well... it works now.
    while (bytesLeftI2C) ;                  //Just a very short pause, waiting for address to be written and EEPROM set to read.

    addrDataI2C = dat_ptr;                  //Data!
    bytesLeftI2C = dat_len+1;               //Gimme, gimme!
    while (bytesLeftI2C) ;                  //Hurry up!!!

    //Yesss! Now be quiet!
    return 0;
}

// Read bytes from EEPROM
void EERead(uint8_t eeAddr, uint8_t *eeValues, uint8_t size) {
    while(NVMCTRL_STATUS & NVMCTRL_EEBUSY_bm);              // Wait until any write operation has finished

    while(size){
        *eeValues++ = *(uint8_t *)(EEPROM_START+eeAddr++);  // Read data from buffer
        --size;
    }
}

//Write bytes to the EEPROM, if address exceeds EEPROM space data wraps around
uint8_t EEWrite(uint8_t eeAddr, uint8_t *eeValues, uint8_t size)
{
    uint8_t lastByteOfPage;
    while(size){
        lastByteOfPage = 0;
        while(NVMCTRL_STATUS & NVMCTRL_EEBUSY_bm);              // Wait until any write operation has finished
        CCP = CCP_SPM_gc;                                       // Gain access to NVMCTRL_CTRLA
        NVMCTRL_CTRLA = NVMCTRL_CMD_PAGEBUFCLR_gc;              // Clear page buffer
        while((size) && (lastByteOfPage == 0)){
            if ((eeAddr % EEPROM_PAGE_SIZE) == (EEPROM_PAGE_SIZE-1)) lastByteOfPage = 1;
            *(uint8_t *)(EEPROM_START+eeAddr++) = *eeValues++;  // Write data to buffer
            --size;
        }
        CCP = CCP_SPM_gc;
        NVMCTRL_CTRLA = NVMCTRL_CMD_PAGEERASEWRITE_gc;          // Erase old data and write new data to EEPROM
        if (NVMCTRL_STATUS & NVMCTRL_WRERROR_bm) return 1;
    }
    return 0;
}

//Decrypts data read from I2C EEPROM, max 255 bytes at a time
void DecryptData(uint16_t offset, uint8_t length, uint8_t type, uint8_t *data){
    while(length){
        *data ^= xor_key[type][(uint8_t)(offset%KEY_LENGTH)];
        ++data;
        ++offset;
        --length;
    }
}

//Game data: Read a number of bytes and decrypt
uint8_t ExtEERead(uint16_t offset, uint8_t length, uint8_t type, uint8_t *data){
    offset &=EXT_EE_MAX;
    uint8_t reg[2] = {(uint8_t)(offset>>8), (uint8_t)(offset&0xff)};
    uint8_t error = (I2C_read_bytes(EE_I2C_ADDR, &reg[0], 2, data, length));
    if (error) return error;
    DecryptData(offset, length, type, data);
    return 0;
}

// Sends a set of characters to the serial port, stops only when character value 0 is reached.
uint8_t SerSend(unsigned char *addr){
    if (serTxDone){
        serTxAddr = addr;
        serTxDone = 0;
        TCB0_INTCTRL = 0x01;
        return 0;
    } else return 1;    //Error: Still sending data
};

// Set serial character output speed, 255-0 (0.8 to 100ms delay between characters)
void SerSpeed(uint8_t serSpd){
    if (serSpd<1) serSpd = 1;
    TCB0_CCMP = ((uint16_t)(0xFF-serSpd)<<8) + 0xFF;
};

// Select temperature sensor
void SelectTSens(){
     adc0Chg = 255;
     VREF_CTRLA   = 0x12;    //0x22 for audio in/out (2.5V), 0x12 for temperature in, audio out (1.1V/2.5V)
     ADC0_CTRLA   &= ~(ADC_RESSEL_bm);
     ADC0_MUXPOS  = 0x1E;    //Audio in: AIN7 at (0x07), Temperature: Internal sensor at (0x1E)
};

// Select "audio" input (0-2.5V)
void SelectAuIn(){
     adc0Chg = 255;
     VREF_CTRLA   = 0x22;    //0x22 for audio in/out (2.5V), 0x12 for temperature in, audio out (1.1V/2.5V)
     ADC0_CTRLA   |= ADC_RESSEL_bm;
     ADC0_MUXPOS  = 0x07;    //Audio in: AIN7 at (0x07), Temperature: Internal sensor at (0x1E)
};

// Returns button combination (4LSB) and number of consecutive times this combination is detected. First read should always be ignored! 
uint8_t CheckButtons(){
    static uint8_t previousValue = 0xFF;
    uint8_t bADC = (uint8_t)(adcBtns>>4);
    uint8_t button = 0xFF;     //FF = released or error

    switch(bADC){

        case 48 ... 56:          //52: Bottom left (+/-4)
        button = 2;
        break;

        case 79 ... 95:          //87: Top left (+/-8)
        button = 3;
        break;

        case 117 ... 141:        //129: Top right (+/- 12)
        button = 1;
        break;

        case 158 ... 190:        //174: Bottom right (+/- 16)
        button = 0;
        break;
    }

    if (((previousValue+1)== 0) || (button != previousValue)) {
        previousValue = button;
        return 0xFF;
    } else return button;
}

//Linear feedback shift register, for cheap pseudo random
uint8_t lfsr(){
    lfsrSeed ^= (lfsrSeed << 13);
    lfsrSeed ^= (lfsrSeed >> 9);
    lfsrSeed ^= (lfsrSeed << 7);
    return (lfsrSeed & 0xff);
}

//Variate the speed of playing audio a bit
void floatSpeed(uint8_t bits, uint16_t min, uint16_t max){
    uint16_t val = TCB1_CCMP;
    bits = mask[(bits-1)&0x07];
    val += (lfsr()&bits);
    val -= (lfsr()&bits);
    if (val > max) val = max;    
    if (val < min) val = min;  
    TCB1_CCMP = val;
}

//About the same as above, but for a sample
uint8_t floatAround(uint8_t sample, uint8_t bits, uint8_t min, uint8_t max){
    bits = mask[(bits-1)&0x07];
    sample += lfsr()&bits;
    sample -= lfsr()&bits;
    if (max){
        if (sample > max) sample = max;
        if (sample < min) sample = min;
    }
    if (min & (sample < min)) sample = min;

    return sample;
}

//Load game state from internal EEPROM (seed lfsr, read game state, check the badge identity and load inventory) 
void LoadGameState() {

    EERead(0, &gameState[0], BOOTCHK);   //Load game status bits from EEPROM

    uint8_t idSet = 0;
    for (uint8_t x=0; x<4; ++x){
        idSet += ReadStatusBit(110+x);
    }

    //Check if badge is reset(0 = cheated!) or new(3) or error(2)
    if (idSet != 1) {
        Reset();
    } else getID();

    inventory[0] = (gameState[INVADDR]<<8|gameState[INVADDR+1]);
    inventory[1] = (gameState[INVADDR+2]<<8|gameState[INVADDR+3]);
}

//Save changed data to EEPROM
uint8_t SaveGameState(){
    uint8_t gameCheck[BOOTCHK];

    //Read all data up to the boot/check result address from EEPROM
    EERead(0, &gameCheck[0], BOOTCHK);

    //Check game status bits
    for (uint8_t x=0; x<STATLEN; ++x){
        if (gameState[x] != gameCheck[x]){
            if (EEWrite(x, &gameState[x], 1)) return 1;
        }
    }

    //Check the inventory too
    gameState[INVADDR] = (uint8_t)(inventory[0]>>8);
    gameState[INVADDR+1] = (uint8_t)(inventory[0]&0xff);
    gameState[INVADDR+2] = (uint8_t)(inventory[1]>>8);
    gameState[INVADDR+3] = (uint8_t)(inventory[1]&0xff);
    if (inventory[0] != (gameCheck[INVADDR]<<8|gameCheck[INVADDR+1])) {
        if (EEWrite(INVADDR, &gameState[INVADDR], 2)) return 1;
    }
    if (inventory[1] != (gameCheck[INVADDR+2]<<8|gameCheck[INVADDR+3])) {
        if (EEWrite(INVADDR+2, &gameState[INVADDR+2], 2)) return 1;
    }

    return 0;
}

//Status bit checking
uint8_t ReadStatusBit(uint8_t number){
    number &= 0x7f;
    if (gameState[number>>3] & (1<<(number&7))) return 1; else return 0;
}

//Update game state: num -> vBBBBbbb v=value(0 is set!), BBBB=Byte number, bbb=bit number
void UpdateState(uint8_t num){
    uint8_t clearBit = num & 0x80;
    num &= 0x7f;
    
    if (num) {
        if (clearBit) {
            gameState[num>>3] &= ~(1<<(num&7));
        } else {
            gameState[num>>3] |= 1<<(num&7);
        }
    }
}

//Checks if state of bit BBBBbbb matches with v (inverted) bit
uint8_t CheckState(uint8_t num){
    uint8_t bitSet = 0;
    if (ReadStatusBit(num & 0x7f)){
        bitSet = 1;
    }
    if (((num & 0x80)>0)^(bitSet>0)){
        return 1;
    }
    return 0;
}

//Give out a number 0..3, calculated using serial number fields
uint8_t getID(){
    uint8_t id = 0;
    uint8_t *serNum;
    serNum = (uint8_t*)&SIGROW_SERNUM0;

    for (uint8_t x=0; x<10; ++x){
        id += *serNum;
        ++serNum;
    }
    id %= 4;
    whoami = id+1;
    return id;
}

//For resetting all game progress, temperature calibration and rom test data.
void WipeAfterBoot(uint8_t full){
    //Reset cheat data by resetting the EEPROM bytes
    uint8_t empty=0xff;
    if (full) {
        for (uint8_t x=0; x<CHEATS+MAX_CHEATS; ++x){
            EEWrite(+x, &empty, 1);
        }
    }

    //Reset game data by wiping the UUID bit
    UpdateState(128+109+whoami);
}

// Reset game progress (all zeros) and set some bits (depending on chip serial number)
void Reset(){
    for (uint8_t x=0; x<sizeof(gameState); ++x){
        gameState[x] = 0;
    }

    //ID is calculated 
    uint8_t id = getID();

    if (id == 0) UpdateState(110);
    else if (id == 1) UpdateState(111);
    else if (id == 2) UpdateState(112);
    else if (id == 3) UpdateState(113);
    UpdateState(100+id);
}

//Sets specific game bits after the badge is heated for one and two times.
uint8_t HotSummer(){
    static uint8_t cooledDown = 0;

    if (CheckState(SUMMERS_COMPLETED)){
        iLED[SCARAB[R]] = 0;
        iLED[SCARAB[G]] = dimValue;
        return 1;
    }

    if (CheckState(FIRST_SUMMER)) {
        iLED[SCARAB[R]] = dimValue;
        if ((cooledDown) && (adcTemp >= (calTemp + 32))) {
            UpdateState(SUMMERS_COMPLETED);
            return 0;
        }
        if (adcTemp <= (calTemp + 8)) cooledDown = 1;
    }            
    if (calTemp == 0) calTemp = adcTemp;

    if (adcTemp >= (calTemp + 32)) {
        UpdateState(FIRST_SUMMER);
    }
    
    return 0;
}

//Turn on a number of LEDs on left/right wings, starting from bottom.
void WingBar(int8_t l, int8_t r) {
    for (int8_t i=0; i<5; i++) {
        if (i<l)
            iLED[WING[L][i]] = dimValue;
        else
            iLED[WING[L][i]] = 0;
        if (i<r)
            iLED[WING[R][i]] = dimValue;
        else
            iLED[WING[R][i]] = 0;
    }
}

//Set both eyes in red or green, 0...255 dimming
void SetBothEyes(uint8_t r, uint8_t g) {
    for (uint8_t i=0; i<2; i++) {
        iLED[EYE[R][i]] = r;
        iLED[EYE[G][i]] = g;
    }
}

//Set all of the necklace LEDs the same brightness in red and green.
void SetHackerLeds(uint8_t r, uint8_t g) {
    for (uint8_t i=0;i<6;i++) {
        iLED[HCKR[R][i]] = r;
        iLED[HCKR[G][i]] = g;
    }
}

//When all of the challenges are finished, play a blinky thing on the LEDs
void VictoryDance() {
    SetBothEyes(0,dimValue);
    switch((RTC_CNT>>12)&0x7) {
        case 0:
        case 1:
        case 2:
            // Falling rain
            effect = 8;
            break;
        case 4:
            // Circling wings
            effect = 6;
            break;
        case 6:
            // Random wings
            effect = 7;
            break;
        default:
            WingBar(0,0);
            effect = 2;
            break;
    }
}

//LED control for game status and light effects during games.
void GenerateBlinks(){
    /*
    HCKR + BADGER are used for game progress and should not be
    used by the games, the other LEDs can be used by setting 
    effect (or be set directly)
    */

    //Activate HCKR & BADGER leds based on the state-bits
    if (gameNow == TEXT) {
        for (uint8_t i=0;i<6;i++) {
            if(CheckState(HACKER_STATES + i)) {
                iLED[HCKR[G][i]] = dimValue;
                iLED[HCKR[R][i]] = 0;
            } else {
                iLED[HCKR[G][i]] = 0;
                iLED[HCKR[R][i]] = dimValue;
            }
        }
    }

    if (CheckState(GEM_STATE)) {
        switch( HeartCount ) {
            case 0:
            case 6:
                iLED[BADGER] = dimValue>>2;
                break;
            case 1:
            case 3:
            case 5:
                iLED[BADGER] = dimValue>>1;
                break;
            case 2:
            case 4:
                iLED[BADGER] = dimValue;
                break;
            default:
                iLED[BADGER] = 0;
                break;
        }
        if (HeartCount<32)
            HeartCount++;
        else
            HeartCount = 0;
    }

    //LEDs effects for all games
    // Keep a counter for dynamic effects
    LedCount++;
    
    switch (effect&0x1f) {
        // All LEDs off
        case 0:
        case 16:
            SetBothEyes(0,0);
            if ((effect&16)==0) {
                WingBar(0,0);
                iLED[CAT] = 0;
            } else {
                effect = 0x1f;
            }
            break;

        //'flashing red eyes',       # 1 can be used when a bad answer is given
        case 1:
        case 17:
            SetBothEyes((LedCount & 1 ? dimValue : 0),0);
            if (effect & 16)
                SetHackerLeds(dimValue,0);
            break;

        //'flashing green eyes',     # 2 can be used when a good answer is given
        case 2:
            SetBothEyes(0, 0x1f + (((LedCount & 0x08) ? ((LedCount&0x07)^0x07) : (LedCount&0x07))<<5));
            break;

        //'flash both wings'
        case 5:
            if ((LedCount & 3) == 0) {
                if (LedCount & 4)
                    WingBar(5,5);
                else
                    WingBar(0,0);
            }
            break;

        //'circle the wing leds'
        case 6:
            if (LedCount > 4) LedCount = 0;
            iLED[WING[L][LedCount]] = 0;
            iLED[WING[L][LedCount == 4 ? 0 : LedCount+1]] = dimValue;
            iLED[WING[R][4-LedCount]] = 0;
            iLED[WING[R][LedCount == 4 ? 4 : 3-LedCount]] = dimValue;
            break;

        //'random wing leds'
        case 7:
            for (uint8_t x=0; x<5; ++x){
                iLED[WING[L][x]] = (lfsr() > 127)?dimValue:0;
                iLED[WING[R][x]] = (lfsr() > 127)?dimValue:0;
            }
            break;

        //'falling rain'
        case 8:
            if ((LedCount & 1) == 0) {
                for (uint8_t x=0; x<4; ++x){
                    iLED[WING[L][x]] = iLED[WING[L][x+1]];
                    iLED[WING[R][x]] = iLED[WING[R][x+1]];
                }
                iLED[WING[L][4]] = (lfsr() > 224)?dimValue:0;
                iLED[WING[R][4]] = (lfsr() > 224)?dimValue:0;
            }
            break;


        // No LED action, game will do it's own LED magic
        case 31:
        default:
            break;
            
    }
}

//Fade out audio (slowest 0: 8s, 1: 4s, 2: 2s, 3: 1s, 4: 0.5s, 5: 0.25s, 6: 0.125s and 7: 0.062 seconds from maximum volume(0xff))
void FadeOut(uint8_t spd, uint8_t off)
{
    spd = 7 - (spd&0x07);
    uint8_t tick = fastTicker >> spd;
    if (tick) {
        if (auVolume > tick) auVolume -= tick; else {
            auVolume = 0;
            if (off) {
                auRepAddr = &zero;
                effect &= 0x1f;
            }
        }
        fastTicker = 0;
    }
}

//Play a sample 
uint8_t Play(uint8_t * auBuffer, uint8_t repeat, uint16_t pitch, uint8_t volume)
{
    TCB1_CCMP = pitch;
    if (repeat) auRepAddr = auBuffer;
    else auSmpAddr = auBuffer;
    auVolume = volume;
    return 1;
}

//This is the audio routine, it's magic!
uint8_t GenerateAudio(){
    static uint8_t start = 0;
    static uint8_t duration;

    //Headphones detected?
    if (auIn < HPLVL) {

        detHdPh = 1;

        //Audio for text adventure
        if ((effect&0xff00)==0) {
        
            //Silence, I kill u
            if ((effect&0xE0)==0){
                auRepAddr = &zero;
                start = 0;
            }

            //Bad answer (buzzer, also used in other games)
            else if ((effect&0xE0)==32){
                static uint8_t auBuffer[17] = {1, 255, 128, 128, 192, 255, 64, 255, 192, 128, 64, 1, 192, 1, 64, 128, 0}; 
                auBuffer[2] = floatAround(0x80, 5, 0x01, 0x00);

                if (start == 0) {
                    start = Play(&auBuffer[0], 1, 0x2100, 0xff);
                    duration = 8;
                }

                if (duration == 0) FadeOut(4, start);
                floatSpeed(1, 0x2000, 0x2200);
            }

            //Good (bell)
            else if ((effect&0xE0)==64){   //64
                static uint8_t auBuffer[3] = {255, 1, 0};

                if (start == 0) {
                    start = Play(&auBuffer[0], 1, 0x0a00, 0xff);
                    duration = 6;
                }

                if (duration == 0) FadeOut(4, start);
                if (buttonMark){
                    TCB1_CCMP -= 0x080;                    
                    if (auVolume == 0) effect = 0;
                }
            }

            //Rain storm with whistling wind and ghostly 
            else if (((effect&0xE0)==128)||((effect&0xE0)==96)){
                static uint8_t auBuffer[7];
                auBuffer[6]= 0;        
                auRepAddr = &auBuffer[0];

                //Noise is to be generated fast, outside of buttonMark loop
                for (uint8_t x=1; x<6; ++x){
                    if ((x>0) && (x!=3)) {
                        auBuffer[x] = floatAround(0x80, 6-((effect&0xa0)>>5), 0x01, 0x00);
                    }
                }

                if (buttonMark){
                    //"Floating" speed for howl (and noise, but that's hardly audible)
                    if (effect & 128) floatSpeed(6, 0x0800, 0x2000); 
                    else              floatSpeed(5, 0x0280, 0x0400);
            
                    //"Floating" volume and wind howl during 8 bit rainstorm needs some randomness
                    auVolume = floatAround(auVolume, 2, 0x10, 0xA0);
                    auBuffer[0] = floatAround(auBuffer[0], 2, 0xD0-effect, 0x90);
                    auBuffer[3] = 0xFF-auBuffer[0];  //Inverse value of wind[0] produces a whistle
                }
            }

            //AAAhhhh failed sound effect, too tired...
            else if ((effect&0xE0)==192){ //128//192
                /*
                static uint8_t auBuffer[8] = {64, 200, 240, 128, 64, 32, 16, 0};
                start = Play(&auBuffer[0], 1, (0x1000 - (effect<<5)), auVolume);
                floatSpeed(5, 0x0280, 0x0400);
                auBuffer[4]=80+lfsr()>>3;
                FadeOut(1, start);         
                */
            }

            //Bleeps
            else if ((effect&0xE0)==160){
                static uint8_t auBuffer[6] = {255, 192, 128, 64 ,1 ,0};
                
                start = Play(&auBuffer[0], 1, TCB1_CCMP, auVolume);

                FadeOut(0, start);
                if (buttonMark){
                    TCB1_CCMP = 0x0400 + (lfsr()<<5);
                    for(uint8_t x=0; x<5; ++x){
                        auBuffer[x]=lfsr()|0x01;
                    }
                }
            }

            //Footsteps
            else if ((effect&0xE0)==224){//224){
                static uint8_t auBuffer[8] = {64, 200, 240, 128, 64, 32, 16, 0};
                static uint8_t interval = 6;

                
                if (start == 0) {
                    auRepAddr = &auBuffer[6];
                    start = Play(&auBuffer[0], 0, 0x6000, auVolume);
                }

                FadeOut(0, start);
                if (buttonMark){
                    --interval;
                    if (interval == 0){
                        TCB1_CCMP += 0x0300;
                        interval = 6;
                        start = 0;
                    }
                }
            }

        } else if ((effect&0xff00)==0x0100) {
            if ((effect&0xE0) <= 0x90) {
                
                static uint8_t auBuffer[3] = {255, 1, 0};
                static uint16_t freq;

                if (start == 0) {
                    freq = ((effect&0xE0)+1)<<6;
                    start = Play(&auBuffer[0], 1, freq, 0xff);
                    duration = 3;
                }

                if (duration == 0) FadeOut(7, start);
                floatSpeed(1, freq+0x0200, freq+0x0300);
                //auBuffer[] = floatAround(0x80, 5, 0x01, 0x00);
            }
        }

    //No headphone, output can do other things like trying to find another bagder.
    } else {
        detHdPh = 0;
    }

    //Buttonmark is used for timing slow things.
    if (buttonMark && duration) --duration;

    return buttonMark;
}

//Tick, tock
uint16_t getClock() {
    uint16_t tmp = RTC_CNT;
    return 60 * minuteMark + (tmp>>9);
}

//Timeout for going back to things and for repeating sequences
uint8_t idleTimeout(uint16_t lastActive, uint16_t maxIdle) {
    uint16_t curClock;

    curClock = getClock();
    if (curClock < lastActive)
        curClock += 256 * 60;

    return (curClock > (lastActive + maxIdle));
}

//Checks the most important hardware features before starting up the first time.
uint8_t SelfTest(){
    uint8_t tstVal[4] = {0, 0, 0, 0};

    //Wait until we have a value for adcTemp, after that seed random and if virgin, save adcTemp as calibration value for HotSummers
    while (adcTemp == 0) ;
    lfsrSeed = (adcPhot + adcTemp + adcHall)<<1 | 0x0001;

    EERead(BOOTCHK, &tstVal[0], 4);
    //already checked and ok, skip test, can be reset by using "ikillu" command.
    if (tstVal[0] == 0xA5) {
        calTemp =  tstVal[1]<<8;
        calTemp |= tstVal[2];
        return 0; 
    } 

    //Old data in EEPROM, wipe!
    if (tstVal[3] != 0xff) return 1;

    //Red BADGER, CAT, EYEs and SCARAB LED on 100% = error
    iLED[BADGER] = 255;
    iLED[CAT] = 255;
    iLED[EYE[R][R]] = 255;    
    iLED[EYE[R][L]] = 255;
    iLED[SCARAB[R]] = 255;

    //Light sensor OK, scarab off
    tstVal[0] = adcPhot&0xff;
    while (tstVal[0] == (adcPhot&0xff)) ;
    iLED[SCARAB[R]] = 0x00;

    //Buttons OK (none pressed / shorted), cat forehead off
    while ((adcBtns>>4) < 200) ;
    iLED[CAT] = 0x00;

    //Right game ROM version, right eye off
    ExtEERead(0x34D2, 4, 0, (uint8_t *)&tstVal[0]);
    if ((tstVal[0] != 0x02) || (tstVal[1] != 0xfe) || (tstVal[2] != 0x00) || (tstVal[3] != 0x54)){  //Newestest ROM @ 0x34D2: 0x02, 0xfe, 0x00, 0x54
        while(1);
    }
    iLED[EYE[R][R]] = 0x00;
    
    //Audio in/out OK, left eye off
    SelectAuIn();
    while ((auIn < 0x78) || (auIn > 0x88)) ;
    iLED[EYE[R][L]] = 0x00;
    
    //All ok!
    tstVal[0] = 0xA5;
    tstVal[1] = adcTemp>>8;
    tstVal[2] = adcTemp&0xff;

    EEWrite(BOOTCHK, &tstVal[0], 3);
    iLED[BADGER] = 0x08;
    return 0;
}
