/*
 * Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __TBBR_IMG_DEF_H__
#define __TBBR_IMG_DEF_H__

/* Firmware Image Package */
#define FIP_IMAGE_ID			0

/* Trusted Boot Firmware BL2 */
#define BL2_IMAGE_ID			1

/* SCP Firmware SCP_BL2 */
#define BLD_IMAGE_ID		2

/* EL3 Runtime Firmware BL31 */
#define BL31_IMAGE_ID			3

/* Secure Payload BL32 (Trusted OS) */
#define BL32_IMAGE_ID			4

/* Non-Trusted Firmware BL33 */
#define BL33_IMAGE_ID			5

/* Certificates */
#define TRUSTED_BOOT_FW_CERT_ID		6
#define BLD1_KEY_CERT_ID		7

#define BLD2_KEY_CERT_ID		8
#define SOC_FW_KEY_CERT_ID		9
#define TRUSTED_OS_FW_KEY_CERT_ID	10
#define BL33_KEY_CERT_ID	11

#define BLD_CONTENT_CERT_ID		12
#define SOC_FW_CONTENT_CERT_ID		13
#define TRUSTED_OS_FW_CONTENT_CERT_ID	14
#define BL33_CONTENT_CERT_ID	15

/* Non-Trusted ROM Firmware NS_BL1U */
#define NS_BL1U_IMAGE_ID		16

/* Trusted FWU Certificate */
#define FWU_CERT_ID			17

/* Trusted FWU SCP Firmware SCP_BL2U */
#define SCP_BL2U_IMAGE_ID		18

/* Trusted FWU Boot Firmware BL2U */
#define BL2U_IMAGE_ID			19

/* Non-Trusted FWU Firmware NS_BL2U */
#define NS_BL2U_IMAGE_ID		20

/* Programmer.bin image */
#define PRG_BIN_IMAGE_ID		21

/* CV privated */
#define CV_BLPARAM_IMAGE_ID 22
#define CV_DDRC_IMAGE_ID 23

/* CV keys */
#define CV_TRUSTED_KEY_CERT_ID 30
#define CV_NON_TRUSTED_KEY_CERT_ID 31

#define CV_LICENSE_FILE_IMAGE_ID 40

#endif /* __TBBR_IMG_DEF_H__ */