/*
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available
 * under the terms of the MIT license which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 */

// Portions of this file were generated with AI assistance.

#include "plic.h"

/* Board-level peripheral bring-up, owned by bsp_board_init().
 *
 * The ThreadX system tick is deliberately NOT started here: hwtimer_init() is
 * called from _tx_initialize_low_level so the CLINT comparand is armed
 * immediately before the scheduler starts, which is the single owner of the
 * kernel tick. Keeping the two apart avoids the peripheral being initialised
 * twice on the path main() -> bsp_board_init() -> tx_kernel_enter(). */
void board_init(void) {
    /* Initialize PLIC (Hart 1 Context 2, enable MMUART1 IRQ 91) */
    plic_init();
}
