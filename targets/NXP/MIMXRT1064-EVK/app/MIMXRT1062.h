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

/*
 * Compatibility header: redirects MIMXRT1062.h from stock NetX Duo driver
 * to MIMXRT1064 device registers without modifying vendor source files.
 */
#ifndef MIMXRT1062_H
#define MIMXRT1062_H

#include "fsl_device_registers.h"

/*
 * Assign distinct MAC addresses to server and client nodes
 * to prevent address collision on the Renode virtual switch.
 */
#if defined(NETX_CLIENT_NODE)
#define NX_DRIVER_ETHERNET_MAC {0x02, 0x11, 0x22, 0x33, 0x44, 0x53}
#else
#define NX_DRIVER_ETHERNET_MAC {0x02, 0x11, 0x22, 0x33, 0x44, 0x52}
#endif

#endif /* MIMXRT1062_H */
