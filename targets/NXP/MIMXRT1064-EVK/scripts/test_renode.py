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
# Contributors:
#    Ali Eissa - 2026 version.
#    Assisted-by: Google DeepMind Antigravity (Gemini 3.8 Flash)

"""
Headless Renode Verification Test for NXP i.MX RT1064-EVK Demos.

Runs deterministic virtual-time emulation in Antmicro Renode and asserts on
the streamed LPUART1 console output via showAnalyzer.
"""

import argparse
import os
import queue
import shutil
import subprocess
import sys
import threading
import time

DEMO_CONFIGS = {
    "threadx_basic": {
        "description": "ThreadX Core Basic Demo (Task Scheduling & GPIO LED)",
        "resc": "mimxrt1064-headless-single.resc",
        "multinode": False,
        "marker": "Executing periodic task",
    },
    "netx_echo": {
        "description": "NetX Duo Multi-Node Echo Demo (ICMP, UDP, TCP)",
        "resc": "mimxrt1064-headless-multinode.resc",
        "multinode": True,
        "marker": "[VERIFICATION SUCCESS] ALL NETWORK TESTS PASSED!",
    },
    "netx_trng_console": {
        "description": "NetX Duo Multi-Node Hardware TRNG & Console Demo",
        "resc": "mimxrt1064-headless-multinode.resc",
        "multinode": True,
        "marker": "[VERIFICATION SUCCESS] ALL TRNG & CONSOLE TESTS PASSED!",
    },
}


def find_renode():
    renode_bin = shutil.which("renode")
    if renode_bin:
        return renode_bin

    candidates = [
        r"C:\Program Files\Renode\renode.exe",
        os.path.expanduser(r"~\AppData\Local\Programs\Renode\renode.exe"),
        os.path.expanduser(r"~/renode/renode"),
        "/opt/renode/renode",
        "/usr/bin/renode",
    ]
    for path in candidates:
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


def run_test(demo_name, seed=None, timeout_seconds=300):
    if demo_name not in DEMO_CONFIGS:
        print(f"[FAIL] Unknown demo: {demo_name}. Choices: {list(DEMO_CONFIGS.keys())}")
        return 1

    config = DEMO_CONFIGS[demo_name]
    renode = find_renode()

    script_dir = os.path.dirname(os.path.abspath(__file__))
    board_dir = os.path.dirname(script_dir)
    build_dir = os.path.join(board_dir, "build")
    resc_rel = f"renode/{config['resc']}"

    # Check binary existence
    demo_dir = os.path.join(build_dir, "app", "demos", demo_name)
    server_elf = os.path.join(demo_dir, "mimxrt1064_threadx.elf")
    if not os.path.isfile(server_elf):
        fallback = os.path.join(build_dir, "mimxrt1064_threadx.elf")
        if os.path.isfile(fallback):
            server_elf = fallback
        else:
            print(f"[FAIL] Server ELF binary not found: {server_elf}")
            return 1

    if config["multinode"]:
        client_elf = os.path.join(demo_dir, "mimxrt1064_client.elf")
        if not os.path.isfile(client_elf):
            fallback_c = os.path.join(build_dir, "mimxrt1064_client.elf")
            if os.path.isfile(fallback_c):
                client_elf = fallback_c
            else:
                print(f"[FAIL] Client ELF binary not found: {client_elf}")
                return 1

    server_elf_rel = os.path.relpath(server_elf, board_dir).replace("\\", "/")
    cmd_parts = []
    if seed is not None:
        cmd_parts.append(f"emulation SetSeed {seed}")

    cmd_parts.append("$platform = @renode/mimxrt1064-evk.repl")

    if config["multinode"]:
        client_elf_rel = os.path.relpath(client_elf, board_dir).replace("\\", "/")
        cmd_parts.append(f"$bin_server = @{server_elf_rel}")
        cmd_parts.append(f"$bin_client = @{client_elf_rel}")
    else:
        cmd_parts.append(f"$bin = @{server_elf_rel}")

    cmd_parts.append(f"include @{resc_rel}")
    renode_script_cmd = "; ".join(cmd_parts)

    cmd = [
        renode,
        "--plain",
        "--disable-gui",
        "--port", "-1",
        "-e", renode_script_cmd
    ]

    print("==========================================")
    print("Renode Headless CI Automated Test Runner")
    print("==========================================")
    print(f"Active Demo: {demo_name}")
    print(f"Test Suite:  {config['description']}")
    print(f"Script:      {config['resc']}")
    if seed is not None:
        print(f"Seed:        {seed} (Deterministic)")
    print(f"Timeout:     {timeout_seconds}s")
    print(f"Engine:      {renode}")
    print("")
    print("[INFO] Launching Renode in headless mode...")
    print("")

    proc = subprocess.Popen(
        cmd,
        cwd=board_dir,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )

    os.makedirs(build_dir, exist_ok=True)
    log_file_path = os.path.join(build_dir, f"renode_test_{demo_name}.log")

    output_q = queue.Queue()
    reader_t = threading.Thread(target=reader_thread_fn, args=(proc.stdout, output_q), daemon=True)
    reader_t.start()

    found_marker = False
    start_time = time.time()

    with open(log_file_path, "w", encoding="utf-8") as log_f:
        try:
            while time.time() - start_time < timeout_seconds:
                try:
                    line = output_q.get(timeout=0.1)
                    sys.stdout.write(line)
                    sys.stdout.flush()
                    log_f.write(line)
                    log_f.flush()

                    if config["marker"] in line:
                        found_marker = True
                        break
                except queue.Empty:
                    if proc.poll() is not None:
                        # Drain remaining output
                        while not output_q.empty():
                            line = output_q.get_nowait()
                            sys.stdout.write(line)
                            sys.stdout.flush()
                            log_f.write(line)
                            log_f.flush()
                            if config["marker"] in line:
                                found_marker = True
                        break
        finally:
            try:
                proc.terminate()
                proc.wait(timeout=3)
            except Exception:
                try:
                    proc.kill()
                except Exception:
                    pass

    print("")
    print("==========================================")
    if found_marker:
        print(f"[PASS] CI Automated Verification Succeeded for '{demo_name}'!")
        print("==========================================")
        return 0
    else:
        print(f"[FAIL] CI Automated Verification Failed or Timed Out for '{demo_name}'!")
        print(f"       Expected assertion marker: '{config['marker']}'")
        print("==========================================")
        return 1


def main():
    parser = argparse.ArgumentParser(description="NXP MIMXRT1064-EVK Headless Renode Test Runner")
    parser.add_argument("-d", "--demo", default="threadx_basic",
                        choices=["threadx_basic", "netx_echo", "netx_trng_console"],
                        help="Demo application to verify")
    parser.add_argument("-s", "--seed", type=int, default=None,
                        help="Deterministic simulation seed")
    parser.add_argument("-t", "--timeout", type=int, default=300,
                        help="Timeout in seconds (default: 300)")

    args = parser.parse_args()
    seed = args.seed
    if args.demo == "netx_trng_console" and seed is None:
        seed = 12345

    ret = run_test(args.demo, seed=seed, timeout_seconds=args.timeout)
    sys.exit(ret)


if __name__ == "__main__":
    main()
