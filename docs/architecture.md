# Eclipse ThreadX BSP Framework Architecture

This document describes the architecture, design philosophy, directory structure, and onboarding process for the reusable Board Support Package (BSP) framework.

---

## 1. Design Philosophy

The BSP framework is designed to be **additive and non-invasive**, allowing new boards to be integrated without modifying existing board implementations.

1. **Legacy Isolation**: The board directories that predate this framework (`/MXChip`, `/OpenHW`, `/STMicroelectronics`) remain completely untouched, preserving their drivers, submodules, and build systems.
2. **Hardware Access Through the BSP**: Application logic reaches LEDs and the console through the abstract interfaces in `/bsp`, not through vendor registers. Applications are target-resident: each target owns its demo under `app/`, and today's demos do additionally include their own `board_config.h` for memory sizing and vendor headers for board-specific startup self-tests. A fully portable shared application layer is a goal of the framework, not a property it has yet.
3. **Independent Build Configuration**: Each target carries its own `cmake/` toolchain files and build helpers. Nothing in the build is shared between targets, so changing one board cannot break another.

---

## 2. Directory Structure

```text
samplex/ (repository root)
├── libs/                           # Shared RTOS components (ThreadX, NetX Duo, FileX, USBX)
├── scripts/                        # Repository-wide helper scripts
├── MXChip/                         # [Pre-framework] Standalone board sample
├── OpenHW/                         # [Pre-framework] Standalone board sample
├── STMicroelectronics/             # [Pre-framework] Standalone board samples
├── targets/                        # [Framework] Supported BSP target boards
│   ├── Microchip/
│   │   └── POLARFIRE_ICICLE_RENODE/ # Board-specific BSP implementation & Renode target
│   └── STMicroelectronics/
│       └── NUCLEO_F401RE/           # Board-specific BSP implementation & Renode target
├── bsp/                            # [Framework] Abstract BSP interface definitions
│   └── include/bsp/                # board.h, led.h, console.h
├── docs/                           # [Framework] Architecture and onboarding documentation
└── templates/                      # [Framework] Templates for onboarding new boards
```

---

## 3. BSP Interface Contract

Every board added to the framework under `/targets` must implement the abstract APIs defined in `/bsp/include/bsp/`.

### Core Board Control (`board.h`)

* `void bsp_board_init(void)`: Initializes the board, including system clocks, GPIO, and required peripherals.

### LED Control (`led.h`)

* `void bsp_led_init(void)`: Configures the board's user LED.
* `void bsp_led_on(void)`: Turns the LED on.
* `void bsp_led_off(void)`: Turns the LED off.
* `void bsp_led_toggle(void)`: Toggles the LED state.

### Serial Console (`console.h`)

* `void bsp_console_init(void)`: Initializes the default UART console.
* `void bsp_console_write(const char *data, size_t length)`: Transmits a block of data over the console interface.

---

## 4. How to Onboard a New Board

1. **Create the Target Folder**: Create a new directory under `targets/<Vendor>/<Board_Name>/` using `/templates/target/` as the starting point.
2. **Define Local Configuration**: Create a `board_config.h` file containing board-specific settings such as clock configuration, UART parameters, and ThreadX memory allocation.
3. **Implement the BSP APIs**: Implement the interfaces defined in `/bsp/include/bsp/` using the vendor SDK or direct register access.
4. **Configure CMake**: Add the board target to `CMakeLists.txt`, supply the target's toolchain file under its own `cmake/`, build the BSP as a static library, and link it with the application in the target's `app/` directory.

Start from `templates/target/app/main.c`, which depends only on `<tx_api.h>` and the `<bsp/...>` contracts and therefore builds on any target that implements them. Grow it in place as the board needs; there is no shared `/apps` directory to link an application from.
