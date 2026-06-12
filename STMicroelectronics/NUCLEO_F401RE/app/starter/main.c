/*
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include "tx_api.h"

#include "board_init.h"
#include "cloud_config.h"
#include "nucleo_bsp.h"

#define THREAD_STACK_SIZE 1024

typedef struct {
  ULONG uptime;
  ULONG byte_pool_total;
  ULONG byte_pool_free;
  ULONG byte_pool_used;
} SystemStats;

static SystemStats system_stats = {0};

/* Global Byte Pool for dynamic stack allocation */
static TX_BYTE_POOL byte_pool;

/* Thread Control Blocks */
static TX_THREAD monitor_thread;
static TX_THREAD reporter_thread;
static TX_THREAD blink_thread;
static TX_THREAD worker_thread;

/* Activity counters */
static ULONG monitor_counter = 0;
static ULONG reporter_counter = 0;
static ULONG blink_counter = 0;
static ULONG worker_counter = 0;

/* Monitor Thread: Producer of statistics */
static void monitor_thread_entry(ULONG parameter) {
  (void)parameter;

  while (1) {
    monitor_counter++;

    /* 1. Track system uptime */
    system_stats.uptime = tx_time_get() / TX_TIMER_TICKS_PER_SECOND;

    /* 2. Track dynamic memory allocation stats */
    CHAR *name;
    ULONG available_bytes;
    ULONG fragments;
    TX_THREAD *suspended_thread;
    ULONG suspended_count;
    TX_BYTE_POOL *next_pool;

    if (tx_byte_pool_info_get(&byte_pool, &name, &available_bytes, &fragments,
                              &suspended_thread, &suspended_count,
                              &next_pool) == TX_SUCCESS) {
      system_stats.byte_pool_total = byte_pool.tx_byte_pool_size;
      system_stats.byte_pool_free = available_bytes;
      system_stats.byte_pool_used =
          byte_pool.tx_byte_pool_size - available_bytes;
    }

    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 10); /* Run at 10 Hz (100ms) */
  }
}

/* Reporter Thread: Consumer/Printer of statistics */
static void reporter_thread_entry(ULONG parameter) {
  (void)parameter;

  printf("\r\n==========================================\r\n");
  printf("NUCLEO-F401RE Device Monitor Demo\r\n");
  printf("Third-party licensing info in NOTICE.md\r\n");
  printf("==========================================\r\n");

  while (1) {
    reporter_counter++;

    printf("\r\nSystem Status:\r\n");
    printf("------------------------------------------\r\n");
    printf("Uptime:           %lu s\r\n", system_stats.uptime);
    printf("Byte Pool Size:   %lu bytes\r\n", system_stats.byte_pool_total);
    printf("Allocated Memory: %lu bytes\r\n", system_stats.byte_pool_used);
    printf("Free Memory:      %lu bytes\r\n", system_stats.byte_pool_free);
    printf("------------------------------------------\r\n");
    printf("Runs: Monitor: %lu | Reporter: %lu | Blink: %lu | Worker: %lu\r\n",
           monitor_counter, reporter_counter, blink_counter, worker_counter);

    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 2); /* Report every 2 seconds */
  }
}

static void blink_thread_entry(ULONG parameter) {
  (void)parameter;

  while (1) {
    nucleo_led_toggle();
    blink_counter++;
    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 2); /* Sleep 500ms */
  }
}

static void worker_thread_entry(ULONG parameter) {
  (void)parameter;

  while (1) {
    worker_counter++;
    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 5); /* Sleep 200ms */
  }
}

void tx_application_define(void *first_unused_memory) {
  UINT status;
  CHAR *stack_ptr;
  ULONG pool_size;

  /* Calculate available RAM for the byte pool, leaving 4KB margin for system
   * stack at the top (0x20018000) */
  pool_size = (0x20018000 - 4096) - (ULONG)first_unused_memory;

  /* Initialize the byte pool */
  status = tx_byte_pool_create(&byte_pool, "system byte pool",
                               first_unused_memory, pool_size);
  if (status != TX_SUCCESS) {
    printf("Byte pool create failed: 0x%08x\r\n", status);
    return;
  }

  /* Allocate stack and create Monitor Thread (Priority 9) */
  status = tx_byte_allocate(&byte_pool, (VOID **)&stack_ptr, THREAD_STACK_SIZE,
                            TX_NO_WAIT);
  if (status == TX_SUCCESS) {
    status = tx_thread_create(
        &monitor_thread, "monitor thread", monitor_thread_entry, 0, stack_ptr,
        THREAD_STACK_SIZE, 9, 9, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS) {
      printf("Monitor thread create failed: 0x%08x\r\n", status);
    }
  }

  /* Allocate stack and create Reporter Thread (Priority 10) */
  status = tx_byte_allocate(&byte_pool, (VOID **)&stack_ptr, THREAD_STACK_SIZE,
                            TX_NO_WAIT);
  if (status == TX_SUCCESS) {
    status = tx_thread_create(
        &reporter_thread, "reporter thread", reporter_thread_entry, 0,
        stack_ptr, THREAD_STACK_SIZE, 10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS) {
      printf("Reporter thread create failed: 0x%08x\r\n", status);
    }
  }

  /* Allocate stack and create Blink Thread (Priority 11) */
  status = tx_byte_allocate(&byte_pool, (VOID **)&stack_ptr, THREAD_STACK_SIZE,
                            TX_NO_WAIT);
  if (status == TX_SUCCESS) {
    status = tx_thread_create(&blink_thread, "blink thread", blink_thread_entry,
                              0, stack_ptr, THREAD_STACK_SIZE, 11, 11,
                              TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS) {
      printf("Blink thread create failed: 0x%08x\r\n", status);
    }
  }

  /* Allocate stack and create Worker Thread (Priority 12) */
  status = tx_byte_allocate(&byte_pool, (VOID **)&stack_ptr, THREAD_STACK_SIZE,
                            TX_NO_WAIT);
  if (status == TX_SUCCESS) {
    status = tx_thread_create(
        &worker_thread, "worker thread", worker_thread_entry, 0, stack_ptr,
        THREAD_STACK_SIZE, 12, 12, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS) {
      printf("Worker thread create failed: 0x%08x\r\n", status);
    }
  }
}

int main(void) {
  board_init();

  /* Start the ThreadX kernel */
  tx_kernel_enter();

  while (1) {
  }
}
