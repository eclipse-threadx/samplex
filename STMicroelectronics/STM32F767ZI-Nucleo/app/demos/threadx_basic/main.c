/* 
 * Copyright (c) 2026 Eclipse ThreadX contributors
 * 
 *  This program and the accompanying materials are made available 
 *  under the terms of the MIT license which is available at
 *  https://opensource.org/license/mit.
 * 
 *  SPDX-License-Identifier: MIT
 * 
 *  Contributors:
 *     Ali Eissa - 2026 version.
 */

#include "board_init.h"
#include "tx_api.h"
#include <stdio.h>

/* --- ThreadX Resource Definitions --- */

/* 1. Green LED Blinky Thread */
#define GREEN_THREAD_STACK_SIZE 1024
TX_THREAD green_thread;
uint8_t green_thread_stack[GREEN_THREAD_STACK_SIZE];

/* 2. Button Controller Thread */
#define BUTTON_THREAD_STACK_SIZE 1024
TX_THREAD button_thread;
uint8_t button_thread_stack[BUTTON_THREAD_STACK_SIZE];

/* 3. Logger Thread */
#define LOGGER_THREAD_STACK_SIZE 1024
TX_THREAD logger_thread;
uint8_t logger_thread_stack[LOGGER_THREAD_STACK_SIZE];

/* 4. Keyboard Input Thread */
#define INPUT_THREAD_STACK_SIZE 1024
TX_THREAD input_thread;
uint8_t input_thread_stack[INPUT_THREAD_STACK_SIZE];

/* 5. ThreadX Message Queue */
#define QUEUE_MAX_MESSAGES 16
#define MESSAGE_SIZE_WORDS 1
TX_QUEUE msg_queue;
ULONG queue_buffer[QUEUE_MAX_MESSAGES * MESSAGE_SIZE_WORDS];

/* Thread Function Prototypes */
void green_thread_entry(ULONG thread_input);
void button_thread_entry(ULONG thread_input);
void logger_thread_entry(ULONG thread_input);
void input_thread_entry(ULONG thread_input);

/* External console function declaration */
int __io_getchar(void);

/**
  * @brief  Standard C Main entry point.
  * @retval int
  */
int main(void)
{
    board_init();
    tx_kernel_enter();

    return 0;
}

/**
  * @brief  Define ThreadX resources
  * @param  first_unused_memory: pointer to first free memory block in RAM
  * @retval None
  */
void tx_application_define(void *first_unused_memory)
{
    (void)first_unused_memory;

    /* ThreadX Message Queue */
    tx_queue_create(&msg_queue,
                    "Message Queue",
                    MESSAGE_SIZE_WORDS,
                    queue_buffer,
                    sizeof(queue_buffer));

    /* Thread 1: Blinks Green LED */
    tx_thread_create(&green_thread,
                     "Green LED Thread",
                     green_thread_entry,
                     0,
                     green_thread_stack,
                     GREEN_THREAD_STACK_SIZE,
                     15, /* Lower priority */
                     15,
                     TX_NO_TIME_SLICE,
                     TX_AUTO_START);

    /* Thread 2: Scans User Button */
    tx_thread_create(&button_thread,
                     "Button Thread",
                     button_thread_entry,
                     0,
                     button_thread_stack,
                     BUTTON_THREAD_STACK_SIZE,
                     10, /* Medium priority */
                     10,
                     TX_NO_TIME_SLICE,
                     TX_AUTO_START);

    /* Thread 3: Prints Logs to PC */
    tx_thread_create(&logger_thread,
                     "Logger Thread",
                     logger_thread_entry,
                     0,
                     logger_thread_stack,
                     LOGGER_THREAD_STACK_SIZE,
                     10, /* Medium priority */
                     10,
                     TX_NO_TIME_SLICE,
                     TX_AUTO_START);

    /* Thread 4: Monitors Keyboard Input */
    tx_thread_create(&input_thread,
                     "Input Thread",
                     input_thread_entry,
                     0,
                     input_thread_stack,
                     INPUT_THREAD_STACK_SIZE,
                     5, /* Higher priority */
                     5,
                     TX_NO_TIME_SLICE,
                     TX_AUTO_START);
}

/**
  * @brief  Thread 1: Blinks the Green LED (LD1) continuously to show life.
  * @param  thread_input: unused
  * @retval None
  */
void green_thread_entry(ULONG thread_input)
{
    (void)thread_input;
    
    printf("\r\n==========================================\r\n");
    printf("ThreadX Multitasking Edge Node Demo Booted!\r\n");
    printf("==========================================\r\n\r\n");

    while (1)
    {
        /* Toggles LD1 (Green) on and off every 250 milliseconds */
        LED1_ON();
        tx_thread_sleep(25); /* 25 ticks = 250 ms at 100 Hz tick rate */
        
        LED1_OFF();
        tx_thread_sleep(25);
    }
}

/**
  * @brief  Thread 2: Scans the user button PC13. When pressed, sends the
  *         current tick count over the queue to the Logger thread.
  * @param  thread_input: unused
  * @retval None
  */
