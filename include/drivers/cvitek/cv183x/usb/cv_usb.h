/*
 * Copyright (c) 2015-2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __CV_USB_H
#define __CV_USB_H

#include <sys/types.h>

#define BCD_USB_SS					0x0300  // 3.00 version USB
#define ID_VENDOR					0x30B1  // Bitmain
#define ID_PRODUCT					0x1000  // example bulk product
#define BCD_DEVICE_SS				0x0001  // 0.1
#define BCD_USB_HS_ONLY				0x0201  // 2.01  /*Only HS with BOS descriptor*/
#define BCD_USB_HS					0x0210  // 2.10
#define BCD_DEVICE_HS				0x0200  // 2.00

#define USB_MANUFACTURER_STRING		"BITMAIN"
#define USB_PRODUCT_STRING			"USB Com Port"
#define USB_SERIAL_NUMBER_STRING		"123456789ABC" // should 12 chars long

#define BULK_EP_IN 0x81
#define BULK_EP_OUT 0x01
#define BULK_EP_NOTIFY 0x82
#define HEADER_SIZE 8

#define SS_PERIPH_DISABLED_SET	(1 << 21)
#define HOST_BUS_DROP			(1 << 9)
#define DEV_BUS_REQ				(1)

typedef enum {
	BM_USB_NONE = 0,
	BM_USB_INFO,
	BM_USB_VERBOSE,
	BM_USB_JUMP,
	BM_USB_BREAK,
	BM_USB_KEEP_DL,
	BM_USB_NONE_FAST = 20, // For BM1880V2 only
	BM_USB_JUMP_ACK
} BM_USB_TOKEN;

typedef struct _bld_header_t {
	uint32_t signature;
	uint32_t version;
	uint32_t length;
} bld_header_t;

typedef void func(void);

#define PD_SIZE		4096
#define BUF_SIZE	1024
#define EP0_SIZE	512
#define TRB_SIZE	256
#define CB_SIZE		128
#define RSP_SIZE	16

#define PD_ADDR			BM_USB_BUF_BASE				// 4096
#define BLK_BUF_ADDR	(PD_ADDR + PD_SIZE)			// 1024
#define CMD_BUF_ADDR	(BLK_BUF_ADDR + BUF_SIZE)	// 1024
#define TRB_BUF_ADDR	(CMD_BUF_ADDR + BUF_SIZE)	// 256
#define CB0_BUF_ADDR	(TRB_BUF_ADDR + TRB_SIZE)	// 128
#define CB1_BUF_ADDR	(CB0_BUF_ADDR + CB_SIZE)	// 128
#define CB2_BUF_ADDR	(CB1_BUF_ADDR + CB_SIZE)	// 128
#define EP0_BUF_ADDR	(CB2_BUF_ADDR + CB_SIZE)	// 512
#define ACM_BUF_ADDR	(EP0_BUF_ADDR + EP0_SIZE)	// 128
#define RSP_BUF_ADDR	(ACM_BUF_ADDR + CB_SIZE)	// 16

#define USB_PHY_VBUS_VALID			BIT(0)
#define USB_PHY_ID_OVERRIDE_ENABLE	BIT(6)
#define USB_PHY_ID_VALUE			BIT(7)

#define DISABLE_TIMEOUT	99999
#endif // __CV_USB_H
