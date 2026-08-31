# Microchip PolarFire SoC Icicle Kit Target Integration (Renode 64-Bit RISC-V)

This directory contains the target Board Support Package (BSP) for the **Microchip PolarFire SoC Icicle Kit** running in the **Renode** emulation environment, and it builds **two executables** against that one BSP:

| Executable | Application source | Why it lives there |
|---|---|---|
| `polarfire_icicle_demo.elf` | `app/main.c` | The board's own LM75 condition-monitoring demo. It models a sensor and drives the MMUART1 receive interrupt, so it is board specific and stays with the board. |
| `polarfire_threadx_demo.elf` | `apps/threadx_demo/main.c` | The shared portable demo, byte for byte the same source `targets/STMicroelectronics/NUCLEO_F401RE` builds for 32-bit Cortex-M4. Building it here is what makes the framework's portability claim something CI verifies on a second architecture rather than something the documentation asserts. |

Both are exercised by their own headless Renode suite, and CI runs both.

---

## 1. Hardware Architecture Overview

* **Target Board**: Microchip PolarFire SoC Icicle Kit (`targets/Microchip/POLARFIRE_ICICLE_RENODE`)
* **Processor Subsystem**: 5 RISC-V Harts (1x E51 Monitor Core + 4x 64-bit U54 Application Cores)
* **Execution Hart**: **Hart 1 (`u54_1`)** executes ThreadX; Harts 2–4 are parked in `wfi` loops while Hart 0 (E51) remains under platform monitor supervision.
* **CPU Core Architecture**: 64-Bit RISC-V (`rv64gc` / `lp64d` ABI @ 600 MHz)
* **System DRAM**: 1 GiB LPDDR4 Memory (`0x80000000` – `0xC0000000`)
* **Machine Timer**: SiFive CLINT `mtime` running at 1 MHz (`0x02000000`, 10ms tick = 10,000 cycles)
* **Serial Debug Console**: Microchip MMUART1 (`0x20100000`) at 115200 baud (8-N-1)
* **Telemetry**: Simulated LM75 temperature data processed via ThreadX queues and event flags (LM75 demo only)

---

## 2. Compilation Instructions

Requirements:
* CMake 3.20+ and Ninja (or Make)
* 64-bit RISC-V GCC cross-compiler (`riscv64-none-elf-gcc`, `riscv-none-elf-gcc`, or xPack RISC-V GCC 14.2.0)

### On Windows (PowerShell):
```powershell
powershell -ExecutionPolicy Bypass -File .\targets\Microchip\POLARFIRE_ICICLE_RENODE\scripts\build.ps1 -Clean -Rebuild
```

### On Linux / macOS (Bash):
```bash
bash targets/Microchip/POLARFIRE_ICICLE_RENODE/scripts/build.sh --rebuild
```

---

## 3. Renode Execution & Verification

### Interactive Simulation (GUI / Terminal Analyzers):
Inside the Renode monitor, pick the executable to watch free-run:
```renode
include @targets/Microchip/POLARFIRE_ICICLE_RENODE/renode/polarfire_demo.resc
include @targets/Microchip/POLARFIRE_ICICLE_RENODE/renode/polarfire_threadx_demo.resc
```

### Automated Headless Test Runner:
One runner drives both suites; `--app` selects which executable to verify.
```bash
python targets/Microchip/POLARFIRE_ICICLE_RENODE/scripts/test_renode.py --app lm75          # default
python targets/Microchip/POLARFIRE_ICICLE_RENODE/scripts/test_renode.py --app threadx_demo
```
Both step through fixed virtual-time intervals rather than free-running and
quit on their own, and both exit non-zero if any assertion is missing. They
share every line of Renode process handling, which is the reason they are one
script: the plumbing is what would drift if it were copied.

**`--app lm75`** drives `renode/polarfire_ci.resc`, which also injects a byte
into MMUART1 to exercise the PLIC external-interrupt path:

| Assertion | Covers |
|---|---|
| Startup self-tests all passed | `_sbrk()` bounds against the heap reservation, the `bsp_ram_region()` invariant, timer catch-up, PLIC configuration |
| ThreadX system tick advancing | CLINT machine timer and `_tx_timer_interrupt` |
| LM75 overtemperature alarm | Queue, event flags, and the analyzer thread |
| PLIC IRQ 91 RX interrupt delivered | MMUART1 -> PLIC -> Hart 1 machine-mode trap path, and the handler registered through `bsp_console_set_rx_handler()` |

