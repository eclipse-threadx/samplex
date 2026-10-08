/* 
 * Copyright (c) Microsoft
 * Copyright (c) 2024 Eclipse Foundation
 * Copyright (c) 2026 Eclipse ThreadX contributors
 * 
 *  This program and the accompanying materials are made available 
 *  under the terms of the MIT license which is available at
 *  https://opensource.org/license/mit.
 * 
 *  SPDX-License-Identifier: MIT
 * 
 *  Contributors: 
 *     Microsoft         - Initial version
 *     Frédéric Desbiens - 2024 version.
 */
// Portions of this file were generated with AI assistance.
#ifndef _SNTP_CLIENT_H
#define _SNTP_CLIENT_H

#include <tx_api.h>
#include <stdint.h>

ULONG sntp_time_get();
/* Unix milliseconds, or zero until a successful sync. Call at least once per tick wrap. */
uint64_t sntp_time_get_ms(void);
UINT sntp_time(ULONG* unix_time);

UINT sntp_init();
UINT sntp_sync();

#endif
