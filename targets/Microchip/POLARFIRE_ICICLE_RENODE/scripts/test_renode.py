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
Headless Renode verification tests for the PolarFire SoC Icicle Kit.

This target builds two executables, so this runner drives two suites:

    --app lm75          the board's own LM75 condition-monitoring demo
                        (app/main.c), which is the default
    --app threadx_demo  the shared portable demo (apps/threadx_demo/main.c),
                        the same source the NUCLEO-F401RE builds

They share every line of Renode process handling below and differ only in
which .resc they load and what they assert on, which is why they are one
script rather than two: the plumbing is what would drift if it were copied.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import threading
import queue
import time


class Assertions(object):
    """Consumes console lines and tracks which named checks have been met."""

    def __init__(self, names):
        self._met = dict((name, False) for name in names)

    def mark(self, name):
        self._met[name] = True

    def is_met(self, name):
        return self._met[name]

    def satisfied(self):
        return all(self._met.values())

    def unmet(self):
        return [name for name, ok in self._met.items() if not ok]


class Lm75Suite(object):
    """The board's own demo: sensor pipeline plus the MMUART1 receive path."""

    resc = "polarfire_ci.resc"
    # The .resc injects this byte into MMUART1 itself; Renode's monitor is not
    # on stdin, so the injection has to happen there rather than here.
    description = "LM75 condition-monitoring demo (app/main.c)"
    timeout_seconds = 120.0

    NAMES = (
        "startup self-tests passed",
        "ThreadX system tick advancing",
        "LM75 overtemperature alarm",
        "PLIC IRQ 91 RX interrupt delivered",
    )

    def __init__(self):
        self.checks = Assertions(self.NAMES)
        self.failed_selftest = False

    def feed(self, line):
        if "[-] FAIL:" in line or "startup verification test(s) FAILED" in line:
            self.failed_selftest = True
        if "[SELF-TEST] All startup verification tests PASSED!" in line:
            self.checks.mark("startup self-tests passed")
        if "Ticks:" in line:
            self.checks.mark("ThreadX system tick advancing")
        if "OVERTEMP ALARM TRIGGERED" in line:
            self.checks.mark("LM75 overtemperature alarm")
        # The only check that proves the PLIC is programmed for the machine-mode
        # context: reading its registers back cannot distinguish that from the
        # supervisor one. It now also proves bsp_console_set_rx_handler() took
        # effect, since the trap path dispatches through the registered handler.
        if "PLIC IRQ 91 handled" in line:
            self.checks.mark("PLIC IRQ 91 RX interrupt delivered")

    def success_message(self):
        return ("startup self-tests, ThreadX ticks, LM75 alarm, and PLIC RX "
                "interrupt all verified!")


class ThreadxDemoSuite(object):
    """The shared portable demo, asserted here exactly as it is on Cortex-M4."""

    resc = "polarfire_threadx_demo_ci.resc"
    description = "shared portable demo (apps/threadx_demo/main.c)"
    timeout_seconds = 600.0

    # The reporter thread prints these once the RTOS primitives have run.
    RUNS_RE = re.compile(
        r"Runs: Monitor: (\d+) \| Reporter: (\d+) \| Blink: (\d+) \| Timer Wakes: (\d+)")
    RTOS_RE = re.compile(
        r"Mutex Locks: (\d+)/(\d+) \| Queue Msgs: (\d+) \| Event Wakes: (\d+) \| Sema Wakes: (\d+)")

    NAMES = (
        "boot banner reached the console",
        "startup self-tests passed",
        "LED blink thread and application timer ran",
        "mutex, queue, event flag and semaphore all exercised",
    )

    def __init__(self):
        self.checks = Assertions(self.NAMES)
        self.failed_selftest = False

    def feed(self, line):
        # Deliberately board-neutral: the identical assertion runs against the
        # Cortex-M4 build of this same source.
        if "Eclipse ThreadX Device Monitor Demo" in line:
            self.checks.mark("boot banner reached the console")
        if "[-] FAIL:" in line or "startup verification test(s) FAILED" in line:
            self.failed_selftest = True
        if "[SELF-TEST] All startup verification tests PASSED!" in line:
            self.checks.mark("startup self-tests passed")

        # The blink thread is the only caller of bsp_led_toggle(), and timer
        # wakes come from the 1 Hz ThreadX application timer, so a non-zero
        # pair covers the LED path and the timer service.
        m = self.RUNS_RE.search(line)
        if m and int(m.group(3)) > 0 and int(m.group(4)) > 0:
            self.checks.mark("LED blink thread and application timer ran")

        m = self.RTOS_RE.search(line)
        if m and all(int(g) > 0 for g in m.groups()):
            self.checks.mark("mutex, queue, event flag and semaphore all exercised")

    def success_message(self):
        return ("boot banner, startup self-tests, LED and timer activity, and "
                "all RTOS primitives verified!")


