# Eclipse ThreadX BSP Framework Architecture

This document describes the architecture, design philosophy, directory structure, and onboarding process for the reusable Board Support Package (BSP) framework.

---

## 1. Design Philosophy

The BSP framework is designed to be **additive and non-invasive**, allowing new boards to be integrated without modifying existing board implementations.

1. **Legacy Isolation**: The board directories that predate this framework (`/MXChip`, `/OpenHW`, `/STMicroelectronics`) remain completely untouched, preserving their drivers, submodules, and build systems.
2. **Hardware Access Through the BSP**: Application logic reaches LEDs, the console, the board's RAM budget and its startup self-tests through the abstract interfaces in `/bsp`, not through vendor registers. Both shipped demos now include only `<tx_api.h>` and `<bsp/...>` headers. Applications are still target-resident, though: each target owns its demo under `app/`, and there is no shared application directory to link one from. A fully portable shared application layer is a goal of the framework, not a property it has yet.
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
│   └── include/bsp/                # board.h, led.h, console.h, memory.h, selftest.h
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

### Application RAM Budget (`memory.h`)

* `void bsp_ram_region(void *first_unused, void **base, size_t *size)`: Reports the RAM region the application may claim, given the pointer ThreadX passed to `tx_application_define()`.

ThreadX reports the first address it believes to be unused, but only the board knows what sits above it - a C heap reservation, a main stack at the top of RAM, or a peripheral window. This interface is what lets an application size a `TX_BYTE_POOL` without naming a board symbol. A board must never include its own reservations in the region it returns; an application will allocate every byte of it.

The two shipped targets show the two shapes this takes. The NUCLEO-F401RE keeps its main stack at the top of SRAM and clamps the region below a fixed reservation; the PolarFire SoC Icicle Kit keeps its boot stack *below* ThreadX's first unused address and only has to skip its C heap reservation.

### Startup Self-Tests (`selftest.h`)

* `unsigned bsp_self_test(bsp_selftest_report_fn report, void *context)`: Runs the board's startup self-tests, reporting each through the callback, and returns the number of failures.

Checks that the board came up as its own configuration promised are BSP tests, not application tests: they need vendor headers, linker symbols and register maps that no portable application can see. Keeping them behind this interface is what removed those headers from both demos' `main.c`.

The application supplies only the reporting callback, so message formatting - and therefore the choice between `printf()` and `bsp_console_write()` - stays on the application side. Both shipped targets verify that their C heap cannot grow into the region `bsp_ram_region()` promises the application; the NUCLEO-F401RE additionally checks its clock tree and HAL timebase, and the PolarFire its CLINT tick arithmetic and PLIC routing.

---

## 4. How to Onboard a New Board

1. **Create the Target Folder**: Create a new directory under `targets/<Vendor>/<Board_Name>/` using `/templates/target/` as the starting point.
2. **Define Local Configuration**: Create a `board_config.h` file containing board-specific settings such as clock configuration, UART parameters, and ThreadX memory allocation.
3. **Implement the BSP APIs**: Implement the interfaces defined in `/bsp/include/bsp/` using the vendor SDK or direct register access.
4. **Configure CMake**: Add the board target to `CMakeLists.txt`, supply the target's toolchain file under its own `cmake/`, build the BSP as a static library, and link it with the application in the target's `app/` directory.

Start from `templates/target/app/main.c`, which depends only on `<tx_api.h>` and the `<bsp/...>` contracts and therefore builds on any target that implements them. Grow it in place as the board needs; there is no shared `/apps` directory to link an application from.
