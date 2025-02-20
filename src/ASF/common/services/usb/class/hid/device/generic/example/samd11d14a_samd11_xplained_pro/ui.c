/**
 * \file
 *
 * \brief User Interface
 *
 * Copyright (c) 2014-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#include <asf.h>
#include "ui.h"

#define BTN_UP				PIN_PA05
#define BTN_RIGHT			PIN_PA08
#define BTN_DOWN			PIN_PA06
#define BTN_LEFT			PIN_PA07
#define BTN_SELECT			PIN_PA02
#define BTN_START			PIN_PA03
#define BTN_A				PIN_PA09
#define BTN_B				PIN_PA04

#define HATSWITCH_UP            0x00
#define HATSWITCH_UPRIGHT       0x01
#define HATSWITCH_RIGHT         0x02
#define HATSWITCH_DOWNRIGHT     0x03
#define HATSWITCH_DOWN          0x04
#define HATSWITCH_DOWNLEFT      0x05
#define HATSWITCH_LEFT          0x06
#define HATSWITCH_UPLEFT        0x07
#define HATSWITCH_NONE          0x0F


static bool ui_b_led_blink = true;
static uint8_t ui_hid_report[UDI_HID_REPORT_IN_SIZE];
static uint8_t buttonPresses = 0;
static uint8_t lastButtonPresses = 0;
static uint8_t hatPresses = 0;
static uint8_t lastHatPresses = 0;

void ui_init(void)
{
	/* Initialize LEDs */
	LED_On(LED_0_PIN);
}

void ui_powerdown(void)
{
	LED_Off(LED_0_PIN);
}


void ui_wakeup_enable(void)
{

}

void ui_wakeup_disable(void)
{

}

void ui_wakeup(void)
{
	LED_On(LED_0_PIN);
}

void ui_process(uint16_t framenumber)
{
	// bool b_btn_state;
	// static bool btn0_last_state = false;
	static uint8_t cpt_sof = 0;

	// Blink LED
	if (ui_b_led_blink) {
		if ((framenumber % 1000) == 0) {
			LED_On(LED_0_PIN);
		}
		if ((framenumber % 1000) == 500) {
			LED_Off(LED_0_PIN);
		}
	}

	// Scan process running each 40ms
	cpt_sof++;
	if (cpt_sof < 40) {
		return;
	}
	cpt_sof = 0;

	// Scan buttons
    bool btn_select  =  !port_pin_get_input_level(BTN_SELECT);
    bool btn_start   =  !port_pin_get_input_level(BTN_START);
    bool btn_a       =  !port_pin_get_input_level(BTN_A);
    bool btn_b       =  !port_pin_get_input_level(BTN_B);

    bool btn_up      =  !port_pin_get_input_level(BTN_UP);
    bool btn_right   =  !port_pin_get_input_level(BTN_RIGHT);
    bool btn_down    =  !port_pin_get_input_level(BTN_DOWN);
    bool btn_left    =  !port_pin_get_input_level(BTN_LEFT);

	buttonPresses = ((btn_select << 3) | (btn_start << 2) | (btn_a << 1) | (btn_b));
	//uint8_t hatButtonPresses =((btn_up << 3) | (btn_right << 2) | (btn_down << 1) | (btn_left));


    if (!btn_up && !btn_right && !btn_down && !btn_left)
        hatPresses = HATSWITCH_NONE;  // Neutral state; adjust your descriptor if needed.

    // Check diagonal combinations first.
    if (btn_up && btn_right && !btn_down && !btn_left)
        hatPresses = HATSWITCH_UPRIGHT;  // Up-Right

    if (btn_right && btn_down && !btn_up && !btn_left)
        hatPresses = HATSWITCH_DOWNRIGHT;  // Down-Right

    if (btn_down && btn_left && !btn_up && !btn_right)
        hatPresses = HATSWITCH_DOWNLEFT;  // Down-Left

    if (btn_left && btn_up && !btn_right && !btn_down)
        hatPresses = HATSWITCH_UPLEFT;  // Up-Left

    // Now check for single directional presses.
    if (btn_up && !btn_right && !btn_down && !btn_left)
        hatPresses = HATSWITCH_UP;  // Up

    if (btn_right && !btn_up && !btn_down && !btn_left)
        hatPresses = HATSWITCH_RIGHT;  // Right

    if (btn_down && !btn_up && !btn_right && !btn_left)
        hatPresses = HATSWITCH_DOWN;  // Down

    if (btn_left && !btn_up && !btn_right && !btn_down)
        hatPresses = HATSWITCH_LEFT;  // Left

// #define HATSWITCH_UP            0x00
// #define HATSWITCH_UPRIGHT       0x01
// #define HATSWITCH_RIGHT         0x02
// #define HATSWITCH_DOWNRIGHT     0x03
// #define HATSWITCH_DOWN          0x04
// #define HATSWITCH_DOWNLEFT      0x05
// #define HATSWITCH_LEFT          0x06
// #define HATSWITCH_UPLEFT        0x07
// #define HATSWITCH_NONE          0x0F

	// switch (hatButtonPresses)
	// {
	// case 0x01:
	// 	hatPresses = HATSWITCH_LEFT;
	// 	break;

	// case 0x02:
	// 	hatPresses = HATSWITCH_DOWN;
	// 	break;

	// case 0x03:
	// 	hatPresses = HATSWITCH_DOWNLEFT;
	// 	break;

	// case 0x04:
	// 	hatPresses = HATSWITCH_RIGHT;
	// 	break;

	// case 0x05:
	// 	hatPresses = HATSWITCH_NONE; // right & left?
	// 	break;

	// case 0x06:
	// 	hatPresses = HATSWITCH_DOWNRIGHT;
	// 	break;

	// case 0x07:
	// 	hatPresses = HATSWITCH_NONE;// right, down & left?
	// 	break;

	// case 0x08:
	// 	hatPresses = HATSWITCH_UP;
	// 	break;

	// case 0x09:
	// 	hatPresses = HATSWITCH_UPLEFT;
	// 	break;

	// case 0x0a:
	// 	hatPresses = HATSWITCH_NONE; // up and down?
	// 	break;

	// case 0x0b:
	// 	hatPresses = HATSWITCH_UPRIGHT;
	// 	break;
	
	// default:
	// 	hatPresses = HATSWITCH_NONE;
	// 	break;
	// }

	// if ((buttonPresses != lastButtonPresses) || (hatPresses != lastHatPresses)) {
		LED_Toggle(PIN_PA22);
		ui_hid_report[0] = buttonPresses;
		ui_hid_report[1] = hatPresses;
		udi_hid_generic_send_report_in(ui_hid_report);
		lastButtonPresses = buttonPresses;
		lastHatPresses = hatPresses;
	// }
}

void ui_led_change(uint8_t *report)
{
	if (report[0]=='1') {
		// A led must be on
		switch(report[1]) {
			case '1':
			ui_b_led_blink = false;
			LED_On(LED_0_PIN);
			break;
		}
	} else {
		// A led can blink
		switch(report[1]) {
			case '1':
			ui_b_led_blink = true;
			break;
		}
	}
}

/**
 * \defgroup UI User Interface
 *
 * Human interface on SAMD11-XPlain:
 * - LED0 blinks when USB host has checked and enabled HID generic interface
 * - HID events LED1 can make LED0 always on or blinking
 * - Event buttons are linked to SW0.
 *
 */
