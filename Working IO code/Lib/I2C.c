/*
 * I2C.c
 *
 * I2C Master
 *
 * Created: 10/05/2019
 * Author: Dieter Reinhardt
 *
 * Tested with Alternate Pinout
 *
 * This software is covered by a modified MIT License, see paragraphs 4 and 5
 *
 * Copyright (c) 2019 Dieter Reinhardt
 *
 * 1. Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * 2. The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * 3. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * 4. This software is strictly NOT intended for safety-critical or life-critical applications.
 * If the user intends to use this software or parts thereof for such purpose additional 
 * certification is required.
 *
 * 5. Parts of this software are adapted from Microchip I2C polled master driver sample code.
 * Additional license restrictions from Microchip may apply.
 *
 * Edited: 11/12/2019
 * By: Ralph Willekes
 * Reason: Libraries unavailable, old crap? Added 16 bit internal addresses (24LC256) support.
 */ 

#include <avr/io.h>
#include "I2C.h"
#include <util/delay.h>

#define NOP() asm volatile(" nop \r\n")

void I2C_init(void)
{
	PORTB_DIRCLR = 0b00000001;	                                    // SCL = PB0, tri-stated high, avoids glitch
    PORTB_OUTCLR = 0b00000001;							
    PORTB_PIN0CTRL = 0;
	PORTB_DIRSET = 0b00000010;                                      // SDA = PB1
	PORTB_OUTCLR = 0b00000010;
    PORTB_PIN1CTRL = 0;

	
	TWI0.MBAUD = (uint8_t) TWI0_BAUD(100000, 0);					// set MBAUD register, TWI0_BAUD macro calculates parameter for 100 kHz
	TWI0.MCTRLB = TWI_FLUSH_bm;										// clear the internal state of the master
	TWI0.MCTRLA =	  1 << TWI_ENABLE_bp							// Enable TWI Master: enabled
					| 0 << TWI_QCEN_bp								// Quick Command Enable: disabled
					| 0 << TWI_RIEN_bp								// Read Interrupt Enable: disabled
					| 0 << TWI_SMEN_bp								// Smart Mode Enable: disabled
					| TWI_TIMEOUT_DISABLED_gc						// Bus Timeout Disabled (inoperative, see errata)
					| 0 << TWI_WIEN_bp;								// Write Interrupt Enable: disabled

	PORTB_DIRSET = 0b00000001;	
	
	TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc;							// force bus idle
	TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm | TWI_BUSERR_bm);		// clear flags	
}

void I2C_recover(void)												// clock out I2C bus if in invalid state (e.g. after incomplete transaction)
{
	uint8_t	i;
	TWI0.MCTRLB |= TWI_FLUSH_bm;									// clear the internal state of the master
	TWI0.MCTRLA = 0;												// disable TWI Master
	PORTB_DIRCLR = 0b00000010;      								// SDA tri-stated high
	for (i = 0; i < 9; i++)											// SCL, 9 x bit-banging
	{
		PORTB_DIRSET = 0b00000001;			        				// 5 us low
		_delay_us(5);
		PORTB_DIRCLR = 0b00000001;      							// 5 us tri-stated high
		_delay_us(5);
	}

	
// re-enable master twice
// for unknown reasons the master might get stuck if re-enabled only once
// second re-enable will fail if SDA not enabled beforehand

	TWI0.MCTRLB = TWI_FLUSH_bm;										// clear the internal state of the master
	TWI0.MCTRLA =	  1 << TWI_ENABLE_bp							// Enable TWI Master: enabled
					| 0 << TWI_QCEN_bp								// Quick Command Enable: disabled
					| 0 << TWI_RIEN_bp								// Read Interrupt Enable: disabled
					| 0 << TWI_SMEN_bp								// Smart Mode Enable: disabled
					| TWI_TIMEOUT_DISABLED_gc						// Bus Timeout Disabled (inoperative, see errata)
					| 0 << TWI_WIEN_bp;								// Write Interrupt Enable: disabled			

	PORTB_DIRSET = 0b00000010;	

	TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc;							// force bus idle
	TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm | TWI_BUSERR_bm);		// clear flags

	TWI0.MCTRLB = TWI_FLUSH_bm;										// clear the internal state of the master (glitch on SDA)
	TWI0.MCTRLA =	  1 << TWI_ENABLE_bp							// Enable TWI Master: enabled
					| 0 << TWI_QCEN_bp								// Quick Command Enable: disabled
					| 0 << TWI_RIEN_bp								// Read Interrupt Enable: disabled
					| 0 << TWI_SMEN_bp								// Smart Mode Enable: disabled
					| TWI_TIMEOUT_DISABLED_gc						// Bus Timeout Disabled (inoperative, see errata)
					| 0 << TWI_WIEN_bp;								// Write Interrupt Enable: disabled

	PORTB_DIRSET = 0b00000001;

	TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc;							// force bus idle
	TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm | TWI_BUSERR_bm);		// clear flags
}