SUITES = {
    "lm75": Lm75Suite,
    "threadx_demo": ThreadxDemoSuite,
}


def find_renode():
    # Check PATH first
    renode_bin = shutil.which("renode")
    if renode_bin:
        return renode_bin

    # Common Windows locations
    win_paths = [
        r"C:\Program Files\Renode\renode.exe",
        os.path.expanduser(r"~\AppData\Local\Programs\Renode\renode.exe")
    ]
    for path in win_paths:
        if os.path.isfile(path):
            return path

    return "renode"


def reader_thread_fn(pipe, q):
    try:
        for line in iter(pipe.readline, ''):
            q.put(line)
    except Exception:
        pass
    finally:
        pipe.close()


def run_test(app, test_timeout_mode=False):
    suite = SUITES[app]()
    renode = find_renode()
    script_dir = os.path.dirname(os.path.abspath(__file__))
    target_dir = os.path.dirname(script_dir)
    resc_path = os.path.join(target_dir, "renode", suite.resc).replace("\\", "/")

    if test_timeout_mode:
        print("[*] Running intentional timeout test mode (2.0s deadline against unresponsive wait)...")
        timeout_seconds = 2.0
    else:
        print(f"[*] Starting headless Renode test using: {renode}")
        print(f"[*] Suite: {suite.description}")
        print(f"[*] Loading script: {resc_path}")
        timeout_seconds = suite.timeout_seconds

    cmd = [
        renode,
        "--plain",
        "--disable-gui",
        "--port", "-1",
        "-e", f"include @{resc_path}"
    ]

    proc = subprocess.Popen(
        cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1
    )

    output_q = queue.Queue()
    reader_t = threading.Thread(target=reader_thread_fn, args=(proc.stdout, output_q), daemon=True)
    reader_t.start()

    start_time = time.time()

    try:
        while time.time() - start_time < timeout_seconds:
            try:
                line = output_q.get(timeout=0.1)
                print(line, end="")
                if test_timeout_mode:
                    continue

                suite.feed(line)

                # A failed self-test is recorded but does not stop the run, so
                # the remaining assertions are still reported rather than hidden.
                if suite.checks.satisfied() and not suite.failed_selftest:
                    print("\n[+] SUCCESS: " + suite.success_message())
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

    failed = suite.checks.unmet()
    if suite.failed_selftest and "startup self-tests passed" not in failed:
        failed.append("startup self-tests passed")

    if not failed:
        print(f"[+] Renode headless test PASSED ({suite.description}).")
        sys.exit(0)

    print("\n[-] FAILED. Unmet assertions:")
    for name in failed:
        print("      - %s" % name)
    sys.exit(1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--app", choices=sorted(SUITES), default="lm75",
                        help="which of this target's two executables to verify "
                             "(default: lm75)")
    parser.add_argument("--test-timeout", action="store_true",
                        help="exercise the runner's own deadline and cleanup path")
    args = parser.parse_args()

    run_test(args.app, test_timeout_mode=args.test_timeout)
