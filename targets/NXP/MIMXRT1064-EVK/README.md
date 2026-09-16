# NXP i.MX RT1064-EVK — Eclipse ThreadX & NetX Duo

Welcome to the board enablement package for running **Eclipse ThreadX RTOS** and **NetX Duo** on the high-performance **NXP i.MX RT1064-EVK** (ARM Cortex-M7 @ 600 MHz).

This target provides ready demos ranging from fundamental task scheduling and GPIO blinking to full multi-node TCP/IP networking and on-chip hardware cryptographic entropy.
The demos are testable on physical hardware or immediately on your workstation using **Antmicro Renode** system simulation.

---

## Quick Start

You don't need a physical board to get started! You can fetch dependencies, build all targets, and run the automated test suite in three simple steps:

### 1. Prerequisites
Ensure you have the following installed on your machine:
* **ARM GNU Toolchain** (`arm-none-eabi-gcc` 10.3+)
* **CMake** (3.20+) and **Ninja** (recommended) or Make
* **Python 3** (3.8+, for automated Renode test runners)
* **Antmicro Renode** (1.15.3+, for simulation)
* **Git** (for repository submodules)

### 2. Fetch Dependencies & Build
Download the official NXP MCUXpresso SDK drivers and build all demos:

* **Windows (PowerShell)**:
  ```powershell
  # 1. Fetch NXP SDK peripheral drivers and CMSIS headers
  powershell -ExecutionPolicy Bypass -File .\scripts\fetch_sdk.ps1

  # 2. Build all demos
  powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1
  ```

* **Linux / macOS (Bash)**:
  ```bash
  # 1. Fetch NXP SDK peripheral drivers and CMSIS headers
  chmod +x ./scripts/*.sh
  ./scripts/fetch_sdk.sh

  # 2. Build all demos
  ./scripts/build.sh
  ```

> [!NOTE]
> The NetX Duo Ethernet driver (`lib/netx_driver`) and KSZ8081 PHY driver (`lib/phyksz8081`) are pre-vendored in this repository. `fetch_sdk` only downloads the official NXP core MCU peripheral drivers and CMSIS headers, verifying every archive against pinned SHA-256 checksums.

### 3. Run Automated Tests
Verify that all demos build and pass under Renode simulation:
```bash
python ./scripts/test_renode.py --demo threadx_basic
python ./scripts/test_renode.py --demo netx_echo
python ./scripts/test_renode.py --demo netx_trng_console --seed 12345
```

---

## Supported Applications

The build system can compile all demos together (default) or individual demos on demand. Output artifacts are placed in `build/app/demos/<demo_name>/`:

| Application | Architecture | What It Demonstrates | Generated Binaries |
| :--- | :--- | :--- | :--- |
| **`threadx_basic`** | Single-Node | ThreadX kernel fundamentals: preemptive priority scheduling, software timer callbacks, and user LED (`D18`) heartbeat blinking. | `mimxrt1064_threadx.elf`<br>`mimxrt1064_threadx.bin` |
| **`netx_echo`** | Multi-Node | Full NetX Duo network stack: ARP resolution, ICMP ping replies, UDP datagram echo (port 7), and TCP stream echo (port 7). | `mimxrt1064_threadx.elf` *(Server: 192.168.0.100)*<br>`mimxrt1064_client.elf` *(Client: 192.168.0.101)* |
| **`netx_trng_console`** | Multi-Node | On-chip hardware True Random Number Generator (`0x400CC000`) integrated with an interactive TCP remote diagnostics management shell (port 23). | `mimxrt1064_threadx.elf` *(Server: 192.168.0.100)*<br>`mimxrt1064_client.elf` *(Client: 192.168.0.101)* |

> [!NOTE]
> **How Multi-Node Verification Works**:
> In `netx_echo` and `netx_trng_console`, Renode boots **two independent virtual i.MX RT1064 machines** interconnected via a simulated Ethernet switch. The **server** node runs ThreadX services, while the **client** node runs an automated test suite that transmits network traffic.

---

## Renode Simulation Guide

Renode provides accurate instruction-level simulation of the ARM Cortex-M7 core and key peripherals, enabling end-to-end verification without hardware.

### 1. Interactive Simulation (GUI)
Launch Renode with virtual serial terminal windows attached to the microcontroller's UART console:

