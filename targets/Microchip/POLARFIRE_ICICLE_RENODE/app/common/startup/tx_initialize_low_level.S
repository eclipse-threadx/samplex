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

#include "csr.h"

    .section .data
    .global  __tx_free_memory_start
__tx_free_memory_start:

    .section .text
    .align 4
    .global trap_entry
    .extern trap_handler
    .extern _tx_thread_context_save
    .extern _tx_thread_context_restore

trap_entry:
    /* Read trap cause, program counter, and trap value */
    csrr    a0, mcause
    csrr    a1, mepc
    csrr    a2, mtval

    /* Check if trap is synchronous (bit 63 is 0) */
    bgez    a0, .synchronous_fault

    /* Asynchronous Interrupt (CLINT Timer) - Save ThreadX context */
#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    addi    sp, sp, -520                 // 65*8 with floating point
#else
    addi    sp, sp, -256                 // 32*8 without floating point
#endif
    sd      x1, 224(sp)                  // Save RA at offset 224 (28*8)

    call    _tx_thread_context_save

    csrr    a0, mcause
    csrr    a1, mepc
    csrr    a2, mtval
    addi    sp, sp, -8
    sd      ra, 0(sp)
    call    trap_handler
    ld      ra, 0(sp)
    addi    sp, sp, 8

    call    _tx_thread_context_restore

_err_hang:
    wfi
    j       _err_hang

.synchronous_fault:
    /* Synchronous exception / fatal trap: Call trap_handler directly */
    addi    sp, sp, -8
    sd      ra, 0(sp)
    call    trap_handler
    ld      ra, 0(sp)
    addi    sp, sp, 8
.halt_loop:
    wfi
    j       .halt_loop

    .section .text
    .global  _tx_initialize_low_level
    .weak    _tx_initialize_low_level
    .extern  _end
    .extern  hwtimer_init

_tx_initialize_low_level:
    /* Save the system stack pointer */
    la      t0, _tx_thread_system_stack_ptr
    sd      sp, 0(t0)

    /* Pickup first free memory address */
    la      t0, _end
    la      t1, _tx_initialize_unused_memory
    sd      t0, 0(t1)

    /* Configure machine interrupt registers */
    li      t0, MSTATUS_MIE
    csrrc   zero, mstatus, t0
    li      t0, (MSTATUS_MPP_M | MSTATUS_MPIE)
    csrrs   zero, mstatus, t0
    li      t0, (MIE_MTIE | MIE_MEIE)
    csrrs   zero, mie, t0

#ifdef __riscv_flen
    li      t0, MSTATUS_FS
    csrrs   zero, mstatus, t0
    fscsr   x0
#endif

    /* Arm the CLINT machine timer that drives the ThreadX system tick.
     * Board peripherals are brought up separately by bsp_board_init(). */
    addi    sp, sp, -8
    sd      ra, 0(sp)
    call    hwtimer_init
    ld      ra, 0(sp)
    addi    sp, sp, 8

    /* Set Machine Trap-Vector Base Address to trap_entry */
    la      t0, trap_entry
    csrw    mtvec, t0

    ret