void button_thread_entry(ULONG thread_input)
{
    (void)thread_input;
    uint8_t button_was_pressed = 0;

    while (1)
    {
        if (BUTTON_IS_PRESSED)
        {
            if (!button_was_pressed)
            {
                /* Button was just pressed */
                button_was_pressed = 1;
                LED2_ON();
                ULONG current_ticks = tx_time_get();

                /* Non-blocking send the tick count over the queue to Thread 3 */
                tx_queue_send(&msg_queue, &current_ticks, TX_NO_WAIT);
            }
        }
        else
        {
            if (button_was_pressed)
            {
                /* Button was released */
                button_was_pressed = 0;
                LED2_OFF();
            }
        }

        /* 20ms for preventing debounce */
        tx_thread_sleep(2);
    }
}

/**
  * @brief  Thread 3: Listens to the message queue. When a button press message
  *         arrives, it prints a real-time serial status log.
  * @param  thread_input: unused
  * @retval None
  */
void logger_thread_entry(ULONG thread_input)
{
    (void)thread_input;
    ULONG received_ticks;

    while (1)
    {
        /* Block until a message arrives in the queue */
        if (tx_queue_receive(&msg_queue, &received_ticks, TX_WAIT_FOREVER) == TX_SUCCESS)
        {
            printf("[LOG] Button Pressed! System Tick Count: %lu\r\n", received_ticks);
        }
    }
}

/* Circular Ring Buffer for Serial Input (USART3) */
#define RING_BUF_SIZE 256
volatile char ring_buf[RING_BUF_SIZE];
volatile int ring_head = 0;
volatile int ring_tail = 0;

/**
  * @brief  USART3 Interrupt Service Routine (ISR)
  *         This runs in hardware interrupt context. It is incredibly fast,
  *         has zero overhead, and bypasses ThreadX scheduling to safely
  *         and instantly capture incoming serial characters, eliminating
  *         any Overrun Error (ORE) or missing byte issues at 115200 baud.
  */
void USART3_IRQHandler(void)
{
    /* Check RXNE (Receive Data Register Not Empty) flag */
    if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_RXNE))
    {
        /* Read data register directly to clear RXNE flag and get the byte */
        char ch = (char)(huart3.Instance->RDR & 0xFF);
        
        /* Insert into circular ring buffer if not full */
        int next_head = (ring_head + 1) % RING_BUF_SIZE;
        if (next_head != ring_tail)
        {
            ring_buf[ring_head] = ch;
            ring_head = next_head;
        }
    }

    /* Clear any hardware error flags to prevent lockups */
    if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_ORE))
    {
        __HAL_UART_CLEAR_FLAG(&huart3, UART_CLEAR_OREF);
    }
    if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_FE))
    {
        __HAL_UART_CLEAR_FLAG(&huart3, UART_CLEAR_FEF);
    }
    if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_NE))
    {
        __HAL_UART_CLEAR_FLAG(&huart3, UART_CLEAR_NEF);
    }
}

/**
  * @brief  Thread 4: Monitors Keyboard Input
  *         This thread sleeps efficiently when idle. When the background interrupt
  *         captures typed characters, they are processed from the ring buffer.
  *         Once a complete string is received (ended by \r or \n), it prints the
  *         result and flashes the Red LED (LD3).
  * @param  thread_input: unused
  * @retval None
  */
void input_thread_entry(ULONG thread_input)
{
    (void)thread_input;
    static char line_buffer[128];
    static int line_index = 0;

    /* Ring Buffer pointers */
    ring_head = 0;
    ring_tail = 0;

    /* Clear any pending error flags and purge hardware receive register */
    __HAL_UART_CLEAR_FLAG(&huart3, UART_CLEAR_OREF | UART_CLEAR_FEF | UART_CLEAR_NEF);
    (void)huart3.Instance->RDR;

    /* Enable USART3 Interrupts in the Cortex-M7 NVIC */
    HAL_NVIC_SetPriority(USART3_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);

    /* Enable USART3 Hardware Receive Interrupt (RXNEIE) */
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_RXNE);

    while (1)
    {
        /* Process all characters accumulated in the ring buffer by the ISR */
        while (ring_tail != ring_head)
        {
            char ch = ring_buf[ring_tail];
            ring_tail = (ring_tail + 1) % RING_BUF_SIZE;

            /* Check for end of line*/
            if (ch == '\r' || ch == '\n')
            {
                if (line_index > 0)
                {
                    line_buffer[line_index] = '\0';
                    printf(" -> [CONSOLE] Received string: \"%s\"\r\n", line_buffer);

                    /* Flash LD3 to signal successful reception */
                    LED3_ON();
                    tx_thread_sleep(5); /* 50 ms flash (yields control to other threads) */
                    LED3_OFF();

                    line_index = 0;
                }
            }
            else
            {
                /* Standard character: append to buffer if there is space */
                if (line_index < (int)sizeof(line_buffer) - 1)
                {
                    line_buffer[line_index++] = ch;
                }
            }
        }

        /* Yield/Sleep for 1 tick (10ms) when no characters are being processed.
         * This allows all other threads to run smoothly. */
        tx_thread_sleep(1);
    }
}





