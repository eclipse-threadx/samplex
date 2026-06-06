# STM32F767ZI-Nucleo Board Enablement Demo

This directory contains the Board Support Package (BSP) and build environment for running the Eclipse ThreadX RTOS on the **STMicroelectronics NUCLEO-F767ZI** evaluation board (ARM Cortex-M7).

The project is structured with an isolated build framework, keeping all platform dependencies localized to ensure clean integration.

---

## Hardware Configuration

* **Development Board**: NUCLEO-F767ZI (Nucleo-144)
* **Microcontroller**: STM32F767ZIT6 (ARM Cortex-M7 running at 216 MHz)
* **Memory**: 2 MB Flash, 320 KB SRAM
* **Virtual COM Port**: USART3 (PD8/PD9) connected to ST-LINK debugger (115,200 baud, 8N1)
* **Board LEDs**:
  * `LD1` (Green) - PB0
  * `LD2` (Blue) - PB7
  * `LD3` (Red) - PB14
* **User Button**: PC13 (Blue button, Active High)

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

Before building, ensure you have the following cross-compilation tools installed on your PATH:

* **ARM GNU Toolchain** (`arm-none-eabi-gcc`)
* **CMake** (version 3.5 or higher)
* **Ninja** or **Make**
* **Git** (for downloading SDK dependencies)

---

## Quick Start Guide

### 1. Download SDK Dependencies
Run the driver fetcher script to clone the official, lightweight STMicroelectronics HAL drivers and CMSIS files locally:

* **On Windows (PowerShell)**:
  ```powershell
  powershell -ExecutionPolicy Bypass -File .\scripts\fetch_sdk.ps1
  ```
* **On Linux (Bash - Ubuntu 24.04)**:
  ```bash
  chmod +x ./scripts/fetch_sdk.sh
  ./scripts/fetch_sdk.sh
  ```

### 2. Build the Project
Run the compilation script to compile the libraries and application, linking the ThreadX RTOS kernel:

* **On Windows (PowerShell)**:
  ```powershell
  powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Rebuild
  ```
* **On Linux (Bash - Ubuntu 24.04)**:
  ```bash
  chmod +x ./scripts/build.sh
  ./scripts/build.sh --rebuild
  ```

---

## Deployment & Verification

1. **Flash the Board**:
   * Connect the NUCLEO-F767ZI board to your computer using a Micro-USB cable via the ST-LINK port.
   * The board will mount as an external USB drive (e.g., `NUCLEO_F767ZI`).
   * Copy the raw binary output file `build/stm32f767_threadx.bin` and paste it directly onto the board's drive.
   * The ST-LINK status LED will blink rapidly while writing, then the board will auto-reboot.

2. **Monitor Serial Console**:
   * Connect a serial terminal program (VS Code Serial Monitor, PuTTY, or Tera Term) to the virtual ST-LINK COM Port.
   * Configuration: **115,200 baud**, **8 data bits**, **1 stop bit**, **no parity**, **CRLF line endings** (`\r\n`).
   * Press the blue button to check log telemetry, and type strings into the console to test the interrupt-driven echo thread!
