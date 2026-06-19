#!/bin/bash
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

IP=${1:-"192.168.0.100"}
PORT=7
MSG="Hello ThreadX NetX Echo!"

echo "Connecting to $IP on port $PORT..."

# Check if netcat is installed
if command -v nc >/dev/null 2>&1; then
    # Use netcat with a 2-second timeout to send the message and read the echo
    RESPONSE=$(echo "$MSG" | nc -w 2 "$IP" "$PORT")
    if [ $? -eq 0 ] && [ ! -z "$RESPONSE" ]; then
        echo "Sending:  '$MSG'"
        echo "Received: '$RESPONSE'"
    else
        echo "Error: Failed to receive echo response. Is the server running?"
        exit 1
    fi
else
    # Fallback to Python if netcat is not available (standard on almost all Linux distros)
    echo "netcat (nc) not found, falling back to python3..."
    python3 -c "
import socket, sys
try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(2.0)
    s.connect(('$IP', $PORT))
    s.sendall(b'$MSG\n')
    data = s.recv(1024)
    print('Sending:  $MSG')
    print('Received: ' + data.decode().strip())
except Exception as e:
    print('Error:', e)
    sys.exit(1)
finally:
    s.close()
"
fi