uint8_t I2C_start(uint8_t shifted_addr)								// device_addr + LSB set if READ
{
	TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm);						// clear Read and Write interrupt flags
	if (TWI0.MSTATUS & TWI_BUSERR_bm) return 4;						// Bus Error, abort
	TWI0.MADDR = shifted_addr;
	return 0;
}

uint8_t I2C_wait_ACK(void)											// wait for slave response after start of Master Write
{
	timeout_I2C = ADDR_TIMEOUT;												// reset timeout counter, will be decremented by LED matrix interrupt
	while (!(TWI0.MSTATUS & TWI_RIF_bm) && !(TWI0.MSTATUS & TWI_WIF_bm))	// wait for RIF or WIF set
	{
		if (!(timeout_I2C)) return 0xff;				            // return timeout error
	}
	TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm);						// clear Read and Write interrupt flags
	if (TWI0.MSTATUS & TWI_BUSERR_bm) return 4;						// Bus Error, abort
	if (TWI0.MSTATUS & TWI_ARBLOST_bm) return 2;					// Arbitration Lost, abort
	if (TWI0.MSTATUS & TWI_RXACK_bm) return 1;						// Slave replied with NACK, abort
	return 0;														// no error
}

// the Atmel device documentation mentions a special command for repeated start TWI_MCMD_REPSTART_gc,
// but this is not used in Atmel's demo code, so we don't use it either

void I2C_rep_start(uint8_t shifted_addr)						    // send repeated start, device_addr + LSB set if READ
{
	TWI0.MADDR = shifted_addr;	
}

uint8_t	I2C_read(uint8_t *data, uint8_t ack_flag)					// read data, ack_flag 0: send ACK, 1: send NACK, returns status
{
	timeout_I2C = READ_TIMEOUT;										// reset timeout counter, will be decremented by LED matrix interrupt
	if ((TWI0.MSTATUS & TWI_BUSSTATE_gm) == TWI_BUSSTATE_OWNER_gc)	// if master controls bus
	{		
		while (!(TWI0.MSTATUS & TWI_RIF_bm))						// wait for RIF set (data byte received)
		{
			if (!(timeout_I2C)) return 0xff;			            // return timeout error
		}
		TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm);					// clear Read and Write interrupt flags	
		if (TWI0.MSTATUS & TWI_BUSERR_bm) return 4;					// Bus Error, abort
		if (TWI0.MSTATUS & TWI_ARBLOST_bm) return 2;				// Arbitration Lost, abort
		if (TWI0.MSTATUS & TWI_RXACK_bm) return 1;					// Slave replied with NACK, abort				
		if (ack_flag == 0) TWI0.MCTRLB &= ~(1 << TWI_ACKACT_bp);	// setup ACK
		else		TWI0.MCTRLB |= TWI_ACKACT_NACK_gc;				// setup NACK (last byte read)
		*data = TWI0.MDATA;
		if (ack_flag == 0) TWI0.MCTRLB |= TWI_MCMD_RECVTRANS_gc;	// send ACK, more bytes to follow					
		return 0;
	}
	else return 8;													// master does not control bus
}

