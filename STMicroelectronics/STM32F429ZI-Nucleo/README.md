# STM32F429ZI-Nucleo Board Enablement Demo

This directory contains the Board Support Package (BSP) static library (`board_bsp`) and build configurations for running the **Eclipse ThreadX RTOS** on the **STMicroelectronics NUCLEO-F429ZI** evaluation board (ARM Cortex-M4).

The project features a decoupled static BSP library that hides all low-level hardware initializations (clocks, GPIO, DMA descriptors, and interrupts) from the application code.

---

## Supported Demos

Use the `ACTIVE_DEMO` CMake variable to select which demo to compile:

| Demo Name | Description | Targets |
| :--- | :--- | :--- |
| **`threadx_basic`** *(Default)* | Multi-threaded RTOS demo showcasing thread scheduling, mechanical button debouncing, message queue logging, and interrupt-driven UART ring buffers. | ThreadX |


## Hardware Configuration

* **Development Board**: NUCLEO-F429ZI (Nucleo-144)
* **Microcontroller**: STM32F429ZIT6 (ARM Cortex-M4 running at 168 MHz)
* **Memory**: 2 MB Flash, 256 KB SRAM
* **Virtual COM Port**: USART3 (PD8/PD9) connected to ST-LINK debugger (115,200 baud, 8N1)
* **User Button**: PC13 (Blue button, Active High)
* **Board LEDs**:
  * `LD1` (Green) - PB0: Blinks to indicate ThreadX heartbeat.
  * `LD2` (Blue) - PB7
  * `LD3` (Red) - PB14
---

## Demo Application Architecture

The application runs a multi-threaded demo showcasing cooperation between ThreadX scheduling, queues, hardware interrupts, and status indicator peripherals:

1. **Thread 1 (Green LED - Heartbeat)** (Priority 15):
   * Toggles the green LED (`LD1`) continuously at `2 Hz` (250 ms active, 250 ms idle) to verify basic scheduler clock ticks.
2. **Thread 2 (User Button Scanner)** (Priority 10):
   * Scans the blue User Button (`PC13`) using software-level mechanical debouncing (20 ms).
   * Turns the Blue LED (`LD2`) ON when the button is held.
   * Sends the current system tick count over a **ThreadX Message Queue** on a button press transition.
3. **Thread 3 (System Logger)** (Priority 10):
   * Blocks efficiently on the Message Queue.
   * Wakes up when a button press event is queued and prints a timestamped system log over the serial port.
4. **Thread 4 (Serial Terminal Input)** (Priority 5):
   * Integrates an asynchronous, non-blocking **Interrupt Service Routine (ISR)** (`USART3_IRQHandler`) and a 256-byte volatile circular **ring buffer** to read keyboard input at 115,200 baud.
   * Collects incoming characters, prints the received string on carriage return/newline, and flashes the Red LED (`LD3`) for 50 ms.
   * Avoids the use of `HAL_GetTick()` to prevent CPU starvation when idle.

---

## Prerequisites

Before building, ensure you have the following cross-compilation tools installed on your system PATH:

* **ARM GNU Toolchain** (`arm-none-eabi-gcc`)
* **CMake** (version 3.10 or higher)
* **Ninja** or **Make**
* **Git** (for downloading SDK dependencies)

---

## Quick Start Guide

### 1. Download SDK Dependencies
Run the driver fetcher script to clone official, stock STMicroelectronics HAL drivers, CMSIS files, and driver configurations locally:

* **On Windows (PowerShell)**:
  ```powershell
  powershell -ExecutionPolicy Bypass -File .\scripts\fetch_sdk.ps1
  ```
* **On Linux (Bash)**:
  ```bash
  chmod +x ./scripts/fetch_sdk.sh
  ./scripts/fetch_sdk.sh
  ```

### 2. Configure and Build the Project

#### Option A: Build `threadx_basic` (Default)
```bash
# Configure CMake
cmake -DACTIVE_DEMO=threadx_basic -B build

# Compile
cmake --build build
```

The raw binary files will be generated in `build/app/demos/<demo_name>/stm32f429_threadx.bin`.

---

## Deployment & Verification

### 1. Flash the Board
1. Connect the Nucleo board to your computer using a Micro-USB cable via the ST-LINK port.
2. The board will mount as an external USB drive (e.g., named `NOD_F429ZI` or `NUCLEO`).
3. Copy the compiled raw binary output file `build/app/demos/<demo_name>/stm32f767_threadx.bin` and paste it directly onto the board's drive.
4. The ST-LINK status LED will blink rapidly while writing, then the board will auto-reboot.

---
### 2. How to Run & Verify the `threadx_basic` Demo
1. Open your serial terminal program at **115,200 baud, 8N1**.
2. Flash the `threadx_basic` binary to the board.
3. **Verify Heartbeat**: The Green LED (`LD1`) will blink continuously at `2 Hz` (250ms ON, 250ms OFF).
4. **Verify Button Scanner**: Press and hold the blue User Button on the board. The Blue LED (`LD2`) will light up immediately. In your serial terminal, a log message will print showing the system tick timestamp of the button transition.
5. **Verify UART Echo**: Type a word or string into your serial terminal and press `Enter`. The board will print the string back to you and flash the Red LED (`LD3`) for 50ms.

---

### Additional debugging options using Segger Ozone
This platform also supplies a configuration to use with Segger Ozone
1. Open Segger Ozone.
2. Select "Open Existing Project".
3. Navigate to the tools folder of this project, and elect the project you want to debug in the file dialog.
4. Connect with Segger Ozone and it will download the SW to the target.

## Developer Guide: How to Add a New Demo

The decoupled architecture of the BSP (`board_bsp`) makes adding a new application or demo extremely simple:

### Step 1: Create the Demo Directory
Create a new folder under `app/demos/` (e.g., `app/demos/my_demo/`).

### Step 2: Write your Application Code
Create your `main.c`. Keep it clean by using the BSP initialization calls:
```c
#include "board_init.h"
#include "tx_api.h"

int main(void)
{
    /* Initialize MPU, Clocks, GPIOs, and Serial Console */
    board_init();

    /* Optional: Initialize Ethernet hardware if using networking */
    // board_ethernet_init();

    /* Enter ThreadX Kernel */
    tx_kernel_enter();
    return 0;
}
```

### Step 3: Create `CMakeLists.txt`
Create a `CMakeLists.txt` in your demo folder:
```cmake
add_executable(${PROJECT_NAME}
    main.c
)

# Set compile definitions
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        STM32F767xx
        USE_HAL_DRIVER
        STM32F7
)

# Include paths
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

# Link with BSP, ThreadX, and optionally NetX Duo
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        board_bsp
        threadx
        # filex              # Uncomment if using filesystem
        # netxduo            # Uncomment if using network
        # netx_stm32_driver  # Uncomment if using network
        stm32cubef7
)

# Apply linker script and print size
set_target_linker(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/../../startup/STM32F767ZITx_FLASH.ld")
post_build(${PROJECT_NAME})
```

### Step 4: Run your Demo
Configure CMake and build:
```bash
cmake -DACTIVE_DEMO=my_demo -B build
cmake --build build
```