* **Windows (PowerShell)**:
  ```powershell
  # ThreadX Core Basic (single node)
  powershell -ExecutionPolicy Bypass -File .\scripts\simulate.ps1 -Demo threadx_basic

  # NetX Duo Network Echo (multi-node server + client)
  powershell -ExecutionPolicy Bypass -File .\scripts\simulate.ps1 -Demo netx_echo

  # Hardware TRNG Diagnostic Console (multi-node server + client)
  powershell -ExecutionPolicy Bypass -File .\scripts\simulate.ps1 -Demo netx_trng_console
  # You can specify a pseudo-random seed for reproducible test runs
  powershell -ExecutionPolicy Bypass -File .\scripts\simulate.ps1 -Demo netx_trng_console -Seed 12345
  ```

* **Linux / macOS (Bash)**:
  ```bash
  # ThreadX Core Basic (single node)
  ./scripts/simulate.sh -d threadx_basic

  # NetX Duo Network Echo (multi-node server + client)
  ./scripts/simulate.sh -d netx_echo

  # Hardware TRNG Diagnostic Console (multi-node server + client)
  ./scripts/simulate.sh -d netx_trng_console
  # You can specify a pseudo-random seed for reproducible test runs
  ./scripts/simulate.sh -d netx_trng_console -s 12345
  ```

### 2. Headless Automated Regression Testing (CI/CD)
Headless testing is designed for automated continuous integration pipelines. The runner boots the simulation, monitors the virtual UART logs, and exits with code `0` on success or code `1` on failure/timeout.

#### Direct Python Runner (Matches CI):
```bash
# Verify ThreadX basic scheduling & timers
python ./scripts/test_renode.py --demo threadx_basic

# Verify NetX Duo ICMP ping, UDP echo, and TCP echo
python ./scripts/test_renode.py --demo netx_echo

# Verify Hardware TRNG entropy generation and remote console
python ./scripts/test_renode.py --demo netx_trng_console --seed 12345
```

#### Convenience Shell Wrappers:
* **Windows (PowerShell)**:
  ```powershell
  powershell -ExecutionPolicy Bypass -File .\scripts\test_headless.ps1 -Demo threadx_basic
  powershell -ExecutionPolicy Bypass -File .\scripts\test_headless.ps1 -Demo netx_echo
  powershell -ExecutionPolicy Bypass -File .\scripts\test_headless.ps1 -Demo netx_trng_console -Seed 12345
  ```
* **Linux / macOS (Bash)**:
  ```bash
  ./scripts/test_headless.sh --demo threadx_basic
  ./scripts/test_headless.sh --demo netx_echo
  ./scripts/test_headless.sh --demo netx_trng_console --seed 12345
  ```

---

## Simulation Scope & Hardware Status

> [!WARNING]
> **Hardware Status: Verified in simulation, not yet on physical silicon.**
> All automated tests in this repository currently run under **Antmicro Renode** system emulation.

### What Renode Accurately Simulates:
* **Boot & Vector Table**: Boots from simulated FlexSPI NOR Flash into Cortex-M7 privileged mode.
* **ThreadX RTOS Kernel**: Preemptive priority scheduling, thread synchronization (mutexes, semaphores), and software timer ticks.
* **NetX Duo Networking**: Ethernet MAC (`ENET`) DMA transfers, ARP cache handling, ICMP ping replies, UDP socket datagrams, and TCP stream connections.

### What Renode Stubs:
* **Clock Tree & PLLs**: Renode uses stub tags for the Clock Control Module (`CCM`) and `ANALOG` power blocks. Registers return fixed default values (e.g. `CCM_CBCDR` returns `0x000A8200`), so PLL lock loops succeed unconditionally without exercising analog timing.
* **Core Frequency**: The `600 MHz` banner in the console is a compile-time SDK constant (`SystemCoreClock`), not a measured silicon frequency.
* **Pin Multiplexing**: `IOMUXC` and `IOMUXC_GPR` writes are acknowledged without modeling electrical pin drive strengths or pin collisions.
* **TRNG Bring-up and Entropy Semantics**: Renode's `IMX_TRNG` model implements neither the programming sequence nor the block-consume semantics, and logs `Unhandled write` for `MCTL[PRGM]`, `MCTL[RST_DEF]` and `SDCTL[SAMP_SIZE]`/`SDCTL[ENT_DLY]`. It also returns a fresh pseudo-random word on *any* `ENT[n]` read, whereas silicon refills the block only when `ENT15` is read and re-arms `ENT_VAL`. The driver follows the reference manual on both points, but the emulator would accept a driver that does neither, so this is the part of the target most in need of a run on real hardware.

---

## Target Hardware & Flashing Guide

If you are flashing to a physical **NXP MIMXRT1064-EVK** board:

