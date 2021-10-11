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
#include <string.h>
#include "bsp/board.h"
#include "tusb.h"

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
volatile uint8_t report_sent_flag = 0;
#define SHIFT 0x80
const uint8_t asciimap[128] = { 0x00,             // NUL
		0x00,             // SOH
		0x00,             // STX
		0x00,             // ETX
		0x00,             // EOT
		0x00,             // ENQ
		0x00,             // ACK
		0x00,             // BEL
		0x2a,          // BS  Backspace
		0x2b,          // TAB Tab
		0x28,          // LF  Enter
		0x00,             // VT
		0x00,             // FF
		0x00,             // CR
		0x00,             // SO
		0x00,             // SI
		0x00,             // DEL
		0x00,             // DC1
		0x00,             // DC2
		0x00,             // DC3
		0x00,             // DC4
		0x00,             // NAK
		0x00,             // SYN
		0x00,             // ETB
		0x00,             // CAN
		0x00,             // EM
		0x00,             // SUB
		0x00,             // ESC
		0x00,             // FS
		0x00,             // GS
		0x00,             // RS
		0x00,             // US

		0x2c,          // ' '
		0x1e | SHIFT,    // !
		0x34 | SHIFT,    // "
		0x20 | SHIFT,    // #
		0x21 | SHIFT,    // $
		0x22 | SHIFT,    // %
		0x24 | SHIFT,    // &
		0x34,          // '
		0x26 | SHIFT,    // (
		0x27 | SHIFT,    // )
		0x25 | SHIFT,    // *
		0x2e | SHIFT,    // +
		0x36,          // ,
		0x2d,          // -
		0x37,          // .
		0x38,          // /
		0x27,          // 0
		0x1e,          // 1
		0x1f,          // 2
		0x20,          // 3
		0x21,          // 4
		0x22,          // 5
		0x23,          // 6
		0x24,          // 7
		0x25,          // 8
		0x26,          // 9
		0x33 | SHIFT,    // :
		0x33,          // ;
		0x36 | SHIFT,    // <
		0x2e,          // =
		0x37 | SHIFT,    // >
		0x38 | SHIFT,    // ?
		0x1f | SHIFT,    // @
		0x04 | SHIFT,    // A
		0x05 | SHIFT,    // B
		0x06 | SHIFT,    // C
		0x07 | SHIFT,    // D
		0x08 | SHIFT,    // E
		0x09 | SHIFT,    // F
		0x0a | SHIFT,    // G
		0x0b | SHIFT,    // H
		0x0c | SHIFT,    // I
		0x0d | SHIFT,    // J
		0x0e | SHIFT,    // K
		0x0f | SHIFT,    // L
		0x10 | SHIFT,    // M
		0x11 | SHIFT,    // N
		0x12 | SHIFT,    // O
		0x13 | SHIFT,    // P
		0x14 | SHIFT,    // Q
		0x15 | SHIFT,    // R
		0x16 | SHIFT,    // S
		0x17 | SHIFT,    // T
		0x18 | SHIFT,    // U
		0x19 | SHIFT,    // V
		0x1a | SHIFT,    // W
		0x1b | SHIFT,    // X
		0x1c | SHIFT,    // Y
		0x1d | SHIFT,    // Z
		0x2f,          // [
		0x31,          // bslash
		0x30,          // ]
		0x23 | SHIFT,    // ^
		0x2d | SHIFT,    // _
		0x35,          // `
		0x04,          // a
		0x05,          // b
		0x06,          // c
		0x07,          // d
		0x08,          // e
		0x09,          // f
		0x0a,          // g
		0x0b,          // h
		0x0c,          // i
		0x0d,          // j
		0x0e,          // k
		0x0f,          // l
		0x10,          // m
		0x11,          // n
		0x12,          // o
		0x13,          // p
		0x14,          // q
		0x15,          // r
		0x16,          // s
		0x17,          // t
		0x18,          // u
		0x19,          // v
		0x1a,          // w
		0x1b,          // x
		0x1c,          // y
		0x1d,          // z
		0x2f | SHIFT,    // {
		0x31 | SHIFT,    // |
		0x30 | SHIFT,    // }
		0x35 | SHIFT,    // ~
		0x00              // DEL
		};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;
char array[100] = { 0 };
int size = 0;
void led_blinking_task(void);
void hid_task(void);
void start(void);

