#!/usr/bin/env bash
#  Copyright (c) 2026 Eclipse ThreadX contributors
# 
#  This program and the accompanying materials are made available 
#  under the terms of the MIT license which is available at
#  https://opensource.org/license/mit.
# 
#  SPDX-License-Identifier: MIT
# 
#  Contributors: 
#     Ali Eissa - 2026 version.

set -e

IP=${1:-"192.168.0.100"}
PORT=7

echo "=========================================="
echo " NetX Duo Virtual Networking Verification"
echo " Target Device: ${IP} (Port: ${PORT})"
echo "=========================================="
echo ""

ALL_PASSED=1

# 1. ICMP Ping Test
echo "[Test 1/3] Testing ICMP Ping (Echo Request)..."
if ping -c 2 -W 2 "${IP}" > /dev/null 2>&1; then
    echo "[PASS] ICMP Ping responded successfully from ${IP}"
else
    echo "[FAIL] ICMP Ping timed out or failed to reach ${IP}"
    ALL_PASSED=0
fi
echo ""

# 2. UDP Echo Test
echo "[Test 2/3] Testing UDP Echo on port ${PORT}..."
UDP_MSG="Hello ThreadX UDP Echo!"
UDP_RES=$(python3 -c "
import socket, sys
try:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(3.0)
    s.sendto(b'${UDP_MSG}', ('${IP}', ${PORT}))
    data, _ = s.recvfrom(1024)
    print(data.decode('ascii', errors='ignore'))
except Exception as e:
    sys.exit(1)
finally:
    s.close()
" 2>/dev/null || true)

if [ "${UDP_RES}" = "${UDP_MSG}" ]; then
    echo "Sent UDP:     '${UDP_MSG}'"
    echo "Received UDP: '${UDP_RES}'"
    echo "[PASS] UDP Echo verified successfully!"
else
    echo "[FAIL] UDP Echo failed (got '${UDP_RES}')"
    ALL_PASSED=0
fi
echo ""

# 3. TCP Echo Test
echo "[Test 3/3] Testing TCP Echo on port ${PORT}..."
TCP_MSG="Hello ThreadX TCP Echo!"
TCP_RES=$(python3 -c "
import socket, sys
try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(3.0)
    s.connect(('${IP}', ${PORT}))
    s.sendall(b'${TCP_MSG}\n')
    data = s.recv(1024)
    print(data.decode('ascii', errors='ignore').strip())
except Exception as e:
    sys.exit(1)
finally:
    s.close()
" 2>/dev/null || true)

if [ "${TCP_RES}" = "${TCP_MSG}" ]; then
    echo "Sent TCP:     '${TCP_MSG}'"
    echo "Received TCP: '${TCP_RES}'"
    echo "[PASS] TCP Echo verified successfully!"
else
    echo "[FAIL] TCP Echo failed (got '${TCP_RES}')"
    ALL_PASSED=0
fi
echo ""

echo "=========================================="
if [ "${ALL_PASSED}" -eq 1 ]; then
    echo " ALL TESTS PASSED! NetX Duo is fully verified."
else
    echo " SOME TESTS FAILED. Verify Renode TAP and network connection."
fi
echo "=========================================="
