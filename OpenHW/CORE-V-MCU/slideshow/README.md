# CORE-V MCU ThreadX Slideshow Demo

A five-screen interactive terminal slideshow running on the
[OpenHW CORE-V MCU](https://github.com/openhwgroup/core-v-mcu) under
[Eclipse ThreadX](https://github.com/eclipse-threadx/threadx).
The demo exercises the I²C temperature sensor, RISC-V CSR introspection,
ThreadX kernel statistics, ANSI terminal control, and polled UART input — all
from a single ThreadX thread running on bare metal.

---

## Table of Contents

1. [Hardware Requirements](#hardware-requirements)
2. [Software Prerequisites](#software-prerequisites)
3. [Terminal Setup](#terminal-setup)
4. [Building](#building)
5. [Deploying](#deploying)
6. [Demo Screens](#demo-screens)
7. [Navigation](#navigation)
8. [Configuration](#configuration)
9. [Source Layout](#source-layout)
10. [License](#license)

---

## Hardware Requirements

| Component | Details |
|-----------|---------|
| FPGA board | Digilent Nexys A7-100T with CORE-V MCU bitstream loaded |
| Debug probe | Ashling Opella-LD (JTAG, USB) |
| Temperature sensor | ADT7420 on I²C bus 1, address `0x4B` (on-board) |
| Serial cable | USB-to-UART adapter connected to the Nexys A7 UART header |

The CORE-V MCU runs at 10 MHz (FC core) / 5 MHz (peripherals).
LED[0] is connected to IO pad 11, GPIO pin 4 (IO_MUX=2) and blinks at startup.

---

## Software Prerequisites

| Tool | Minimum version | Notes |
|------|----------------|-------|
| `riscv64-unknown-elf-gcc` | 13.x | Bare-metal RISC-V toolchain |
| `riscv64-unknown-elf-gdb` | 13.x | Or `gdb-multiarch` as fallback |
| `riscv64-unknown-elf-size` | any | Bundled with the toolchain |
| CMake | 3.20 | |
| Ninja | any | |
| OpenOCD | 0.11+ | Must support FTDI-based probes |

All tools must be on `PATH`.  Run `setup_opella.sh` (in the BSP directory)
once to install the required udev rules for the Opella-LD probe.

---

## Terminal Setup

The demo uses **ANSI VT100** escape sequences and **Latin-1** (Windows-1252)
character encoding.  Any VT100-compatible terminal emulator works; the
settings below are for Tera Term:

| Setting | Value |
|---------|-------|
| Baud rate | 115 200 |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Flow control | None |
| Terminal type | VT100 |
| Character encoding | UTF-8 **disabled**; use default (Latin-1) |
| Window size | 80 × 24 or larger (recommended: 120 × 40) |

> **Important — degree symbol:** The firmware sends `0xB0` (Latin-1 degree
> sign).  If your terminal is set to UTF-8 it will display as `ï¿½`.
> Disable UTF-8 mode or use a Latin-1 / Windows-1252 encoding.

The slideshow automatically probes the terminal size at each screen change
using the ANSI CPR trick (`ESC[999;999H` + `ESC[6n`) and centres all content
accordingly.  If the probe times out it falls back to 80 × 24.

---

## Building

```bash
cd OpenHW/CORE-V-MCU/slideshow
bash scripts/build.sh
```

`scripts/build.sh` runs a clean CMake + Ninja build and prints section sizes on
success.  The output artefacts are placed in `build/`:

| File | Description |
|------|-------------|
| `build/core_v_mcu_slideshow.elf` | ELF with debug info |
| `build/core_v_mcu_slideshow.bin` | Raw binary for direct SRAM load |
| `build/core_v_mcu_slideshow.map` | Linker map |

---

## Deploying

Connect the Nexys A7 USB-JTAG and USB-UART cables, then:

```bash
# Build and flash in one step
bash scripts/deploy.sh --build

# Flash a pre-built ELF (default)
bash scripts/deploy.sh

# Flash and halt at main() for interactive GDB debugging
bash scripts/deploy.sh --debug

# Override the ELF path
bash scripts/deploy.sh --elf /path/to/custom.elf

# Override the OpenOCD config
bash scripts/deploy.sh --openocd-cfg /path/to/board.cfg
```

The script:
1. Starts OpenOCD in the background and waits up to 10 s for it to be ready.
2. Prints the ELF section sizes.
3. Launches GDB in batch mode: loads the ELF into SRAM via `load`, then
   issues `monitor resume` so the CPU starts from the ELF entry point without
   any hardware reset (which would wipe SRAM).
4. Disconnects GDB and stops OpenOCD.

Open the serial port in a separate terminal to see the output:

```bash
minicom -b 115200 -D /dev/ttyUSBx   # replace x with your device number
```

---

## Demo Screens

The slideshow cycles through five screens automatically every **10 seconds**.
A progress bar at the bottom of the terminal shows time remaining on the
current slide.

### Screen 1 — Eclipse ThreadX

```
  ________                        ___  __
 /_  __/ /_  ________  ____ _____/ / |/ /
  / / / __ \/ ___/ _ \/ __ `/ __  /|   /
 / / / / / / /  /  __/ /_/ / /_/ //   |
/_/ /_/ /_/_/   \___/\__,_/\__,_//_/|_|

   Eclipse ThreadX for OpenHW CORE-V MCU v6.5.0.202601
          Copyright (c) 2026 Eclipse ThreadX Contributors

                   Uptime: 0d 00:01:23
```

The version string is assembled at compile time from the ThreadX version
constants (`THREADX_MAJOR_VERSION`, `THREADX_MINOR_VERSION`,
`THREADX_PATCH_VERSION`, `THREADX_BUILD_VERSION`) — never hardcoded.

---

### Screen 2 — OpenHW Group (green)

```
   ___                   _  ____      __
  / _ \ _ __  ___ _ _   | ||  \ \    / /
 | (_) | '_ \/ -_) ' \  | __ |\ \/\/ /
  \___/| .__/\___|_||_| |_||_| \_/\_/
       |_|

  Vendor:  OpenHW Group
  CPU:     CV32E40P (CORE-V)
  ISA:     RV32IMACX
  XLEN:    32
  Privs:   M-mode
```

The ISA string and register width are read live from the `misa` CSR and
decoded at runtime.

---

### Screen 3 — Temperature

Displays the current ambient temperature read from the on-board ADT7420
sensor using big 5×5 ASCII-art digits, with a live Min / Max / Trend line:

```
              ████  ████  ●  ████
              ...

              °C   Current room temperature
      Min: 21.2°C   Max: 22.0°C   Trend: ^ Rising
```

**Calibration:** The ADT7420 reads approximately 3.7 °C high due to FPGA
self-heating on the Nexys A7.  A compile-time offset `TEMP_CALIBRATION_X10`
(set to `−37`, i.e. −3.7 °C) is applied to every reading.

**Warm-up:** The first I²C read after power-on returns 0 while the ADT7420
completes its initial conversion (~240 ms).  The screen shows
*"Sensor warming up…"* and skips that reading rather than displaying −3.7 °C.

---

### Screen 4 — ThreadX Statistics

Live kernel statistics refreshed every 10 seconds:

```
         ── ThreadX Runtime Statistics ──
  Version       : v6.5.0.202601
  Uptime        : 0d 00:02:10
  Tick rate     : 100 Hz
  System ticks  : 13000
  Context sw    : 87
  ── Threads ──
  slideshow     SUSPENDED  pri=20  preempt=20  stack=2048/1792
  tx_timer_th   SUSPENDED  pri=0   preempt=0   stack=1024/...
```

---

### Screen 5 — RISC-V Register Dump

Key CSR values read live with inline assembly:

```
      ── RISC-V CSR Dump ──
  misa      : 0x40001105   RV32IMAC
  mstatus   : 0x00001800
  mtvec     : 0x1A001000
  mcycle    : 0x0028FA30
  minstret  : 0x00197B44
  mvendorid : 0x00000602   OpenHW Group
  marchid   : 0x00000004   CV32E40P
  mimpid    : 0x00000000
  mhartid   : 0x00000000
```

---

## Navigation

| Input | Action |
|-------|--------|
| Any arrow key → / any other key | Advance to next screen, reset timer |
| Arrow key ← | Go back to previous screen, reset timer |
| Automatic | Advance after 10 s (configurable via `SLIDE_PERIOD_TICKS`) |

Arrow keys are detected by polling the UART for VT100 escape sequences
(`ESC [ A/B/C/D`).  The input poll is non-blocking; the slideshow thread
sleeps for 1 s between checks so the temperature reading stays live on
screen 3.

---

## Configuration

All user-tunable constants live in `SDKConfig.h` and `app/slideshow.h`:

| Constant | File | Default | Description |
|----------|------|---------|-------------|
| `TEMP_CALIBRATION_X10` | `SDKConfig.h` | `−37` | Sensor calibration offset (tenths of °C). Set to `0` to disable. |
| `UART_ID_CONSOLE` | `SDKConfig.h` | `0` | UART peripheral index used for the terminal. |
| `SLIDE_PERIOD_TICKS` | `app/slideshow.h` | `1000` | Ticks per slide (100 Hz tick rate → 10 s). |
| `SLIDE_COUNT` | `app/slideshow.h` | `5` | Number of slides. |

---

## Source Layout

```
slideshow/
├── CMakeLists.txt          CMake build definition
├── scripts/
│   ├── build.sh            Clean-build helper
│   └── deploy.sh           Flash-and-run / debug helper
├── SDKConfig.h             Board/sensor configuration
├── tx_user.h               ThreadX user configuration overrides
├── FreeRTOSConfig.h        Stub (required by compatibility layer)
├── libc_shim.c             Minimal libc stubs (_sbrk, _write, …)
├── compat_include/         Header compatibility shims
└── app/
    ├── main.c              ThreadX entry, thread creation
    ├── slideshow.c         Slideshow engine (thread entry, timer, input loop)
    ├── slideshow.h         Slide count, period, type definitions
    ├── screen_threadx.c    Screen 1 — ThreadX logo + banner + uptime
    ├── screen_openhw.c     Screen 2 — OpenHW logo (green) + ISA info
    ├── screen_temp.c       Screen 3 — ADT7420 temperature (big digits)
    ├── screen_stats.c      Screen 4 — ThreadX kernel statistics
    ├── screen_regs.c       Screen 5 — RISC-V CSR dump
    ├── big_digits.c/.h     5×5 ASCII-art digit renderer
    ├── progress_bar.c/.h   Bottom progress bar
    ├── ansi.c/.h           ANSI escape helpers + terminal size probe
    ├── input.c/.h          Non-blocking UART arrow-key detection
    └── uart_io.c/.h        Low-level UART put/print helpers
```

The BSP (board support package) is provided by the
[threadx-fd](https://github.com/fdesbiens/threadx-fd) submodule at
`OpenHW/libs/threadx` and covers clock init, UART driver, I²C master,
GPIO, timer interrupt, and the ThreadX low-level initialisation stub.

---

## License

Source files authored for this project are released under the
[MIT License](https://opensource.org/licenses/MIT).

Files that were substantially AI-generated (noted with an *AI Disclosure*
header) carry a dual `MIT AND CC0-1.0` identifier; the AI-generated portions
may be treated as public domain (CC0-1.0) by downstream users.

ThreadX itself is licensed under the MIT License by Microsoft / Eclipse
Foundation.  See
[`OpenHW/libs/threadx/LICENSE.txt`](../../libs/threadx/LICENSE.txt).