void press(uint8_t a);
void delay_ms(void);
/*------------- MAIN -------------*/
int main(void) {
	board_init();
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

void delay_ms(void) {
	int start = board_millis();
	while (board_millis() - start < 100)
		;

}
//--------------------------------------------------------------------+
// USB HID	firefoxeyoutu.be/dQw4w9WgXcQ
//--------------------------------------------------------------------+

void press(uint8_t a) {
	uint8_t keys[6] = { 0 };
	/*uint8_t modifier = 0;

	 if (a < 128) {
	 a = asciimap[a];
	 if (a & 0x80) {
	 modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
	 keys[0] = a & 0x7F;
	 } else {
	 keys[0] = a;
	 }
	 tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keys);
	 report_sent_flag = 0;*/
	char temparr[] = "hello world!";
	strcpy(array, temparr);
	size = strlen(temparr);
	tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keys);

}

static void send_hid_report(uint8_t report_id, uint32_t btn) {
// skip if hid is not ready yet
	if (!tud_hid_ready())
		return;

	/*

	 uint8_t keycode[6] = { 0 };
	 int start = 0;
	 if (btn == 1) {
	 keycode[1] = HID_KEY_T;
	 tmp = 0;
	 tud_hid_keyboard_report(REPORT_ID_KEYBOARD,
	 KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_LEFTALT,
	 keycode);
	 start = board_millis();

	 while (board_millis() - start < 500)
	 ;
	 } else if (btn == 2) {
	 keycode[1] = string_array[tmp];
	 if (tmp == 8) {
	 tud_hid_keyboard_report(REPORT_ID_KEYBOARD,
	 KEYBOARD_MODIFIER_LEFTGUI, keycode);
	 start = board_millis();
	 while (board_millis() - start < 700)
	 ;
	 }

	 else if (tmp == 19 || tmp == 24 || tmp == 26 || tmp == 28) {
	 tud_hid_keyboard_report(REPORT_ID_KEYBOARD,
	 KEYBOARD_MODIFIER_LEFTSHIFT, keycode);
	 } else {
	 tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
	 }
	 }

	 if (tmp == 30) {
	 tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
	 }
	 */

}

// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(void) {
// Poll every 10ms
	const uint32_t interval_ms = 2000;
	static uint32_t start_ms = 0;

	if (board_millis() - start_ms < interval_ms)
		return; // not enough time
	start_ms += interval_ms;
	if (!tud_hid_ready())
		return;

	press(66);
	/*
	 uint32_t const btn = board_button_read();

	 // Remote wakeup
	 if (tud_suspended() && btn) {
	 // Wake up host if we are in suspend mode
	 // and REMOTE_WAKEUP feature is enabled by host
	 tud_remote_wakeup();
	 } else if (btn) {
	 // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
	 send_hid_report(REPORT_ID_KEYBOARD, btn);
	 }*/
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t itf, uint8_t const *report, uint8_t len) {
	(void) itf;
	(void) len;
	uint8_t keys[6] = { 0 };
	if (report_sent_flag == 0) {
		keys[0] = 0;
		tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keys);
		report_sent_flag = 1;
	}
	static int count = 0;
	uint8_t modifier = 0;
	if (count <= size) {
		static uint8_t a = 0;
		if (a == array[count]) {
			a = 0;
			tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keys);
			return;
		}
		a = array[count];
		if (a < 128) {
			a = asciimap[a];
			if (a & 0x80) {
				modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
				keys[0] = a & 0x7F;
			} else {
				keys[0] = a;
			}
			tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keys);
			count++;
		}
		return;
	}
	count = 0;
	size = 0;
	//report_sent_flag = 0;
//send_hid_report(REPORT_ID_KEYBOARD, board_button_read());
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
	/*uint8_t keycode[6] = {0};
	 keycode[0] = HID_KEY_T;
	 hid_keyboard_report_t report;
	 report.modifier = 0;
	 memcpy(report.keycode, keycode, 6);

	 memcpy(buffer , &report , reqlen);*/
	return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
		hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
	(void) itf;

	if (report_type == HID_REPORT_TYPE_OUTPUT) {
// Set keyboard LED e.g Capslock, Numlock etc...
		if (report_id == REPORT_ID_KEYBOARD) {
			// bufsize should be (at least) 1
			if (bufsize < 1)
				return;

			uint8_t const kbd_leds = buffer[0];

			if (kbd_leds & KEYBOARD_LED_CAPSLOCK) {
				// Capslock On: disable blink, turn led on
				blink_interval_ms = 0;
				//press(65);

				board_led_write(true);
			} else {
				// Caplocks Off: back to normal blink
				board_led_write(false);
				blink_interval_ms = BLINK_MOUNTED;
			}
		}
	}
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void) {
	static uint32_t start_ms = 0;
	static bool led_state = false;

// blink is disabled
	if (!blink_interval_ms)
		return;

// Blink every interval ms
	if (board_millis() - start_ms < blink_interval_ms)
		return; // not enough time
	start_ms += blink_interval_ms;

	board_led_write(led_state);
	led_state = 1 - led_state; // toggle
}
