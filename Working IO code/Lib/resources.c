/*
 * resources.c
 *
 * Created: 06/01/2020 18:26:44
 *  Author: Badge.team
 */ 

#include <resources.h>
#include <I2C.h>                //Fixed a semi-crappy lib found on internet, replace with interrupt driven one? If so, check hardware errata pdf!

volatile uint16_t tmp16bit;     

void setup(){
    cli();

    //Set up clock at 10MHz
    CCP = CCP_IOREG_gc;
    CLKCTRL_MCLKCTRLB = 0x01;

    //Set I/O direction registers
    PORTA_DIR = 0b01001010;
    PORTB_DIR = 0b01111100;
    PORTC_DIR = 0b00111111;
     
    //UART (Alternative pins PA1=TxD, PA2=RxD, baudrate 9600, 8n1, RX and Buffer empty interrupts on)
    PORTMUX_CTRLB = 0x01;
    PORTA_OUTSET = 0x02;
    USART0_BAUD = (uint16_t)USART0_BAUD_RATE(115200);
    USART0_CTRLA = 0xA0; //Interrupts on
    USART0_CTRLB = 0xC0; //RX and TX on
    USART0_CTRLC = 0x03; //8 bits data, no parity, 1 stop bit
     
    //GPIO registers 0..3 can be used for global variables used in ISR routines (ASM capability: IN, OUT, SBI, CBI, SBIS, SBIC)
    L_COL = 0;     //Used in LED array to hold currently driven column value
    RXCNT = 0;     //Used as UART receive counter
    AUPOS = 0;     //Used as audio input/output buffer counter
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

    //Init I2C (100kHz)
    I2C_init();

    //VREF (DAC, ADC's)
    VREF_CTRLA   = 0x12;    //0x22 for audio in/out (2.5V), 0x12 for temperature in, audio out (1.1V/2.5V)
    VREF_CTRLC   = 0x20;    //ADC1 reference at 2.5V
    VREF_CTRLB   = 0x01;    //DAC0 ref forced enabled

    //Init ADC0 (audio in, also controls DAC output rate, internal temperature?)
    ADC0_CTRLA   = 0x06;    //8 bit resolution, free running
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
    tmp16bit = (RTC_CNT + BTN_TMR)%RTC_PER;
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
    /*"cpi r24, 252      \n"      //Quick fix: Somehow values above 251 can cause data to shift to other LEDs, weird...
    "brlo .+2          \n"      //Quick fix
    "ldi r24, 251      \n"      //Quick fix
    */"st %a[wo]+, r24   \n"
    "dec r25           \n"
    "brne .-8         \n"
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
    
    if(timeout_I2C) timeout_I2C--;
    TCA0_SPLIT_INTFLAGS = 0xFF;
}

// TCB0 is used for (slowly) sending serial characters. A 0x00 character code is needed to terminate sending or it will leak memory to serial. It's a feature, honest! 
ISR(TCB0_INT_vect){
    if (*serTxAddr) {
        USART0_TXDATAL = *serTxAddr;
        serTxAddr++;
        USART0_CTRLA |= USART_DREIE_bm;
    } else {
        serTxDone = 1;
        TCB0_INTCTRL = 0x00;
    }
    TCB0_INTFLAGS = TCB_CAPT_bm;
}

// TCB1 is used for audio generation. Keeps playing "data" until 0 is reached. Audio sample data can contain 0x01 to 0xFF, centered around 0x80
ISR(TCB1_INT_vect){
    if (*auSmpAddr) {
        DAC0_DATA = *auSmpAddr;
        auSmpAddr++;
        } else {
        DAC0_DATA = 0x80;
        auPlayDone = 1;
    }
    TCB1_INTFLAGS = TCB_CAPT_bm;
}

