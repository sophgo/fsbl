/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __TPU_SECURITY_H__
#define __TPU_SECURITY_H__

#include <tpu_security_def.h>
#include "cvsp_private.h"

enum {
	CV_DECRYPT_CMD = 1 << 0,
	CV_DECRYPT_WEIGHT = 1 << 1
};

typedef struct {
	void *buf;
	size_t size;
	uint8_t tag[AI_MODEL_TAG_SIZE];
	uint8_t nonce[AI_MODEL_NONCE_SIZE];
} aimodel_buf_t;

typedef struct {
	aimodel_buf_t cmd;
	aimodel_buf_t weight;
	uint8_t fingerprint[AI_MODEL_FINGERPRINT_SIZE];
	uint8_t vdr_ai_pwd[AI_MODEL_RSA_N_SIZE];
	uint8_t sig_vdr_ai_pwd[AI_MODEL_RSA_N_SIZE];
} aimodel_t;

typedef struct {
	uint8_t ai_mdl_ek[AI_MODEL_KEY_SIZE];

	uint8_t vdr_ai_ek[AI_MODEL_KEY_SIZE];

	uint8_t vdr_ai_N[AI_MODEL_RSA_N_SIZE];
	uint8_t vdr_ai_E[AI_MODEL_RSA_E_SIZE];

	uint8_t cv_ai_N[AI_MODEL_RSA_N_SIZE];
	uint8_t cv_ai_P[AI_MODEL_RSA_N_SIZE / 2];
	uint8_t cv_ai_Q[AI_MODEL_RSA_N_SIZE / 2];
	uint8_t cv_ai_D[AI_MODEL_RSA_N_SIZE];
	uint8_t cv_ai_E[AI_MODEL_RSA_E_SIZE];
} tpu_keys_t;

int tpu_set_keyfile(const char *buf, size_t buf_size);
int tpu_aimodel_decrypt_verify(aimodel_t *model, uint8_t *output_cmd, uint8_t *weight_cmd, uint32_t flag);

bmsp_args_t *cvsp_privte_tpu_send_keys_handler(uint64_t func, uint64_t arg1, uint64_t arg2, uint64_t arg3,
					       uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7);
bmsp_args_t *cvsp_privte_tpu_load_model_handler(uint64_t func, uint64_t arg1, uint64_t arg2, uint64_t arg3,
						uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7);
bmsp_args_t *cvsp_private_tpu_run_handler(uint64_t func, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
					  uint64_t arg5, uint64_t arg6, uint64_t arg7);
bmsp_args_t *cvsp_privte_tpu_wait_handler(uint64_t func, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
					  uint64_t arg5, uint64_t arg6, uint64_t arg7);
bmsp_args_t *cvsp_privte_tpu_set_prot_range_handler(
	uint64_t func, uint64_t arg1, uint64_t arg2, uint64_t arg3,
	uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7);
bmsp_args_t *cvsp_privte_tpu_clear_prot_range_handler(
	uint64_t func, uint64_t arg1, uint64_t arg2, uint64_t arg3,
	uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7);
#endif /* __TPU_SECURITY_H__ */
