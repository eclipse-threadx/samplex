/**************************************************************************/
/*  Copyright (c) Microsoft                                               */
/*  Copyright (c) 2026 Eclipse ThreadX contributors                        */
/*                                                                        */
/*  This program and the accompanying materials are made available        */
/*  under the terms of the MIT license which is available at              */
/*  https://opensource.org/license/mit.                                   */
/*                                                                        */
/*  SPDX-License-Identifier: MIT                                          */
/*                                                                        */
/*  Contributors:                                                         */
/*     Ali Eissa - 2026 version.                                          */
/**************************************************************************/

#ifndef TX_USER_H
#define TX_USER_H

/* Enable hardware FPU register context switching support for Cortex-M7 */
#define TX_ENABLE_FPU_SUPPORT

/* System tick frequency in Hz (typically 100 or 1000) */
#ifndef TX_TIMER_TICKS_PER_SECOND
#define TX_TIMER_TICKS_PER_SECOND 100
#endif

/* Enable ThreadX runtime stack checking */
#define TX_ENABLE_STACK_CHECKING

#endif /* TX_USER_H */