// Reads up to RXLEN characters until LF is found, LF sets the serRxDone flag and writes 0 instead of LF.
ISR(USART0_RXC_vect){
    serRx[RXCNT] = USART0.RXDATAL;
    if (serRx[RXCNT] == 0x0A){
        serRx[RXCNT] = 0;
        serRxDone = 1;
    } else if (RXCNT < (RXLEN-1)) RXCNT++;
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
    AUPOS = (AUPOS+1)&(AULEN-1);
    if (adc0Chg == 0){
        if (ADC0_MUXPOS == 0x1E) adcTemp = ADC0_RESL; else auIn[AUPOS]=ADC0_RESL;
    } else adc0Chg = 0;
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

//RTC compare interrupt, triggers at 512/BTN_TMR rate, also RTC overflow interrupt, triggers once a minute
ISR(RTC_CNT_vect) {
    if (RTC_INTFLAGS & RTC_CMP_bm){
        if (buttonMark<255) buttonMark++;   // For button timing purposes
        tmp16bit = (RTC_CNT + BTN_TMR)%RTC_PER;
        while(RTC_STATUS & RTC_CMPBUSY_bm);
        RTC_CMP = tmp16bit;                 // Button timing: next interrupt set
        RTC_INTFLAGS = RTC_CMP_bm;		    // clear interrupt flag
    } else {
        if (minuteMark<255) minuteMark++;   // For very slow timing purposes
        RTC_INTFLAGS = RTC_OVF_bm;		    // clear interrupt flag
    }
}

//PIT interrupt (timing of ADC0: audio/temperature value)
ISR(RTC_PIT_vect) {						// PIT interrupt handling code
    ADC1_COMMAND = 0x01;
    RTC_PITINTFLAGS = RTC_PI_bm;		// clear interrupt flag
}

//Read bytes from EEPROM
void EERead(uint8_t eeAddr, uint8_t *eeValues, uint8_t size)
{
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

//Sends a set of characters to the serial port, stops only when character value 0 is reached.
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
     VREF_CTRLA   = 0x12;    //0x22 for audio in/out (2.5V), 0x12 for temperature in, audio out (1.1V/2.5V)
     ADC0_CTRLA   |= ADC_RESSEL_bm;
     ADC0_MUXPOS  = 0x1E;    //Audio in: AIN7 at (0x07), Temperature: Internal sensor at (0x1E)
     adc0Chg = 1;
};

// Select "audio" input (0-2.5V)
void SelectAuIn(){
     VREF_CTRLA   = 0x22;    //0x22 for audio in/out (2.5V), 0x12 for temperature in, audio out (1.1V/2.5V)
     ADC0_CTRLA   &= ~(ADC_RESSEL_bm);
     ADC0_MUXPOS  = 0x07;    //Audio in: AIN7 at (0x07), Temperature: Internal sensor at (0x1E)
     adc0Chg = 1;
};

//Returns button combination (4LSB) and number of consecutive times this combination is detected. First read should always be ignored! 
uint8_t CheckButtons(uint8_t previousValue){
    uint8_t bADC = (uint8_t)(adcBtns>>4);
    uint8_t bNibble = 0x0F;     //MSB to LSB: Bottom left, top left, top right, bottom right, 0F is error
    
    switch(bADC){
        case 28 ... 32:         //30: All buttons pressed
        bNibble = 0b1111;
        break;

        case 35 ... 39:          //37: Both left
        bNibble = 0b1100;
        break;

        case 45 ... 49:          //47: Both bottom
        bNibble = 0b1001;
        break;

        case 50 ... 54:          //52: Bottom left
        bNibble = 0b1000;
        break;

        case 62 ... 68:          //65: Both top
        bNibble = 0b0110;
        break;

        case 83 ... 91:          //87: Top left
        bNibble = 0b0100;
        break;

        case 99 ... 109:         //104: Both right
        bNibble = 0b0011;
        break;

        case 121 ... 137:        //129: Top right
        bNibble = 0b0010;
        break;

        case 162 ... 186:        //174: Bottom right
        bNibble = 0b0001;
        break;

        case 240 ... 255:        //255: No button pressed
        bNibble = 0b0000;
        break;
    }

    if ((previousValue&0x0F)==bNibble){
        if ((previousValue&0xF0) < 0xF0) previousValue += 0x10; //Same value? Increase time nibble
        return previousValue;       
    } else if (bNibble == 0) return 0xFF; //Buttons released, 0xFF triggers readout, previousValue holds last button value.
              else return bNibble;  //New value  
}

unsigned char nibbleSwap(unsigned char a)
{
    return (a<<4) | (a>>4);
}


