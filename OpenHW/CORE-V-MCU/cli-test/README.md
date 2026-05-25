# CORE-V MCU CLI Test — ThreadX Sample Application

This sample application runs on the
[CORE-V MCU](https://github.com/openhwgroup/core-v-mcu) (CV32E40P, RV32IMC) and
demonstrates a multi-threaded ThreadX application with a UART command-line
interface.  It is a ThreadX port of the
[core-v-mcu-cli-test](https://github.com/openhwgroup/core-v-mcu-cli-test)
application.

The application runs two concurrent threads:

- **Blinky** — toggles GPIO pin 5 at 1 Hz using native ThreadX APIs.
- **CLI** — drives a UART console that accepts typed commands, using the
  [QuickLogic CLI library](https://github.com/openhwgroup/core-v-mcu-cli-test/tree/main/cli_test/libs/cli)
  through the ThreadX FreeRTOS compatibility layer.

---

## Table of Contents

1. [Prerequisites](#1-prerequisites)
2. [Directory structure](#2-directory-structure)
3. [Application architecture](#3-application-architecture)
4. [FreeRTOS compatibility layer](#4-freertos-compatibility-layer)
5. [Building](#5-building)
6. [Flashing and debugging](#6-flashing-and-debugging)
7. [Using the CLI](#7-using-the-cli)
8. [Adding new commands](#8-adding-new-commands)
9. [Unit tests](#9-unit-tests)
10. [Third-party code and licensing](#10-third-party-code-and-licensing)
11. [Setup and deployment scripts](#11-setup-and-deployment-scripts)

---

## 1. Prerequisites

This application depends on the CORE-V MCU ThreadX BSP, which lives in the sibling
`threadx-fd` repository.  The expected layout on disk is:

```
core-v/
├── threadx-fd/          ThreadX fork — source of truth during development
└── samplex-fd/          This repository
    └── OpenHW/
        ├── libs/
        │   └── threadx/ ← git submodule (fdesbiens/threadx-fd, core-v-mcu branch)
        └── CORE-V-MCU/
        └── cli-test/    ← you are here
```

### Required tools

Run `../../scripts/install_deps.sh` to install all dependencies in one step:

```bash
bash ../../scripts/install_deps.sh
```

Or install individually:

| Tool | Minimum version | Notes |
|------|----------------|-------|
| `riscv64-unknown-elf-gcc` | 13.x | `apt install gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf` |
| CMake | 3.20 | `apt install cmake` |
| Ninja | 1.10 | `apt install ninja-build` |
| OpenOCD | 0.12 | `apt install openocd` |
| GDB | any multiarch | `apt install gdb-multiarch` — Ubuntu does not ship `riscv64-unknown-elf-gdb` |
| minicom (or similar) | any | `apt install minicom` — for the UART console |

---

## 2. Directory structure

```
cli-test/
├── app/
│   ├── main.c              ThreadX entry point and thread creation
│   ├── app_commands.c      CLI command handlers
│   └── app_commands.h
├── libs/
│   └── cli/
│       ├── include/
│       │   ├── cli.h                  Top-level CLI API
│       │   └── cli/                   Sub-headers (structs, args, print, …)
│       └── source/
│           ├── cli_platform.c         Platform layer — UART I/O + task creation
│           ├── cli_loop.c             ← verbatim QuickLogic Apache-2.0
│           ├── cli_stdcmds.c          ← verbatim QuickLogic Apache-2.0
│           ├── cli_misc.c             ← verbatim QuickLogic Apache-2.0
│           └── cli_args.c             ← verbatim QuickLogic Apache-2.0
├── compat_include/         Bare-metal stubs for C standard headers
│   ├── setjmp.h            longjmp stub (CLI error handling)
│   ├── stdio.h             Redirects printf/puts to CLI_printf
│   ├── stdlib.h            Minimal stubs
│   └── string.h            Redirects to BSP string shim
├── tests/
│   ├── mock/
│   │   ├── cli_mock.c      Captures CLI_printf output to a buffer
│   │   └── cli_mock.h
│   ├── test_cli_commands.c Host-compiled tests for command dispatch
│   └── CMakeLists.txt
├── CMakeLists.txt
├── FreeRTOSConfig.h        ThreadX FreeRTOS compat-layer configuration
├── SDKConfig.h             Platform constants (UART ID, feature flags)
├── tx_user.h               ThreadX compile-time tunables
├── libc_shim.c             Freestanding C runtime helpers
├── scripts/
│   ├── build.sh            One-shot build script
│   └── deploy.sh           Flash-and-run / debug helper
└── README.md               This file
```

---

## 3. Application architecture

### Boot sequence

```
crt0.S: init GP, SP, clear BSS
  └─ main()
       ├─ system_init()          FLL → 50 MHz, UART 115 200 baud, timer 100 Hz, IRQ
       └─ tx_kernel_enter()
            └─ tx_application_define()
                 ├─ gpio_set_output(LED)   configure LED pin as output
                 ├─ tx_thread_create(blinky_thread, …)
                 ├─ tx_freertos_init()     initialise FreeRTOS compat layer
                 └─ CLI_start_task(menu)   create CLI thread via compat layer
```

### Thread inventory

| Thread | Priority | Stack | Period | Function |
|--------|----------|-------|--------|----------|
| `blinky` | 10 | 1 024 B | 500 ms (toggle) | `blinky_entry` in `main.c` |
| `CLI` | `tskIDLE_PRIORITY + 2` | 14 × 512 = 7 168 B | event-driven | `CLI_task` in `cli_platform.c` |

The blinky thread is created directly with `tx_thread_create` (native ThreadX API).
The CLI thread is created by `CLI_start_task` via `xTaskCreate` from the FreeRTOS
compatibility layer — which internally maps to `tx_thread_create`.

### Data flow

```
UART RX byte
  → uart_read_byte() polled in CLI_getkey_raw()
  → CLI_rx_byte(k)
  → CLI_dispatch_command()
  → command handler (e.g. do_version)
  → CLI_printf() → CLI_putc() → CLI_putc_raw() → uart_write_byte()
  → UART TX
```

---

## 4. FreeRTOS compatibility layer

ThreadX ships a FreeRTOS compatibility layer at:

```
threadx-fd/utility/rtos_compatibility_layers/FreeRTOS/
├── FreeRTOS.h      — main API header (wraps ThreadX)
├── task.h          — xTaskCreate, vTaskDelay, xTaskGetTickCount, …
└── tx_freertos.c   — implementation
```

This allows the unmodified QuickLogic CLI library sources (`cli_loop.c`,
`cli_stdcmds.c`, etc.) to include `FreeRTOS.h` and `task.h` and call
`vTaskDelay` / `xTaskCreate` / `configASSERT` without any modifications.

The mapping is transparent:

| FreeRTOS call | ThreadX equivalent |
|---------------|--------------------|
| `xTaskCreate(f, name, depth, param, pri, handle)` | `tx_thread_create` |
| `vTaskDelay(ticks)` | `tx_thread_sleep` |
| `xTaskGetTickCount()` | `tx_time_get` |
| `configASSERT(x)` | no-op (bare metal) |

**`FreeRTOSConfig.h`** is tuned to match the BSP:

```c
#define configTICK_RATE_HZ      100U   /* matches PULP FC Timer 100 Hz tick */
#define configMINIMAL_STACK_SIZE 512U
#define configMAX_PRIORITIES    32U
#define TX_FREERTOS_AUTO_INIT   0      /* we drive init via tx_kernel_enter() */
```

`tx_freertos_init()` must be called from `tx_application_define()` before any
`xTaskCreate` call.

---

## 5. Building

Before building, initialise the submodule if you have not already done so:

```bash
git submodule update --init OpenHW/libs/threadx
```

### Quick start

```bash
cd samplex-fd/OpenHW/CORE-V-MCU/cli-test
bash scripts/build.sh
```

Outputs:

```
build/core_v_mcu_cli_test.elf   — ELF with debug symbols
build/core_v_mcu_cli_test.bin   — raw binary
build/core_v_mcu_cli_test.map   — linker map
```

### Manual CMake invocation

```bash
THREADX_ROOT="$(realpath ../../libs/threadx)"
cmake -B build -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE="${THREADX_ROOT}/cmake/riscv64-gcc-rv32imc.cmake" \
      -DCMAKE_BUILD_TYPE=Debug \
      .
cmake --build build
```

### Expected section layout

```
Section     Address      Size
.vectors    0x1c000800   128 B   (32 × 4-byte jump instructions)
.text       0x1c000880   ~25 KB  (code + rodata)
.data       after .text  ~8 B
.bss        after .data  ~70 KB  (thread stacks + CLI buffers)
```

---

## 6. Flashing and debugging

These steps are identical to the BSP demo.  Full details are in the
[BSP README](../../libs/threadx/ports/risc-v32/gnu/example_build/core_v_mcu/README.md).

### Quick deploy (recommended)

```bash
# Flash and run:
bash scripts/deploy.sh

# Build first, then flash:
bash scripts/deploy.sh --build

# Flash and stop at main for an interactive debug session:
bash scripts/deploy.sh --debug
```

> **First time only:** run `bash ../../scripts/setup_opella.sh` to install the udev
> rule for the Ashling Opella LD.  See [Section 11](#11-setup-and-deployment-scripts).

### Manual quick reference

1. Connect Ashling Opella LD and USB-UART to the Nexys A7.
2. Start OpenOCD:
   ```bash
   openocd -f ../../libs/threadx/ports/risc-v32/gnu/example_build/core_v_mcu/openocd-nexys-Ashling-Opella-LD.cfg
   ```
3. In a second terminal, start GDB:
   ```bash
   riscv64-unknown-elf-gdb \
       --eval-command="target extended-remote localhost:3333" \
       --eval-command="file build/core_v_mcu_cli_test.elf" \
       --eval-command="load" \
       --eval-command="break main" \
       --eval-command="continue"
   ```
4. Open the serial console:
   ```bash
   minicom -b 115200 -D /dev/ttyUSBx
   ```

On successful boot you will see:

```
#*******************
Command Line Interface
May 14 2026 22:35:00
App SW Version: CORE-V MCU ThreadX CLI Demo v1.0.0
#*******************
>
```

The LED on GPIO pin 5 will blink at 1 Hz.

---

## 7. Using the CLI

### Available commands

| Command | Description |
|---------|-------------|
| `version` | Print firmware version and build timestamp |
| `help` | List available commands |
| `wait <ms>` | Sleep for the given number of milliseconds (built-in) |

Type a command and press **Enter**.  Use **Ctrl+C** (or type `^c`) to abort a
running command and return to the prompt.  The CLI processes one command at a time;
the blinky thread runs concurrently throughout.

### Example session

```
> version
CORE-V MCU ThreadX CLI Demo v1.0.0
Build: May 14 2026 22:35:00
> help
  version          Print firmware version
  help             List commands (this)
  wait             wait <milliseconds>
> wait 2000
(2-second pause)
>
```

---

## 8. Adding new commands

1. Add a handler function in `app/app_commands.c`:

```c
static void do_blink_fast(const struct cli_cmd_entry *pEntry)
{
    (void)pEntry;
    CLI_printf("Blinking fast for 5 seconds\n");
    /* ... your logic ... */
}
```

2. Register it in the menu table:

```c
static const struct cli_cmd_entry app_main_menu[] = {
    CLI_CMD_SIMPLE("version",    do_version,    "Print firmware version"),
    CLI_CMD_SIMPLE("blink-fast", do_blink_fast, "Blink LED rapidly for 5 s"),
    CLI_CMD_TERMINATE()
};
```

3. Rebuild with `bash scripts/build.sh`.

### Command entry macros

| Macro | Use |
|-------|-----|
| `CLI_CMD_SIMPLE(name, fn, help)` | Simple command, no subcommands |
| `CLI_CMD_WITH_ARGS(name, fn, help)` | Command that reads additional arguments |
| `CLI_CMD_SUBMENU(name, table, help)` | Nested submenu |
| `CLI_CMD_TERMINATE()` | Required last entry in every table |

### Reading arguments in a handler

```c
static void do_set_period(const struct cli_cmd_entry *pEntry)
{
    int32_t ms = 0;
    (void)pEntry;
    CLI_int32_getshow("period_ms", &ms);
    CLI_printf("Period set to %d ms\n", (int)ms);
}
```

---

## 9. Unit tests

Host-compiled tests (no RISC-V toolchain needed):

```bash
cd tests
cmake -B build -G Ninja
cmake --build build
./build/cli_tests
```

Expected output:

```
All CLI tests passed
```

### What is tested

| Test | Description |
|------|-------------|
| `test_version_output` | `do_version()` emits a string containing "CORE-V MCU ThreadX" |
| `test_version_no_crash` | `do_version()` does not NULL-deref or crash |

The mock (`tests/mock/cli_mock.c`) intercepts `CLI_printf` and appends output to
a static buffer.  `cli_mock_reset()` clears the buffer before each test;
`cli_mock_output()` returns a pointer to the captured string.

### Adding a test

```c
static void test_my_command(void)
{
    cli_mock_reset();
    do_my_command(NULL);
    TEST_ASSERT(strstr(cli_mock_output(), "expected string") != NULL);
}
```

Register it in `test_cli_commands.c`'s `main()` alongside the existing tests.

---

## 10. Third-party code and licensing

| File(s) | Origin | Licence |
|---------|--------|---------|
| `libs/cli/source/cli_loop.c` | [core-v-mcu-cli-test](https://github.com/openhwgroup/core-v-mcu-cli-test) | Apache-2.0, © 2020 QuickLogic Corporation |
| `libs/cli/source/cli_stdcmds.c` | same | Apache-2.0, © 2020 QuickLogic Corporation |
| `libs/cli/source/cli_misc.c` | same | Apache-2.0, © 2020 QuickLogic Corporation |
| `libs/cli/source/cli_args.c` | same | Apache-2.0, © 2020 QuickLogic Corporation |
| `libs/cli/include/cli.h` and `cli/` sub-headers | same | Apache-2.0, © 2020 QuickLogic Corporation |
| `libs/cli/source/cli_platform.c` | Derived from cli_platform.c | Apache-2.0 AND MIT, © 2020 QuickLogic + © 2026 Eclipse ThreadX contributors |
| All other files | Original | MIT and CC0-1.0, © 2026 Eclipse ThreadX contributors |

The four verbatim-copied QuickLogic files are **not modified**.  Pre-existing
compiler warnings in those files are suppressed per-file in `CMakeLists.txt` using
`set_source_files_properties` to avoid altering the upstream source and triggering
Apache-2.0 §4(b) modification notices.

The FreeRTOS compatibility layer (`threadx-fd/utility/rtos_compatibility_layers/FreeRTOS/`)
is © 2024 Microsoft Corporation / © 2026 Eclipse ThreadX contributors, MIT.

---

## 11. Setup and deployment scripts

### 11.1 `setup_opella.sh` — one-time host setup

Located at `samplex-fd/OpenHW/scripts/setup_opella.sh`.  Run **once per machine**
to grant user-space access to the Ashling Opella LD USB device.

```bash
# Native Linux:
sudo bash ../../scripts/setup_opella.sh

# WSL (also attempts usbipd-win USB forwarding):
bash ../../scripts/setup_opella.sh
```

What it does:

| Step | Native Linux | WSL |
|------|-------------|-----|
| Write `/etc/udev/rules.d/99-ashling-opella.rules` | ✓ | ✓ |
| Add user to `plugdev` group | ✓ | ✓ |
| Reload udev | ✓ | ✓ |
| `usbipd.exe attach --wsl` (v3: `wsl attach`) | — | automatic |

> **WSL — one-time bind (Windows admin required):**
> Before the first attach, run this once in an **elevated (Run as Administrator) PowerShell** on the Windows host:
> ```powershell
> usbipd bind --hardware-id 0b6b:0040
> ```
> After binding, `setup_opella.sh` handles subsequent attaches automatically.

> **WSL — if attach fails with "not USBIP capable" (usbipd-win v3 + Ubuntu 22.04+):**
> Ubuntu 22.04 and 24.04 enable systemd by default.  usbipd-win **v3** has a
> known bug where it incorrectly reports the kernel as not USBIP capable when
> systemd is running.  **Upgrade usbipd-win to v4 or later:**
> <https://github.com/dorssel/usbipd-win/releases>
>
> If you are on an older WSL2 kernel without USBIP support, first run:
> ```powershell
> wsl --update
> wsl --shutdown
> ```

After running, **re-plug the probe**.  On WSL, re-run the script after each WSL restart.

### 11.2 `deploy.sh` — flash and run

```bash
bash scripts/deploy.sh [--build] [--debug] [--elf <path>] [--openocd-cfg <path>]
```

| Flag | Effect |
|------|--------|
| `--build` | Run `build.sh` before flashing |
| `--debug` | Load the ELF and stop at `main`; leave GDB attached interactively |
| `--elf <path>` | Override the ELF file (default: `build/core_v_mcu_cli_test.elf`) |
| `--openocd-cfg <p>` | Override the OpenOCD config file |

The script starts OpenOCD in the background, waits up to 10 s for it to become
ready, then drives GDB to flash the target.  OpenOCD is always stopped on exit
(including Ctrl-C).

