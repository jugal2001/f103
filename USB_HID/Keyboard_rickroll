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
int i = 1;
int tmp = 0;
uint8_t string_array[30] = { HID_KEY_F, HID_KEY_I, HID_KEY_R, HID_KEY_E,
HID_KEY_F,
HID_KEY_O, HID_KEY_X, HID_KEY_ENTER, HID_KEY_H, HID_KEY_Y, HID_KEY_O, HID_KEY_U,
HID_KEY_T,
HID_KEY_U, HID_KEY_PERIOD, HID_KEY_B, HID_KEY_E, HID_KEY_SLASH,
HID_KEY_D, HID_KEY_Q, HID_KEY_W, HID_KEY_4, HID_KEY_W, HID_KEY_9, HID_KEY_W,
HID_KEY_G, HID_KEY_X, HID_KEY_C, HID_KEY_Q, HID_KEY_ENTER };

void led_blinking_task(void);
void hid_task(void);
void start(void);
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

//--------------------------------------------------------------------+
// USB HID	firefoxeyoutu.be/dQw4w9WgXcQ
//--------------------------------------------------------------------+

/*uint8_t firefox[8] = { HID_KEY_F, HID_KEY_I, HID_KEY_R, HID_KEY_E, HID_KEY_F,
 HID_KEY_O, HID_KEY_X, HID_KEY_ENTER };*/

static void send_hid_report(uint8_t report_id, uint32_t btn) {
// skip if hid is not ready yet
	if (!tud_hid_ready())
		return;

	uint8_t keycode[6] = { 0 };
	int start = 0;
	if (btn == 1) {
		keycode[1] = HID_KEY_T;
		tmp = 0;
		tud_hid_keyboard_report(REPORT_ID_KEYBOARD,
				KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_LEFTALT,
				keycode);
		start = board_millis();

		while (board_millis() - start < 1000)
			;
	} else if (btn == 2) {
		keycode[1] = string_array[tmp];
		if (tmp == 8) {
			tud_hid_keyboard_report(REPORT_ID_KEYBOARD,
					KEYBOARD_MODIFIER_LEFTGUI, keycode);
			start = board_millis();
			while (board_millis() - start < 1000)
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
	} else if (btn) {
// Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
		send_hid_report(REPORT_ID_KEYBOARD, btn);
	}
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t itf, uint8_t const *report, uint8_t len) {
	(void) itf;
	(void) len;
	if (tmp < 31) {
		send_hid_report(REPORT_ID_KEYBOARD, 2);
		tmp++;
	} else if (tmp == 31)
		send_hid_report(REPORT_ID_KEYBOARD, 0);
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
