/**
 * \file
 *
 * \brief USB Device Human Interface Device (HID) generic interface.
 *
 * Copyright (c) 2009-2018 Microchip Technology Inc. and its subsidiaries.
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

#include "conf_usb.h"
#include "usb_protocol.h"
#include "udd.h"
#include "udc.h"
#include "udi_hid.h"
#include "udi_hid_generic.h"
#include <string.h>

/**
 * \ingroup udi_hid_generic_group
 * \defgroup udi_hid_generic_group_udc Interface with USB Device Core (UDC)
 *
 * Structures and functions required by UDC.
 *
 * @{
 */
bool udi_hid_generic_enable(void);
void udi_hid_generic_disable(void);
bool udi_hid_generic_setup(void);
uint8_t udi_hid_generic_getsetting(void);

//! Global structure which contains standard UDI interface for UDC
UDC_DESC_STORAGE udi_api_t udi_api_hid_generic = {
	.enable = (bool(*)(void))udi_hid_generic_enable,
	.disable = (void (*)(void))udi_hid_generic_disable,
	.setup = (bool(*)(void))udi_hid_generic_setup,
	.getsetting = (uint8_t(*)(void))udi_hid_generic_getsetting,
	.sof_notify = NULL,
};
//@}


/**
 * \ingroup udi_hid_generic_group
 * \defgroup udi_hid_generic_group_internal Implementation of UDI HID Generic
 *
 * Class internal implementation
 * @{
 */

/**
 * \name Internal defines and variables to manage HID generic
 */
//@{

//! To store current rate of HID generic
COMPILER_WORD_ALIGNED
		static uint8_t udi_hid_generic_rate;
//! To store current protocol of HID generic
COMPILER_WORD_ALIGNED
		static uint8_t udi_hid_generic_protocol;
//! To signal if the report IN buffer is free (no transfer on going)
static bool udi_hid_generic_b_report_in_free;
//! Report to send
COMPILER_WORD_ALIGNED
		static uint8_t udi_hid_generic_report_in[UDI_HID_REPORT_IN_SIZE];
//! Report to receive
COMPILER_WORD_ALIGNED
		static uint8_t udi_hid_generic_report_out[UDI_HID_REPORT_OUT_SIZE];
//! Report to receive via SetFeature
COMPILER_WORD_ALIGNED
		static uint8_t udi_hid_generic_report_feature[UDI_HID_REPORT_FEATURE_SIZE];

//@}

//! HID report descriptor for standard HID generic
UDC_DESC_STORAGE udi_hid_generic_report_desc_t udi_hid_generic_report_desc = { 
    {
        0x05, 0x01,          //  USAGE_PAGE (Generic Desktop)
        0x09, 0x04,          //  USAGE (Game Pad) // joystick = 0x04
        0xa1, 0x01,          //  COLLECTION (Application)
        0xa1, 0x00,          //  COLLECTION (Physical)
// Buttons
        0x05, 0x09,          //  USAGE_PAGE (Button)
        0x19, 0x01,          //  USAGE_MINIMUM (Button 1)
        0x29, 0x04,          //  USAGE_MAXIMUM (Button 4)
        0x15, 0x00,          //  LOGICAL_MINIMUM (0)
        0x25, 0x01,          //  LOGICAL_MAXIMUM (1)
        0x75, 0x01,          //  REPORT_SIZE (1)
        0x95, 0x04,          //  REPORT_COUNT (4)
        0x81, 0x02,          //  INPUT (Data, Var, Abs)
// Padding
        0x75, 0x04,          //  REPORT_SIZE (4)
        0x95, 0x01,          //  REPORT_COUNT (1)
        0x81, 0x03,          //  INPUT (Const, Array, Abs)
// Hat Switch
        0x05, 0x01,          //  USAGE_PAGE (Generic Desktop)
        0x09, 0x39,          //  USAGE (Hat Switch)
        0x15, 0x00,          //  LOGICAL_MINIMUM (0)
        0x25, 0x07,          //  LOGICAL_MAXIMUM (7)
        0x46, 0x3B, 0x01,    //  Physical Maximum  : 315 degrees (Optional)
        0x65, 0x14,          //  Unit              : English Rotation/Angular Position 1 degree (Optional)
        0x75, 0x04,          //  REPORT_SIZE (4)
        0x95, 0x01,          //  REPORT_COUNT (1)
        0x81, 0x02,          //  INPUT (Data, Var, Abs)
// Padding
        0x75, 0x04,          //  REPORT_SIZE (4)
        0x95, 0x01,          //  REPORT_COUNT (1)
        0x81, 0x03,          //  INPUT (Const, Array, Abs)
// end
        0xc0,                //  END_COLLECTION
        0xc0                 //  END_COLLECTION 
//Totaling 57 bytes long
    }
};

