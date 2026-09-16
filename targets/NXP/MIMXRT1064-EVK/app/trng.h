/*
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available 
 * under the terms of the MIT license which is available at
 * https://opensource.org/license/mit.
 *
 * SPDX-License-Identifier: MIT
 *
 * Contributors:
 *    Ali Eissa - 2026 version.
 */

// Portions of this file were generated with AI assistance.

#ifndef TRNG_H
#define TRNG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the on-chip True Random Number Generator (TRNG) peripheral.
 * Enables TRNG peripheral clock gating and initializes default sampling parameters.
 *
 * @return 0 on success, non-zero on error.
 */
int trng_init(void);

/**
 * @brief Read a single 32-bit hardware random word from TRNG entropy registers.
 *
 * @param[out] random_val Pointer to uint32_t to receive the random word.
 * @return 0 on success, non-zero on error or timeout.
 */
int trng_get_random_u32(uint32_t *random_val);

/**
 * @brief Fill a buffer with hardware random bytes from TRNG entropy registers.
 *
 * @param[out] buffer Output buffer to receive random bytes.
 * @param[in] length Number of bytes to generate.
 * @return Number of bytes filled on success, or negative on error.
 */
int trng_get_random_data(void *buffer, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* TRNG_H */
