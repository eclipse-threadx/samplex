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
#include "csr.h"

void plic_init(void) {
    /* Set Priority for MMUART1 (IRQ 91) to 1 */
    PLIC_PRIORITY_REG(MMUART1_IRQ) = 1;

    /* Enable IRQ 91 for Hart 1 Machine Mode (Context 1). Bit position = 91 % 32 = 27 */
    PLIC_HART1_M_ENABLE_REG(MMUART1_IRQ) |= (1U << (MMUART1_IRQ % 32));

    /* Set Priority Threshold for Hart 1 to 0 (allow all non-zero priority interrupts) */
    PLIC_HART1_M_THRESHOLD_REG = 0;

    /* Enable Machine External Interrupts in CPU mie register */
    __asm__ volatile("csrs mie, %0" : : "r"(MIE_MEIE));
}

uint32_t plic_claim(void) {
    return PLIC_HART1_M_CLAIM_REG;
}

void plic_complete(uint32_t irq) {
    PLIC_HART1_M_CLAIM_REG = irq;
}
