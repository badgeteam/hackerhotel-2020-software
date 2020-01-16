/*
 * resources.h
 *
 * Contains all the crap that is needed under the hood 
 *
 * Created: 06/01/2020 18:25:19
 *  Author: Badge.team
 */ 


#ifndef RESOURCES_H_
#define RESOURCES_H_
    #include <main_def.h>
    #include <avr/io.h>
    #include <avr/interrupt.h>
    #include <util/delay.h>
    #include <stdlib.h>

    void setup();
    ISR(TCA0_LUNF_vect);                                                // LED matrix interrupt routine
    ISR(TCB0_INT_vect);                                                 // Used for sending serial data and serSpeed "typing" effect
    ISR(USART0_RXC_vect);                                               // RX handling of serial data
    ISR(USART0_DRE_vect);                                               // TX handling of serial data, working together with TCB0_INT_vect
    ISR(ADC0_RESRDY_vect);                                              // Used for getting temperature (adcTemp) and audio input data
    ISR(ADC1_RESRDY_vect);                                              // Used for getting light (adcPhot), magnetic (adcHall) and raw button values (adcButt)
    ISR(RTC_PIT_vect);                                                  // Periodic interrupt for triggering button and sensor readout of ADC1 (32 samples per second)
    uint8_t SerSend(unsigned char *addr);                               // Send characters beginning with *addr, stops at string_end (0 character).
    void SerSpeed(uint8_t serSpd);                                      // Change the character speed from badge to user for "typing" effects during the text adventure.
    void SelectTSens();                                                 // Select the temperature sensor for ADC0 (discard the first two samples after switching)
    void SelectAuIn();                                                  // Select the "audio input" for ADC0 (discard the first two samples after switching)
    uint8_t CheckButtons(uint8_t previousValue);                        // Readout of the button state, with duration. Run periodically, but not faster than (PIT interrupt sps / 3) times per second.
    void EERead(uint8_t eeAddr, uint8_t *eeValues, uint8_t size);       // Read from internal EEPROM, wraps around if eeAddr+size>255
    uint8_t EEWrite(uint8_t eeAddr, uint8_t *eeValues, uint8_t size);   // Write to internal EEPROM, wraps around if eeAddr+size>255
    
    

#endif /* RESOURCES_H_ */