### Hardware Specifications
* **Evaluation Board**: NXP MIMXRT1064-EVK (ARM Cortex-M7 @ up to 600 MHz)
* **Memory**: 4 MB on-chip FlexSPI NOR Flash (`0x70000000`), 1 MB on-chip SRAM (ITCM, DTCM, NonCacheable OCRAM)
* **Serial Console**: LPUART1 via OpenSDA micro-USB (`J41`), 115,200 baud, 8N1
* **User LED & Button**: Green LED `D18` (`GPIO1_IO09`), SW8 WAKEUP button (`GPIO5_IO00`)
* **Ethernet**: ENET MAC + Microchip KSZ8081RNA PHY via RMII
* **TRNG Hardware**: On-chip True Random Number Generator (`0x400CC000`)

### Boot Switch Configuration
Set boot switches **SW7** for **Internal Boot (FlexSPI NOR Flash)**:
* `SW7-1`: OFF
* `SW7-2`: ON
* `SW7-3`: OFF
* `SW7-4`: ON

### Flashing Methods

#### Option 1: OpenSDA Drag-and-Drop (Fastest)
1. Connect micro-USB cable to `J41` on the EVK board.
2. The board mounts as a USB drive named `RT1064-EVK`.
3. Copy `build/app/demos/<demo_name>/mimxrt1064_threadx.bin` and paste it directly onto the drive.
4. The OpenSDA LED blinks rapidly during flashing. Press `SW3` (RESET) to boot.

#### Option 2: SEGGER J-Link
```text
JLink.exe -device MIMXRT1064xxx6A -if SWD -speed 4000 -autoconnect 1
loadfile build/app/demos/<demo_name>/mimxrt1064_threadx.hex
r
g
```

#### Option 3: pyOCD Command Line
```bash
pip install pyocd && pyocd pack install MIMXRT1064
pyocd flash -t mimxrt1064 build/app/demos/<demo_name>/mimxrt1064_threadx.hex
```

#### Option 4: NXP MCUXpresso IDE / GUI Flash Tool
1. In MCUXpresso IDE, select **GUI Flash Tool** from the toolbar.
2. Choose target device `MIMXRT1064xxxxA` and memory `PROGRAM_FLASH` (`0x70000000`).
3. Select `build/app/demos/<demo_name>/mimxrt1064_threadx.elf` and click **Program**.

---

## Software Architecture & Developer Guide

### Modular BSP Architecture
The target features a decoupled, three-tier design:
* **`mimxrt1064_bsp`**: Clean C hardware abstraction layer (`bsp/board.h`, `bsp/led.h`, `bsp/console.h`). Application code interacts solely through BSP APIs rather than raw vendor registers.
* **`board_bsp`**: Startup assembly (`startup_mimxrt1064.S`), low-level ThreadX initialization (`tx_initialize_low_level.S`), newlib standard C library syscalls (`_sbrk` heap protection), and hardware TRNG drivers.
* **`mimxrt1064_common`**: Central CMake `INTERFACE` library propagating required MCU compiler definitions (`CPU_MIMXRT1064DVL6A`, `XIP_EXTERNAL_FLASH=1`, etc.) and SDK include directories to all targets automatically.

### Adding a Custom Demo in 4 Steps

#### Step 1: Create the Demo Directory
Create a folder under `app/demos/` (e.g., `app/demos/my_new_demo/`).

#### Step 2: Write Application Code (`main.c`)
```c
#include "bsp/board.h"
#include "bsp/led.h"
#include "bsp/console.h"
#include "tx_api.h"

int main(void)
{
    /* Initialize MPU, system clocks, pins, LED, and console */
    bsp_board_init();

    /* Enter ThreadX RTOS Kernel */
    tx_kernel_enter();
    return 0;
}
```

#### Step 3: Create `CMakeLists.txt`
Thanks to CMake target inheritance, you only need to link `board_bsp`, `threadx`, and `mcux_sdk` — all compiler definitions and SDK include paths are inherited automatically:
```cmake
set(DEMO_TARGET "demo_my_new_demo")
add_executable(${DEMO_TARGET} main.c)
set_target_properties(${DEMO_TARGET} PROPERTIES OUTPUT_NAME "mimxrt1064_threadx")

# Only private demo includes are needed; SDK headers and board definitions
# are inherited automatically from board_bsp.
target_include_directories(${DEMO_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${DEMO_TARGET} PRIVATE
    board_bsp
    threadx
    mcux_sdk
    # netxduo             # Uncomment if using network stack
    # netx_imxrt_driver   # Uncomment if using network driver
)

set_target_linker(${DEMO_TARGET} "${CMAKE_CURRENT_SOURCE_DIR}/../../startup/MIMXRT1064xxxxx_flexspi_nor.ld")
post_build(${DEMO_TARGET})
```

#### Step 4: Build and Simulate
```bash
cmake -B build -G Ninja -DACTIVE_DEMO=my_new_demo
cmake --build build
```
