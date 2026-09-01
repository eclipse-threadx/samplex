#!/usr/bin/env python3
#
# Copyright (c) 2026 Eclipse ThreadX contributors
#
# This program and the accompanying materials are made available
# under the terms of the MIT license which is available at
# https://opensource.org/licenses/MIT.
#
# SPDX-License-Identifier: MIT
#

"""
Headless Renode Verification Test for the NUCLEO-F401RE ThreadX Demo.

Runs targets/STMicroelectronics/NUCLEO_F401RE/renode/nucleo_f401re_ci.resc and
asserts on the USART2 console output. The run advances a fixed span of virtual
time and quits on its own, so the result does not depend on host speed.

The application under test is apps/threadx_demo/main.c, shared with the
Microchip PolarFire SoC Icicle Kit. The assertions below are deliberately the
same ones targets/Microchip/POLARFIRE_ICICLE_RENODE/scripts/test_renode.py
makes with --app threadx_demo, so the two runs compare the same source built
for a 32-bit Cortex-M4 and a 64-bit RISC-V hart.
"""

import os
import re
import shutil
import subprocess
import sys
import threading
import queue
import time

RESC_NAME = "nucleo_f401re_ci.resc"

# The reporter thread prints these once the RTOS primitives have been exercised.
RUNS_RE = re.compile(r"Runs: Monitor: (\d+) \| Reporter: (\d+) \| Blink: (\d+) \| Timer Wakes: (\d+)")
RTOS_RE = re.compile(
    r"Mutex Locks: (\d+)/(\d+) \| Queue Msgs: (\d+) \| Event Wakes: (\d+) \| Sema Wakes: (\d+)"
)


def find_renode():
    renode_bin = shutil.which("renode")
    if renode_bin:
        return renode_bin

    win_paths = [
        r"C:\Program Files\Renode\renode.exe",
        os.path.expanduser(r"~\AppData\Local\Programs\Renode\renode.exe"),
    ]
    for path in win_paths:
        if os.path.isfile(path):
            return path

    return "renode"


def reader_thread_fn(pipe, q):
    try:
        for line in iter(pipe.readline, ""):
            q.put(line)
    except Exception:
        pass
    finally:
        pipe.close()


def run_test(test_timeout_mode=False):
    renode = find_renode()
    script_dir = os.path.dirname(os.path.abspath(__file__))
    target_dir = os.path.dirname(script_dir)
    resc_path = os.path.join(target_dir, "renode", RESC_NAME).replace("\\", "/")

    if test_timeout_mode:
        print("[*] Running intentional timeout test mode (2.0s deadline)...")
        timeout_seconds = 2.0
    else:
        print(f"[*] Starting headless Renode test using: {renode}")
        print(f"[*] Loading script: {resc_path}")
        timeout_seconds = 600.0

    cmd = [renode, "--plain", "--disable-gui", "--port", "-1",
           "-e", f"include @{resc_path}"]

    proc = subprocess.Popen(
        cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )

    output_q = queue.Queue()
    reader_t = threading.Thread(target=reader_thread_fn, args=(proc.stdout, output_q), daemon=True)
    reader_t.start()

    found_banner = False
    found_selftests = False
    found_selftest_failure = False
    found_led_and_timer = False
    found_rtos_primitives = False
    start_time = time.time()

    try:
        while time.time() - start_time < timeout_seconds:
            try:
                line = output_q.get(timeout=0.1)
                print(line, end="")
                if test_timeout_mode:
                    continue

                # apps/threadx_demo/main.c prints this, and it names no
                # board on purpose: the PolarFire suite asserts on the very
                # same line against the RISC-V build of that same source.
                if "Eclipse ThreadX Device Monitor Demo" in line:
                    found_banner = True
                if "[-] FAIL:" in line or "startup verification test(s) FAILED" in line:
                    found_selftest_failure = True
                if "[SELF-TEST] All startup verification tests PASSED!" in line:
                    found_selftests = True

                # The blink thread is the only caller of bsp_led_toggle(), and
                # timer wakes come from the 1 Hz ThreadX application timer, so a
                # non-zero pair covers the LED path and the timer service.
                m = RUNS_RE.search(line)
                if m and int(m.group(3)) > 0 and int(m.group(4)) > 0:
                    found_led_and_timer = True

                m = RTOS_RE.search(line)
                if m and all(int(g) > 0 for g in m.groups()):
                    found_rtos_primitives = True

                # A failed self-test is recorded but does not stop the run, so
                # the remaining assertions are still reported rather than hidden.
                if (found_banner and found_selftests and not found_selftest_failure
                        and found_led_and_timer and found_rtos_primitives):
                    print("\n[+] SUCCESS: boot banner, startup self-tests, LED and "
                          "timer activity, and all RTOS primitives verified!")
                    break
            except queue.Empty:
                if proc.poll() is not None:
                    break
    finally:
        try:
            proc.terminate()
            proc.wait(timeout=5)
        except Exception:
            try:
                proc.kill()
            except Exception:
                pass

    if test_timeout_mode:
        elapsed = time.time() - start_time
        print(f"\n[+] SUCCESS: Intentional timeout triggered after {elapsed:.2f}s "
              f"and terminated child process cleanly.")
        sys.exit(0)

    checks = {
        "boot banner reached the console": found_banner,
        "startup self-tests passed": found_selftests and not found_selftest_failure,
        "LED blink thread and application timer ran": found_led_and_timer,
        "mutex, queue, event flag and semaphore all exercised": found_rtos_primitives,
    }
    failed = [name for name, ok in checks.items() if not ok]

    if not failed:
        print("[+] Renode headless test PASSED.")
        sys.exit(0)

    print("\n[-] FAILED. Unmet assertions:")
    for name in failed:
        print("      - %s" % name)
    sys.exit(1)


if __name__ == "__main__":
    run_test(test_timeout_mode="--test-timeout" in sys.argv)
