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

#ifndef PLIC_H
#define PLIC_H

#include <stdint.h>

/* SiFive PLIC Base Address on Microchip PolarFire SoC Icicle Kit */
#define PLIC_BASE                       0x0C000000ULL

/* Standard SiFive PLIC register-block offsets. */
#define PLIC_PRIORITY_OFFSET            0x000000ULL  /* 4 bytes per source   */
#define PLIC_ENABLE_OFFSET              0x002000ULL  /* 0x80 per context     */
#define PLIC_CONTEXT_OFFSET             0x200000ULL  /* 0x1000 per context   */

/*
 * PolarFire SoC PLIC context assignment. The E51 monitor core has machine mode
 * only; each U54 application core contributes a machine-mode context followed
 * by a supervisor-mode one:
 *
 *   0: E51    M     3: U54_2  M     5: U54_3  M     7: U54_4  M
 *   1: U54_1  M     4: U54_2  S     6: U54_3  S     8: U54_4  S
 *   2: U54_1  S
 *
 * ThreadX runs in machine mode on Hart 1 (u54_1), so the correct context is 1.
 * Context 2 is the supervisor-mode context for the same hart: programming it
 * is silently accepted by the PLIC but leaves the machine-mode enable bitmap
 * clear, so MEIP never asserts and no external interrupt is ever delivered.
 */
#define PLIC_HART1_M_CONTEXT            1U

#define PLIC_CONTEXT_BASE(ctx)          (PLIC_BASE + PLIC_CONTEXT_OFFSET + ((uint64_t)(ctx) * 0x1000ULL))
#define PLIC_ENABLE_BASE(ctx)           (PLIC_BASE + PLIC_ENABLE_OFFSET + ((uint64_t)(ctx) * 0x80ULL))

/* Hart 1 Machine-Mode (Context 1) Control Registers */
#define PLIC_HART1_M_THRESHOLD_REG      (*(volatile uint32_t *)(PLIC_CONTEXT_BASE(PLIC_HART1_M_CONTEXT)))
#define PLIC_HART1_M_CLAIM_REG          (*(volatile uint32_t *)(PLIC_CONTEXT_BASE(PLIC_HART1_M_CONTEXT) + 4ULL))

/* Hart 1 Machine-Mode enable bitmap word holding source `irq` (32 sources per word) */
#define PLIC_HART1_M_ENABLE_REG(irq)    (*(volatile uint32_t *)(PLIC_ENABLE_BASE(PLIC_HART1_M_CONTEXT) + (((uint64_t)(irq) / 32ULL) * 4ULL)))

/* Interrupt Source Priority Register (1..186) */
#define PLIC_PRIORITY_REG(irq)          (*(volatile uint32_t *)(PLIC_BASE + PLIC_PRIORITY_OFFSET + ((irq) * 4ULL)))

/* Microchip PolarFire SoC MMUART1 PLIC Source ID */
#define MMUART1_IRQ                     91

void plic_init(void);
uint32_t plic_claim(void);
void plic_complete(uint32_t irq);

#endif /* PLIC_H */
