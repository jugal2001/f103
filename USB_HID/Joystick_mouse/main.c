/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"
#include "stm32f1xx.h"
#include "usb_descriptors.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum {
	BLINK_NOT_MOUNTED = 250, BLINK_MOUNTED = 1000, BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);
void hid_task(void);
volatile uint16_t adcdata[2] = { 0, 0 };

void adc_init(void) {
	//enable clock for port A and B , and AFIO
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN;

	//set up gpio A5 as analog input (pin A5-->adc channel 5)
	GPIOA->CRL |= GPIO_CRL_CNF5_1;
	GPIOA->CRL &= ~GPIO_CRL_CNF5_0;
	GPIOA->CRL &= ~(GPIO_CRL_MODE5_0 | GPIO_CRL_MODE5_1);

	//set up gpio A6 as analog input (pin A6-->adc channel 6)
	GPIOA->CRL |= GPIO_CRL_CNF6_1;
	GPIOA->CRL &= ~GPIO_CRL_CNF6_0;
	GPIOA->CRL &= ~(GPIO_CRL_MODE6_0 | GPIO_CRL_MODE6_1);

	RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6;   			//adc prescaler /6 (<14mhz)
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; 				//clock enable for adc
	//enable clock for DMA1
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;
	// L[3:0] in ADC1_SQR1 TO SET THE NUMBER OF CONVERSIONS (here default 0000 - 1 conv)

	ADC1->SQR1 |= ADC_SQR1_L_0;							//2 conversions
	ADC1->SQR3 |= ADC_SQR3_SQ1_0 | ADC_SQR3_SQ1_2; 	//channel 5 in sequence 1
	ADC1->SMPR2 |= ADC_SMPR2_SMP5_2; 		 		//set sampling rate (ch5)
	ADC1->SQR3 |= ADC_SQR3_SQ2_1 | ADC_SQR3_SQ2_2; 	//channel 6 in sequence 2
	ADC1->SMPR2 |= ADC_SMPR2_SMP6_2; 		 		//set sampling rate (ch6)

	/*****DMA SETTINGS*****/
	ADC1->CR2 |= ADC_CR2_DMA;							//enable DMA for ADC1
	DMA1_Channel1->CNDTR = 2;
	DMA1_Channel1->CMAR = (uint32_t) adcdata;
	DMA1_Channel1->CPAR = (uint32_t) &(ADC1->DR);
	DMA1_Channel1->CCR |= (DMA_CCR_CIRC | DMA_CCR_MINC | DMA_CCR_MSIZE_0
			| DMA_CCR_PSIZE_0);
	DMA1_Channel1->CCR |= DMA_CCR_EN;
	/**********************/
	ADC1->CR1 |= ADC_CR1_SCAN;
	ADC1->CR2 |= ADC_CR2_ADON | ADC_CR2_CONT; //turn on adc and set it to continous conversion mode
	int i;
	for (i = 0; i < 1000; i++)
		__NOP();
	ADC1->CR2 |= ADC_CR2_ADON;
	for (i = 0; i < 1000; i++)
		__NOP();
	ADC1->CR2 |= ADC_CR2_CAL;  		      		    //run calibration
	for (i = 0; i < 5000; i++)
		__NOP();
}
/*------------- MAIN -------------*/
int main(void) {
	board_init();
	adc_init();
	tusb_init();

	while (1) {
		tud_task(); // tinyusb device task
		led_blinking_task();

		hid_task();
	}

	return 0;
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
	blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
	blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
	(void) remote_wakeup_en;
	blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
	blink_interval_ms = BLINK_MOUNTED;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

static void send_hid_report(uint8_t report_id, uint32_t btn) {
	// skip if hid is not ready yet
	if (!tud_hid_ready())
		return;
	int8_t X, Y;
	if (adcdata[0] > 2400)
		X = -3;
	else if (adcdata[0] < 1600)
		X = 3;
	else
		X = 0;
	if (adcdata[1] > 2400)
		Y = 3;
	else if (adcdata[1] < 1600)
		Y = -3;
	else
		Y = 0;
	// no button, right + down, no scroll, no pan
	tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, X, Y, 0, 0);
}

// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(void) {
	// Poll every 10ms
	const uint32_t interval_ms = 10;
	static uint32_t start_ms = 0;

	if (board_millis() - start_ms < interval_ms)
		return; // not enough time
	start_ms += interval_ms;

	uint32_t const btn = board_button_read();

	// Remote wakeup
	if (tud_suspended() && btn) {
		// Wake up host if we are in suspend mode
		// and REMOTE_WAKEUP feature is enabled by host
		tud_remote_wakeup();
	} else {
		// Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
		send_hid_report(REPORT_ID_MOUSE, btn);
	}
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t itf, uint8_t const *report, uint8_t len) {
	(void) itf;
	(void) len;

	//uint8_t next_report_id = report[0]+1;
	send_hid_report(REPORT_ID_MOUSE, board_button_read());
	/*if (next_report_id < REPORT_ID_COUNT)
	 {
	 send_hid_report(next_report_id, board_button_read());
	 }*/
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
		hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
	// TODO not Implemented
	(void) itf;
	(void) report_id;
	(void) report_type;
	(void) buffer;
	(void) reqlen;

	return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
		hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
	// TODO set LED based on CAPLOCK, NUMLOCK etc...
	(void) itf;
	(void) report_id;
	(void) report_type;
	(void) buffer;
	(void) bufsize;
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void) {
	static uint32_t start_ms = 0;
	static bool led_state = false;

	// Blink every interval ms
	if (board_millis() - start_ms < blink_interval_ms)
		return; // not enough time
	start_ms += blink_interval_ms;

	board_led_write(led_state);
	led_state = 1 - led_state; // toggle
}
