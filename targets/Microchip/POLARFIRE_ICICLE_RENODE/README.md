# Microchip PolarFire SoC Icicle Kit Target Integration (Renode 64-Bit RISC-V)

This directory contains the target Board Support Package (BSP) and condition-monitoring demonstration application for the **Microchip PolarFire SoC Icicle Kit** running in the **Renode** emulation environment.

---

## 1. Hardware Architecture Overview

* **Target Board**: Microchip PolarFire SoC Icicle Kit (`targets/Microchip/POLARFIRE_ICICLE_RENODE`)
* **Processor Subsystem**: 5 RISC-V Harts (1x E51 Monitor Core + 4x 64-bit U54 Application Cores)
* **Execution Hart**: **Hart 1 (`u54_1`)** executes ThreadX; Harts 2–4 are parked in `wfi` loops while Hart 0 (E51) remains under platform monitor supervision.
* **CPU Core Architecture**: 64-Bit RISC-V (`rv64gc` / `lp64d` ABI @ 600 MHz)
* **System DRAM**: 1 GiB LPDDR4 Memory (`0x80000000` – `0xC0000000`)
* **Machine Timer**: SiFive CLINT `mtime` running at 1 MHz (`0x02000000`, 10ms tick = 10,000 cycles)
* **Serial Debug Console**: Microchip MMUART1 (`0x20100000`) at 115200 baud (8-N-1)
* **Telemetry**: Simulated LM75 temperature data processed via ThreadX queues and event flags

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
Inside the Renode monitor:
```renode
include @targets/Microchip/POLARFIRE_ICICLE_RENODE/renode/polarfire_demo.resc
```
This free-runs the machine so the telemetry stream can be watched live.

### Automated Headless Test Runner:
```bash
python targets/Microchip/POLARFIRE_ICICLE_RENODE/scripts/test_renode.py
```
The runner drives `renode/polarfire_ci.resc`, which steps through fixed
virtual-time intervals rather than free-running, injects a byte into MMUART1 to
exercise the PLIC external-interrupt path, and quits on its own. It asserts on
four things and exits non-zero if any of them is missing:

| Assertion | Covers |
|---|---|
| Startup self-tests all passed | `_sbrk()` bounds against the heap reservation, the `bsp_ram_region()` invariant, timer catch-up, PLIC configuration |
| ThreadX system tick advancing | CLINT machine timer and `_tx_timer_interrupt` |
| LM75 overtemperature alarm | Queue, event flags, and the analyzer thread |
| PLIC IRQ 91 RX interrupt delivered | MMUART1 -> PLIC -> Hart 1 machine-mode trap path |

The last of these is the only check that proves the PLIC is programmed for the
right context. Reading the controller's registers back cannot: the machine-mode
context (1) and the supervisor-mode context (2) for Hart 1 both accept the
writes and read back identically, but only the former ever raises `MEIP`.

### Expected Output Stream (`mmuart1` @ 115200 baud):
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
