<!--
  Copyright (c) 2026 Eclipse ThreadX contributors
 
  This program and the accompanying materials are made available 
  under the terms of the MIT license which is available at
  https://opensource.org/license/mit.
 
  SPDX-License-Identifier: MIT
 
  Contributors: 
      Eclipse ThreadX contributors - Initial version and validation.
-->

# Eclipse ThreadX NUCLEO-F401RE Starter Application

This demonstration application validates the NUCLEO-F401RE Board Support Package for Eclipse ThreadX.

The demo creates multiple threads, blinks the onboard LED, and reports runtime statistics through USART2 using the ST-LINK virtual COM port.

It integrates the Eclipse ThreadX RTOS with the STM32CubeF4 HAL library in a clean, self-contained CMake build structure.

Third-party licensing information is provided in [NOTICE.md](NOTICE.md).

## Building the Demo Application

### Prerequisites
- Install a GNU Arm Embedded Toolchain with `arm-none-eabi-` prefix
- Common source: [Arm GNU Toolchain Downloads](https://developer.arm.com/Tools-and-Software/open-source-software-developer-tools/gnu-toolchain)

Verify the toolchain:
```bash
arm-none-eabi-gcc --version
arm-none-eabi-objdump --version
```

### CMake-Based Build
From the `STMicroelectronics/NUCLEO_F401RE` directory:

```bash
cmake -Bbuild -GNinja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-cortex-m4.cmake .
cmake --build ./build/
```

This uses `cmake/arm-gcc-cortex-m4.cmake` and the top-level `CMakeLists.txt` to configure the cross-compiler flags and produce the target binary image.

### Build Scripts
The package includes build scripts under the `scripts/` directory for convenience:
- `scripts/build.ps1` (Windows PowerShell)
- `scripts/build.sh` (Linux Bash)

These scripts clean the build directory, run CMake configuration, and compile the target executable.

## Flashing the Application

Connect the NUCLEO-F401RE board via the ST-LINK USB connector.

Copy:

  build/app/nucleo_f401re.bin

to the mounted NUCLEO-F401RE mass-storage device.

The board automatically resets and starts execution.

To view console output:
- Open a serial terminal (such as PuTTY or Tera Term)
- Select the ST-LINK Virtual COM Port (COM3 in this case)
- Configure 115200 baud, 8-N-1

## Hardware Configuration

The starter application is preconfigured to work with the physical STMicroelectronics NUCLEO-F401RE board:
- **MCU**: Single-core ARM Cortex-M4 running at 84 MHz
- **Onboard User LED**: LD2 connected to pin `PA5` for heartbeat feedback
- **UART Console**: USART2 connected to the ST-Link virtual COM port on pins `PA2` (TX) and `PA3` (RX) configured for 115200 baud, 8N1
- **HAL Timebase Tick**: TIM2 General Purpose Timer configured for 1ms intervals (SysTick remains owned by ThreadX)

### Clock Tree Configuration
System clock is configured via the PLL:
- **Input source**: HSE bypass (8 MHz from ST-Link MCO pin)
- **PLL configuration**: M = 8, N = 336, P = 4, Q = 7
- **SYSCLK**: 84 MHz (maximum frequency of the STM32F401)
- **HCLK (AHB)**: 84 MHz
- **PCLK1 (APB1)**: 42 MHz (peripheral limit)
- **PCLK2 (APB2)**: 84 MHz
- **Flash Latency**: 2 Wait States (`FLASH_LATENCY_2`)

### TIM2 Configuration
TIM2 is configured for the HAL 1ms timebase tick:
- **Timer clock source**: 84 MHz (from APB1 timer branch)
- **Prescaler**: `83` (reduces timer frequency to 1 MHz)
- **Period**: `999` (counts 1000 cycles at 1 MHz, triggering an interrupt every 1ms)
- **Handler**: `TIM2_IRQHandler` triggers update event to increment `uwTick`


## Interrupt Handling & Priority Configuration

All system exceptions and hardware peripheral interrupts trap to vector addresses managed by the Nested Vectored Interrupt Controller (NVIC).

## Interrupt Configuration

The BSP separates the ThreadX scheduler tick from the STM32 HAL timebase to avoid conflicts between the RTOS and HAL timing services.

- **SysTick** (ThreadX, Priority 4)
  - Provides the ThreadX scheduler tick (100 Hz).

- **PendSV** (ThreadX, Priority 15)
  - Performs context switching between threads.

- **SVCall** (ThreadX, Priority 15)
  - Used during ThreadX kernel startup.

- **TIM2** (STM32 HAL, Priority 15)
  - Provides the HAL 1 ms timebase tick.

- **USART2**
  - Operates in polling mode and does not use interrupts in this demonstration.

### HAL Timebase Integration 

ThreadX requires exclusive ownership of the SysTick exception for scheduler operation. To avoid conflicts, the STM32 HAL timebase is implemented using TIM2 instead of the default SysTick source.

The BSP overrides `HAL_InitTick()` to configure TIM2 as the HAL timebase and provides custom implementations of `HAL_SuspendTick()` and `HAL_ResumeTick()` in `board_init.c`. These functions enable and disable the TIM2 update interrupt without affecting the ThreadX scheduler tick.


## File Organization

- **`app/`**: Main application logic and boot setups
  - **`app/common/`**: Console serial driver and board configurations
  - **`app/common/startup/`**: Linker scripts and assembly boot startup code
  - **`app/starter/`**: Demo main thread logic
- **`lib/`**: Libraries and dependencies
  - **`lib/nucleo_bsp/`**: Board specific support libraries (LD2 LED)
  - **`lib/stm32cubef4/`**: HAL Driver wrapper and platform drivers
  - **`lib/threadx/`**: ThreadX user configuration file (`tx_user.h`)
- **`cmake/`**: Cross-compilation module definitions
- **`scripts/`**: Utility build automation scripts


## Validation Record

### Verification Environment
- **Toolchain**: Arm GNU Toolchain 15.2.Rel1 (GCC 15.2.1)
- **Static ROM usage**: 18244 Bytes (3.48% of 512 KB Flash)
- **Static RAM usage**: 5632 Bytes (5.73% of 96 KB RAM)
- **Dynamic Stack & Buffer allocation**: Stacks (8 x 1024 bytes) and Queue buffer (40 bytes) are dynamically allocated from the `TX_BYTE_POOL` (consuming 8312 bytes total, including pool headers).
- **Board Hardware**: NUCLEO-F401RE

### Hardware Verification Checklist
- [x] **Board boots successfully**: System clock configuration correctly executes and sets SysClk to 84 MHz.
- [x] **LED heartbeat operates correctly**: Onboard user LED LD2 (PA5) blinks at a steady rate.
- [x] **UART console operates at 115200 baud**: Diagnostic outputs are cleanly transmitted over USART2 and displayed in the terminal.
- [x] **ThreadX scheduler runs correctly**: RTOS scheduler successfully handles 8 threads with priorities ranging from 9 to 15.
- [x] **Thread Registry & Traversal**: Monitor thread queries thread names, states, run counts, and priorities dynamically using `tx_thread_info_get()`.
- [x] **Stack High-Water Mark Profiling**: Dynamic upward memory scanning verifies stack peaks (e.g., reporter thread at 42%, other tasks at 11-14%).
- [x] **RTOS Primitives Showcase**: Verified mutex lock hand-offs under contention, queue message transfers, and event flag synchronization.
- [x] **Runtime monitor output verified on hardware**: UART serial output prints thread diagnostics table and showcase counters every 2 seconds:

```text
System Status:
------------------------------------------
Uptime:           257 s
Byte Pool Size:   88572 bytes
Allocated Memory: 8312 bytes
Free Memory:      80260 bytes
------------------------------------------

Thread Name      Priority State      Run Count    Stack Peak (Max / Size)
----------------------------------------------------------------------------
monitor thread   9        READY      2575          120 / 1024 bytes (11%)
reporter thread  10       SLEEP      256           432 / 1024 bytes (42%)
blink thread     11       SLEEP      488           136 / 1024 bytes (13%)
event thread     12       EVENT_FLAG 488           128 / 1024 bytes (12%)
mutex thread 1   13       SLEEP      7319          144 / 1024 bytes (14%)
mutex thread 2   13       READY      7319          144 / 1024 bytes (14%)
queue sender     14       READY      1220          128 / 1024 bytes (12%)
queue receiver   15       QUEUE_SUSP 1220          128 / 1024 bytes (12%)
----------------------------------------------------------------------------
Runs: Monitor: 2576 | Reporter: 123 | Blink: 488
RTOS Showcase: Mutex Locks: 2440/2440 | Queue Msgs: 1220 | Event Wakes: 488
```


## Revision History

```
06-06-2026  Eclipse ThreadX contributors
            Initial NUCLEO_F401RE BSP integration and validation.
```
