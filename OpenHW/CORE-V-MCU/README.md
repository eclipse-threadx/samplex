# CORE-V MCU — Eclipse ThreadX Sample Applications

Sample applications for the
[OpenHW CORE-V MCU](https://github.com/openhwgroup/core-v-mcu) (CV32E40P,
RV32IMC) running [Eclipse ThreadX](https://github.com/eclipse-threadx/threadx).

All demos target a [Digilent Nexys A7](https://digilent.com/reference/programmable-logic/nexys-a7/start)
FPGA board loaded with the CORE-V MCU bitstream and use the
[Ashling Opella-LD](https://www.ashling.com/ashling-opella-ld/) JTAG probe for
flashing and debugging.  The BSP and ThreadX kernel are provided by the
[threadx-fd](https://github.com/fdesbiens/threadx-fd) submodule at
`OpenHW/libs/threadx`.

---

## Demos

### [`cli-test/`](cli-test/)

**CORE-V MCU CLI Test** — a multi-threaded command-line interface demo.

Two concurrent ThreadX threads run simultaneously:

- **Blinky** — toggles the on-board LED (GPIO pin 5) at 1 Hz.
- **CLI** — presents a UART terminal with typed commands for GPIO control,
  IO pad multiplexer configuration, and sensor access.  Built on the
  [QuickLogic CLI library](https://github.com/openhwgroup/core-v-mcu-cli-test)
  via the ThreadX FreeRTOS compatibility layer.

→ See [`cli-test/README.md`](cli-test/README.md) for full build and usage
instructions.

---

### [`slideshow/`](slideshow/)

**CORE-V MCU ThreadX Slideshow** — a five-screen interactive terminal demo.

A single ThreadX thread cycles through five full-screen displays every
10 seconds, with live data and keyboard navigation via arrow keys:

| Screen | Content |
|--------|---------|
| 1 | Eclipse ThreadX ASCII-art logo, version banner, uptime counter |
| 2 | OpenHW Group ASCII-art logo (green), live ISA string from `misa` CSR |
| 3 | Current room temperature from the ADT7420 I²C sensor in 5×5 big digits, with min/max/trend |
| 4 | Live Eclipse ThreadX kernel statistics (thread list, context switches, tick count) |
| 5 | RISC-V CSR register dump (`misa`, `mstatus`, `mtvec`, `mcycle`, vendor/arch IDs) |

→ See [`slideshow/README.md`](slideshow/README.md) for full build, deploy, and
configuration instructions.

---

## Common Prerequisites

| Tool | Notes |
|------|-------|
| `riscv64-unknown-elf-gcc` 13.x | Bare-metal RISC-V toolchain |
| `riscv64-unknown-elf-gdb` 13.x | Or `gdb-multiarch` |
| CMake ≥ 3.20 + Ninja | Build system |
| OpenOCD 0.11+ | Must support FTDI-based probes |

Run `setup_opella.sh` (in `OpenHW/libs/threadx/ports/risc-v32/gnu/example_build/core_v_mcu/`)
once to install the udev rules for the Opella-LD probe.

Each demo has its own `scripts/build.sh` (clean CMake/Ninja build) and
`scripts/deploy.sh` (flash via OpenOCD + GDB) scripts.  Both accept `--build`, `--debug`,
`--elf`, and `--openocd-cfg` options.

---

## License

Sample application source files are released under the
[MIT License](https://opensource.org/licenses/MIT).  AI-generated portions
carry a dual `MIT AND CC0-1.0` identifier.  ThreadX is MIT-licensed by
Microsoft / Eclipse Foundation; see
[`../libs/threadx/LICENSE.txt`](../libs/threadx/LICENSE.txt).
