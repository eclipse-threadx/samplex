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

param (
    [string]$IP = "192.168.0.100"
)

Write-Host "Connecting to $IP on port 7..."
try {
    $client = New-Object System.Net.Sockets.TcpClient($IP, 7)
    $stream = $client.GetStream()
    $writer = New-Object System.IO.StreamWriter($stream)
    $reader = New-Object System.IO.StreamReader($stream)

    $msg = "Hello ThreadX NetX Echo!"
    Write-Host "Sending: '$msg'"
    $writer.WriteLine($msg)
    $writer.Flush()

    $response = $reader.ReadLine()
    Write-Host "Received: '$response'"
} catch {
    Write-Error "Failed to connect or communicate: $_"
} finally {
    if ($client) { $client.Close() }
}