uint8_t I2C_write(uint8_t *data)									// write data, return status
{
	timeout_I2C = WRITE_TIMEOUT;									// reset timeout counter, will be decremented by LED matrix interrupt
	if ((TWI0.MSTATUS & TWI_BUSSTATE_gm) == TWI_BUSSTATE_OWNER_gc)	// if master controls bus
	{
		TWI0.MDATA = *data;		
		while (!(TWI0.MSTATUS & TWI_WIF_bm))						// wait until WIF set, status register contains ACK/NACK bit
		{
			if (!(timeout_I2C)) return 0xff;			            // return timeout error
		}
		if (TWI0.MSTATUS & TWI_BUSERR_bm) return 4;					// Bus Error, abort
		if (TWI0.MSTATUS & TWI_RXACK_bm) return 1;					// Slave replied with NACK, abort
		return 0;													// no error	
	}
	else return 8;													// master does not control bus
}

void I2C_stop()
{
	TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
}

// read/write multiple bytes (maximum number of bytes MAX_LEN)
// slave device address 7 bit
// simple error processing, on error master will be reset and error status = 1 returned
// addr_ptr		address of first array element to transfer
// slave reg	starting slave register
// num_bytes	number of bytes to transfer

uint8_t	I2C_read_bytes(uint8_t slave_addr, uint8_t *reg_ptr, uint8_t reg_len, uint8_t *dat_ptr, uint8_t dat_len)
{
	uint8_t status;
	if (dat_len > MAX_LEN) dat_len = MAX_LEN;
	status = I2C_start(slave_addr << 1);							// slave write address, LSB 0
	if (status != 0) goto error;
	status = I2C_wait_ACK();										// wait for slave ACK
	if (status == 1) {
		I2C_stop();													// NACK, abort
		return 1;
	}
	if (status != 0) goto error;
    while(reg_len > 0){
	    status = I2C_write(reg_ptr);    							// send slave start register
	    if (status != 0) goto error;
        reg_ptr++;
        reg_len--;
    }
	I2C_rep_start((slave_addr << 1) + 1);							// slave read address, LSB 1
	while (dat_len > 1) {
		status = I2C_read(dat_ptr, 0);								// first bytes, send ACK
		if (status != 0) goto error;
		dat_ptr++;
		dat_len--;
	}
	status = I2C_read(dat_ptr, 1);									// single or last byte, send NACK
	if (status != 0) goto error;
	I2C_stop();
	return 0;
	
error:
	I2C_recover();													// clock out possibly stuck slave, reset master
	return 0xff;													// flag error
}

uint8_t	I2C_write_bytes(uint8_t slave_addr, uint8_t *reg_ptr, uint8_t reg_len, uint8_t *dat_ptr, uint8_t dat_len)
{
	uint8_t status;
	if (dat_len > MAX_LEN) dat_len = MAX_LEN;
	status = I2C_start(slave_addr << 1);							// slave write address, LSB 0
	if (status != 0) goto error;
	status = I2C_wait_ACK();										// wait for Slave ACK
	if (status == 1) {
		I2C_stop();													// NACK, abort	
		return 1;													
	}	 										
	if (status != 0) goto error;
	while(reg_len > 0){
    	status = I2C_write(reg_ptr);    							// send slave start register
    	if (status != 0) goto error;
    	reg_ptr++;
    	reg_len--;
	}
	if (status != 0) goto error;
	while (dat_len > 0) {											// write bytes
		status = I2C_write(dat_ptr);
		if (status != 0) goto error;
		dat_ptr++;
		dat_len--;		
	}
	I2C_stop();
	return 0;

error:
	I2C_recover();
	return 0xff;
}