/*
OLD GENERIC HID DESCRIPTOR:
				0x06, 0xFF, 0xFF,	// 04|2   , Usage Page (vendor defined?)
				0x09, 0x01,	// 08|1   , Usage      (vendor defined
				0xA1, 0x01,	// A0|1   , Collection (Application)
				// IN report
				0x09, 0x02,	// 08|1   , Usage      (vendor defined)
				0x09, 0x03,	// 08|1   , Usage      (vendor defined)
				0x15, 0x00,	// 14|1   , Logical Minimum(0 for signed byte?)
				0x26, 0xFF, 0x00,	// 24|1   , Logical Maximum(255 for signed byte?)
				0x75, 0x08,	// 74|1   , Report Size(8) = field size in bits = 1 byte
				// 94|1   , ReportCount(size) = repeat count of previous item
				0x95, sizeof(udi_hid_generic_report_in),
				0x81, 0x02,	// 80|1   , IN report (Data,Variable, Absolute)
				// OUT report
				0x09, 0x04,	// 08|1   , Usage      (vendor defined)
				0x09, 0x05,	// 08|1   , Usage      (vendor defined)
				0x15, 0x00,	// 14|1   , Logical Minimum(0 for signed byte?)
				0x26, 0xFF, 0x00,	// 24|1   , Logical Maximum(255 for signed byte?)
				0x75, 0x08,	// 74|1   , Report Size(8) = field size in bits = 1 byte
				// 94|1   , ReportCount(size) = repeat count of previous item
				0x95, sizeof(udi_hid_generic_report_out),
				0x91, 0x02,	// 90|1   , OUT report (Data,Variable, Absolute)
				// Feature report
				0x09, 0x06,	// 08|1   , Usage      (vendor defined)
				0x09, 0x07,	// 08|1   , Usage      (vendor defined)
				0x15, 0x00,	// 14|1   , LogicalMinimum(0 for signed byte)
				0x26, 0xFF, 0x00,	// 24|1   , Logical Maximum(255 for signed byte)
				0x75, 0x08,	// 74|1   , Report Size(8) =field size in bits = 1 byte
				0x95, sizeof(udi_hid_generic_report_feature),	// 94|x   , ReportCount in byte
				0xB1, 0x02,	// B0|1   , Feature report
				0xC0	// C0|0   , End Collection
*/

/**
 * \name Internal routines
 */
//@{

/**
 * \brief Send a report to HID interface
 *
 */
static bool udi_hid_generic_setreport(void);

/**
 * \brief Initialize UDD to receive setfeature data
 */
static void udi_hid_generic_setfeature_valid(void);

/**
 * \brief Callback called when the report is received
 *
 * \param status     UDD_EP_TRANSFER_OK, if transfer is completed
 * \param status     UDD_EP_TRANSFER_ABORT, if transfer is aborted
 * \param nb_sent    number of data received
 */
static void udi_hid_generic_report_out_received(udd_ep_status_t status,
		iram_size_t nb_received, udd_ep_id_t ep);

/**
 * \brief Enable reception of out report
 *
 * \return \c 1 if function was successfully done, otherwise \c 0.
 */
static bool udi_hid_generic_report_out_enable(void);

/**
 * \brief Callback called when the report is sent
 *
 * \param status     UDD_EP_TRANSFER_OK, if transfer is completed
 * \param status     UDD_EP_TRANSFER_ABORT, if transfer is aborted
 * \param nb_sent    number of data transfered
 */