The last of these is the only check that proves the PLIC is programmed for the
right context. Reading the controller's registers back cannot: the machine-mode
context (1) and the supervisor-mode context (2) for Hart 1 both accept the
writes and read back identically, but only the former ever raises `MEIP`.

**`--app threadx_demo`** drives `renode/polarfire_threadx_demo_ci.resc`. Its
four assertions are deliberately the same ones
`targets/STMicroelectronics/NUCLEO_F401RE/scripts/test_renode.py` makes, so the
two runs compare one source file built for two architectures:

| Assertion | Covers |
|---|---|
| Boot banner reached the console | `bsp_console_write()` through `printf()`, and a banner that names no board |
| Startup self-tests all passed | The same BSP checks as above, reached through the portable `<bsp/selftest.h>` callback |
| LED blink thread and application timer ran | `bsp_led_toggle()` and the ThreadX application timer |
| Mutex, queue, event flag and semaphore all exercised | The RTOS primitives, sized out of the pool `bsp_ram_region()` reports |

The shared demo prints through `printf()` where the LM75 demo calls
`bsp_console_write()` directly, so it is the first executable on this board to
pull in newlib stdio. Measured heap use is **2944 bytes** of the 64 KB
`BSP_HEAP_RESERVE_BYTES`, and the whole run completes with the reservation
temporarily cut to 4 KB, so the reservation holds with room to spare.

### Expected Output Stream, `--app lm75` (`mmuart1` @ 115200 baud):
```text
====================================================
Microchip PolarFire SoC Icicle Kit (Renode Target)
64-Bit RISC-V Industrial LM75 Condition-Monitoring App
====================================================
[SELF-TEST] Starting Hardware & Runtime Verification...
[+] PASS: _sbrk() valid allocation returned base pointer
[+] PASS: _sbrk() underflow guard rejected with EINVAL
[+] PASS: _sbrk() overflow guard rejected with ENOMEM
[+] PASS: HWTimer catch-up (missed deadline rebased to ..., pending deadline advanced to ...)
[+] PASS: PLIC Hart 1 (IRQ 91 prio=1 en=0x08000000 thresh=0 mie=0x800)
[SELF-TEST] All startup verification tests PASSED!

[+] PASS: Queue 16-byte structure round-trip verified
[Monitor] Ticks: 0 | Active Runs: Sampler=1, Analyzer=1, Reporter=1
[Monitor] Ticks: 100 | Active Runs: Sampler=3, Analyzer=3, Reporter=2
[Console RX] PLIC IRQ 91 handled: byte 'X' received and processed by ThreadX
[Monitor] Ticks: 200 | Active Runs: Sampler=5, Analyzer=5, Reporter=3
[LM75 Sensor] Temperature: OVERTEMP ALARM TRIGGERED (>45.0C)
```

### Expected Output Stream, `--app threadx_demo`:
```text
==========================================
Eclipse ThreadX Device Monitor Demo
==========================================

System Status:
------------------------------------------
Uptime:           4 s
Byte Pool Size:   1073585120 bytes
Allocated Memory: 18896 bytes
Free Memory:      1073566224 bytes
------------------------------------------

Thread Name      Priority State      Run Count    Stack Peak (Max / Size)
----------------------------------------------------------------------------
monitor thread   9        READY      41            536 / 2048 bytes (26%)
reporter thread  10       READY      2            1048 / 2048 bytes (51%)
blink thread     11       READY      8             536 / 2048 bytes (26%)
...
----------------------------------------------------------------------------
Runs: Monitor: 41 | Reporter: 3 | Blink: 8 | Timer Wakes: 4
RTOS Showcase: Mutex Locks: 40/40 | Queue Msgs: 20 | Event Wakes: 8 | Sema Wakes: 8
```

Thread stacks are 2048 bytes here and 1024 on the NUCLEO-F401RE from the one
`THREAD_STACK_SIZE` expression, which is written in machine words rather than
bytes: every saved register and spilled pointer doubles in width on a 64-bit
hart, as does the newlib `printf()` call chain. The byte pool spans the whole
region `bsp_ram_region()` reports, which on this board is the DRAM above the
heap reservation - roughly 1 GiB.
