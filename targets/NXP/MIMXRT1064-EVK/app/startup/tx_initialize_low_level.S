/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 *
 * Contributors:
 *    Ali Eissa - 2026 version.
 **************************************************************************/

// Portions of this file were generated with AI assistance.

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** ThreadX Component                                                     */
/**                                                                       */
/**   Initialize                                                          */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

    .global     _tx_thread_system_stack_ptr
    .global     _tx_initialize_unused_memory
    .global     __RAM_segment_used_end__
    .global     _tx_timer_interrupt
    .global     _vectors
    .global     __tx_NMIHandler                     // NMI
    .global     __tx_BadHandler                     // HardFault
    .global     __tx_SVCallHandler                  // SVCall
    .global     __tx_DBGHandler                     // Monitor
    .global     __tx_PendSVHandler                  // PendSV
    .global     __tx_SysTickHandler                 // SysTick
    .global     __tx_IntHandler                     // Int 0

SYSTICK_CYCLES_HW      =   ((600000000 / 100) - 1)     // 5,999,999 cycles: 600 MHz on real silicon -> 100 Hz
SYSTICK_CYCLES_RENODE  =   ((72000000 / 100) - 1)      //   719,999 cycles: 72 MHz in Renode NVIC -> 100 Hz

    .text
    .align 4
    .syntax unified

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _tx_initialize_low_level                          Cortex-M7/GNU     */
/*                                                           6.4.0        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function is responsible for any low-level processor            */
/*    initialization, including setting up interrupt vectors, setting     */
/*    up a periodic timer interrupt source, saving the system stack       */
/*    pointer for use in ISR processing later, and finding the first      */
/*    available RAM memory address for tx_application_define.             */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    _tx_initialize_kernel_enter           ThreadX entry function        */
/*                                                                        */
/**************************************************************************/
    .global  _tx_initialize_low_level
    .thumb_func
_tx_initialize_low_level:

    /* Disable interrupts during ThreadX initialization.  */
    CPSID   i

    /* Set base of available memory to end of non-initialised RAM area.  */
    LDR     r0, =_tx_initialize_unused_memory       // Build address of unused memory pointer
    LDR     r1, =__RAM_segment_used_end__           // Build first free address
    ADD     r1, r1, #4                              //
    STR     r1, [r0]                                // Setup first unused memory pointer

    /* Setup Vector Table Offset Register.  */
    MOV     r0, #0xE000E000                         // Build address of NVIC registers
    LDR     r1, =_vectors                           // Pickup address of vector table
    STR     r1, [r0, #0xD08]                        // Set vector table address

    /* Set system stack pointer from vector value.  */
    LDR     r0, =_tx_thread_system_stack_ptr        // Build address of system stack pointer
    LDR     r1, =_vectors                           // Pickup address of vector table
    LDR     r1, [r1]                                // Pickup reset stack pointer
    STR     r1, [r0]                                // Save system stack pointer

    /* Enable the DWT cycle count register if DWT hardware is present (physical silicon).  */
    LDR     r1, =0xE000ED90                         // MPU->TYPE register
    LDR     r1, [r1]
    LSRS    r1, r1, #8
    AND     r1, r1, #0xFF                           // Extract DREGION field
    CMP     r1, #12                                 // Physical silicon has 16 regions, Renode has 8
    BLT     .Lskip_dwt
    LDR     r0, =0xE0001000                         // Build address of DWT register
    LDR     r1, [r0]                                // Pickup the current value
    ORR     r1, r1, #1                              // Set the CYCCNTENA bit
    STR     r1, [r0]                                // Enable the cycle count register
.Lskip_dwt:

    /* Configure SysTick reload: detect Renode (72 MHz NVIC clock) vs physical silicon (600 MHz core clock).  */
    LDR     r1, =0xE000ED90                         // MPU->TYPE register
    LDR     r1, [r1]
    LSRS    r1, r1, #8
    AND     r1, r1, #0xFF                           // Extract DREGION field
    CMP     r1, #12
    BLT     .Lrenode_systick
    LDR     r1, =SYSTICK_CYCLES_HW                  // 600 MHz clock -> reload for 100 Hz
    B       .Lset_systick
.Lrenode_systick:
    LDR     r1, =SYSTICK_CYCLES_RENODE              // 72 MHz clock -> reload for 100 Hz
.Lset_systick:
    MOV     r0, #0xE000E000                         // Build address of NVIC registers
    STR     r1, [r0, #0x14]                         // Setup SysTick Reload Value
    MOV     r1, #0x7                                // Build SysTick Control Enable Value (CLKSOURCE|TICKINT|ENABLE)
    STR     r1, [r0, #0x10]                         // Setup SysTick Control

    /* Configure handler priorities (upper 4 bits implemented on Cortex-M7, mask 0xF0).  */
    LDR     r1, =0x00000000                         // Rsrv, UsgF, BusF, MemM
    STR     r1, [r0, #0xD18]                        // Setup System Handlers 4-7 Priority Registers
    LDR     r1, =0xF0000000                         // SVCl, Rsrv, Rsrv, Rsrv
    STR     r1, [r0, #0xD1C]                        // Setup System Handlers 8-11 Priority Registers
                                                    // Note: SVC must be lowest priority (0xF0 for 4-bit NVIC)
    LDR     r1, =0x40F00000                         // SysT (0x40), PnSV (0xF0), Rsrv, DbgM
    STR     r1, [r0, #0xD20]                        // Setup System Handlers 12-15 Priority Registers
                                                    // Note: PnSV must be lowest priority (0xF0 for 4-bit NVIC)

    /* Return to caller.  */
    BX      lr

/* Define shells for each of the unused vectors.  */
    .global  __tx_BadHandler
    .thumb_func
__tx_BadHandler:
    B       __tx_BadHandler

/* Catch HardFault */
    .global  __tx_HardfaultHandler
    .thumb_func
__tx_HardfaultHandler:
    B       __tx_HardfaultHandler

/* Catch SVC */
    .global  __tx_SVCallHandler
    .thumb_func
__tx_SVCallHandler:
    B       __tx_SVCallHandler

/* Generic interrupt handler template */
    .global  __tx_IntHandler
    .thumb_func
__tx_IntHandler:
    PUSH    {r0, lr}
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    BL      _tx_execution_isr_enter             // Call the ISR enter function
#endif
    /* Do interrupt handler work here */
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    BL      _tx_execution_isr_exit              // Call the ISR exit function
#endif
    POP     {r0, lr}
    BX      lr

/* System Tick timer interrupt handler */
    .global  __tx_SysTickHandler
    .global  SysTick_Handler
    .thumb_func
__tx_SysTickHandler:
    .thumb_func
SysTick_Handler:
    PUSH    {r0, lr}
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    BL      _tx_execution_isr_enter             // Call the ISR enter function
#endif
    BL      _tx_timer_interrupt
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    BL      _tx_execution_isr_exit              // Call the ISR exit function
#endif
    POP     {r0, lr}
    BX      lr

/* NMI, DBG handlers */
    .global  __tx_NMIHandler
    .thumb_func
__tx_NMIHandler:
    B       __tx_NMIHandler

    .global  __tx_DBGHandler
    .thumb_func
__tx_DBGHandler:
    B       __tx_DBGHandler