static void udi_hid_generic_report_in_sent(udd_ep_status_t status,
		iram_size_t nb_sent, udd_ep_id_t ep);

//@}


//--------------------------------------------
//------ Interface for UDI HID level

bool udi_hid_generic_enable(void)
{
	// Initialize internal values
	udi_hid_generic_rate = 0;
	udi_hid_generic_protocol = 0;
	udi_hid_generic_b_report_in_free = true;
	if (!udi_hid_generic_report_out_enable())
		return false;
	return UDI_HID_GENERIC_ENABLE_EXT();
}


void udi_hid_generic_disable(void)
{
	UDI_HID_GENERIC_DISABLE_EXT();
}


bool udi_hid_generic_setup(void)
{
	return udi_hid_setup(&udi_hid_generic_rate,
								&udi_hid_generic_protocol,
								(uint8_t *) &udi_hid_generic_report_desc,
								udi_hid_generic_setreport);
}


uint8_t udi_hid_generic_getsetting(void)
{
	return 0;
}


static bool udi_hid_generic_setreport(void)
{
	if ((USB_HID_REPORT_TYPE_FEATURE == (udd_g_ctrlreq.req.wValue >> 8))
			&& (0 == (0xFF & udd_g_ctrlreq.req.wValue))
			&& (sizeof(udi_hid_generic_report_feature) ==
					udd_g_ctrlreq.req.wLength)) {
		// Feature type on report ID 0
		udd_g_ctrlreq.payload =
				(uint8_t *) & udi_hid_generic_report_feature;
		udd_g_ctrlreq.callback = udi_hid_generic_setfeature_valid;
		udd_g_ctrlreq.payload_size =
				sizeof(udi_hid_generic_report_feature);
		return true;
	}
	return false;
}

//--------------------------------------------
//------ Interface for application

bool udi_hid_generic_send_report_in(uint8_t *data)
{
	if (!udi_hid_generic_b_report_in_free)
		return false;
	irqflags_t flags = cpu_irq_save();
	// Fill report
	memset(&udi_hid_generic_report_in, 0, sizeof(udi_hid_generic_report_in));
	memcpy(&udi_hid_generic_report_in, data, sizeof(udi_hid_generic_report_in));

	udi_hid_generic_b_report_in_free =
			!udd_ep_run(UDI_HID_GENERIC_EP_IN,
							false,
							(uint8_t *) & udi_hid_generic_report_in,
							sizeof(udi_hid_generic_report_in),
							udi_hid_generic_report_in_sent);
	cpu_irq_restore(flags);
	return !udi_hid_generic_b_report_in_free;

}

//--------------------------------------------
//------ Internal routines

static void udi_hid_generic_setfeature_valid(void)
{
	if (sizeof(udi_hid_generic_report_feature) != udd_g_ctrlreq.payload_size)
		return;	// Bad data
	UDI_HID_GENERIC_SET_FEATURE(udi_hid_generic_report_feature);
}

static void udi_hid_generic_report_out_received(udd_ep_status_t status,
		iram_size_t nb_received, udd_ep_id_t ep)
{
	UNUSED(ep);
	if (UDD_EP_TRANSFER_OK != status)
		return;	// Abort reception

	if (sizeof(udi_hid_generic_report_out) == nb_received) {
		UDI_HID_GENERIC_REPORT_OUT(udi_hid_generic_report_out);
	}
	udi_hid_generic_report_out_enable();
}


static bool udi_hid_generic_report_out_enable(void)
{
	return udd_ep_run(UDI_HID_GENERIC_EP_OUT,
							false,
							(uint8_t *) & udi_hid_generic_report_out,
							sizeof(udi_hid_generic_report_out),
							udi_hid_generic_report_out_received);
}


static void udi_hid_generic_report_in_sent(udd_ep_status_t status,
		iram_size_t nb_sent, udd_ep_id_t ep)
{
	UNUSED(status);
	UNUSED(nb_sent);
	UNUSED(ep);
	udi_hid_generic_b_report_in_free = true;
}

//@}
