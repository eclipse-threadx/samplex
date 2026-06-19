# STM32F767ZI-Nucleo Board Enablement Demos

This directory contains the Board Support Package (BSP) static library (`board_bsp`) and build configurations for running the **Eclipse ThreadX RTOS** and **NetX Duo TCP/IP stack** on the **STMicroelectronics NUCLEO-F767ZI** evaluation board (ARM Cortex-M7).

The project features a decoupled static BSP library that hides all low-level hardware initializations (clocks, GPIO, MPU, DMA descriptors, and interrupts) from the application code.

---

## Supported Demos

Use the `ACTIVE_DEMO` CMake variable to select which demo to compile:

| Demo Name | Description | Targets |
| :--- | :--- | :--- |
| **`netx_echo`** *(Default)* | Dynamic network state machine monitoring cable hot-plugging, DHCP IP configuration, and a TCP Echo Server listening on port 7. | ThreadX, NetX Duo |
| **`threadx_basic`** | Multi-threaded RTOS demo showcasing thread scheduling, mechanical button debouncing, message queue logging, and interrupt-driven UART ring buffers. | ThreadX |

---

## Hardware Configuration

* **Development Board**: NUCLEO-F767ZI (Nucleo-144)
* **Microcontroller**: STM32F767ZIT6 (ARM Cortex-M7 running at 216 MHz)
* **Memory Layout**: 2 MB Flash, 512 KB SRAM
  * *Note: The MPU maps `128 KB` of SRAM2 (starting at `0x20060000`) as non-cacheable/non-bufferable for Ethernet DMA descriptors and the packet pool.*
* **Virtual COM Port**: USART3 (PD8/PD9) connected to ST-LINK debugger (115,200 baud, 8N1)
* **User Button**: PC13 (Blue button, Active High)
* **Board LEDs**:
  * `LD1` (Green) - PB0
  * `LD2` (Blue) - PB7
  * `LD3` (Red) - PB14

---

## Prerequisites

Before building, ensure you have the following cross-compilation tools installed on your system PATH:

* **ARM GNU Toolchain** (`arm-none-eabi-gcc`)
* **CMake** (version 3.5 or higher)
* **Ninja** or **Make**
* **Git** (for downloading SDK dependencies)

---

## Quick Start Guide

### 1. Download SDK Dependencies
Run the driver fetcher script to clone official, stock STMicroelectronics HAL drivers and CMSIS files locally:

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

#### Option A: Build `netx_echo` (Default)
```bash
# Configure CMake
cmake -DACTIVE_DEMO=netx_echo -B build

# Compile
cmake --build build
```

#### Option B: Build `threadx_basic`
```bash
# Configure CMake
cmake -DACTIVE_DEMO=threadx_basic -B build

# Compile
cmake --build build
```

The raw binary files will be generated in `build/app/demos/<demo_name>/stm32f767_threadx.bin`.

---

## Deployment & Verification

### 1. Flash the Board
1. Connect the Nucleo board to your computer using a Micro-USB cable via the ST-LINK port.
2. The board will mount as an external USB drive (e.g., named `NOD_F767ZI` or `NUCLEO`).
3. Copy the compiled raw binary output file `build/app/demos/<demo_name>/stm32f767_threadx.bin` and paste it directly onto the board's drive.
4. The ST-LINK status LED will blink rapidly while writing, then the board will auto-reboot.

---

### 2. How to Run & Verify the `netx_echo` Demo
1. **Set Up Serial Monitor**: Open your serial terminal program (e.g., VS Code Serial Monitor, PuTTY, or Tera Term) to the virtual ST-LINK COM Port at **115,200 baud, 8N1**.
2. **Boot Unplugged**: Reset the board with the Ethernet cable disconnected. You should see the following log confirming the BSP initialization completed successfully and the network monitor is polling:
   ```text
   ==========================================
   NetX Echo Demo Booted!
   ==========================================
   [HAL] Calling HAL_ETH_Init...
   [HAL] HAL_ETH_Init returned status: 0
   [NetX] Initializing PHY transceiver...
   [NetX] Waiting for Ethernet link (max 3s)...
   [NetX] Ethernet link timeout! Cable connected?
   [NetX] Packet pool created
   [NetX] IP instance created
   [NetX] Enabling ARP...
   [NetX] Enabling TCP...
   [NetX] Enabling UDP...
   [NetX] Enabling ICMP...
   [NetX] Creating DHCP Thread...
   [NetX] tx_application_define completed
   [NetX] Network Monitor Thread Started.
   ```
3. **Connect Cable**: Plug in your network Ethernet cable. The console will report the connection, initiate DHCP, and spawn the TCP Echo Server:
   ```text
   [NetX] Ethernet cable connected! Enabling link...
   [NetX] Ethernet link is UP! Starting DHCP client...
   [NetX] DHCP Success!
   [NetX] IP Address : <IP_ADDRESS>
   [NetX] Subnet Mask: 255.255.255.0
   [Echo] TCP Echo Server listening on port 7
   ```
4. **Run the Simple Echo Test**: Open a terminal on your computer and run one of the included scripts:
   * **On Windows (PowerShell)**:
     ```powershell
     powershell -ExecutionPolicy Bypass -File "app/demos/netx_echo/test_echo_simple.ps1" <BOARD_IP_ADDRESS>
     ```
   * **On Linux (Bash)**:
     ```bash
     chmod +x app/demos/netx_echo/test_echo_simple.sh
     ./app/demos/netx_echo/test_echo_simple.sh <BOARD_IP_ADDRESS>
     ```
   * **Expected Output**:
     ```text
     Connecting to <BOARD_IP_ADDRESS> on port 7...
     Sending:  'Hello ThreadX NetX Echo!'
     Received: 'Hello ThreadX NetX Echo!'
     ```
5. **Test Mid-Run Hot-Unplugging**: Disconnect the Ethernet cable. You will see `[NetX] Ethernet cable disconnected! Disabling link...` on the serial console, and the interface IP resets to `0.0.0.0`. Plug the cable back in, and it will re-acquire its configuration dynamically.

---

### 3. How to Run & Verify the `threadx_basic` Demo
1. Open your serial terminal program at **115,200 baud, 8N1**.
2. Flash the `threadx_basic` binary to the board.
3. **Verify Heartbeat**: The Green LED (`LD1`) will blink continuously at `2 Hz` (250ms ON, 250ms OFF).
4. **Verify Button Scanner**: Press and hold the blue User Button on the board. The Blue LED (`LD2`) will light up immediately. In your serial terminal, a log message will print showing the system tick timestamp of the button transition.
5. **Verify UART Echo**: Type a word or string into your serial terminal and press `Enter`. The board will print the string back to you and flash the Red LED (`LD3`) for 50ms.

---

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
)

# Link with BSP, ThreadX, and optionally NetX Duo
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        board_bsp
        threadx
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
