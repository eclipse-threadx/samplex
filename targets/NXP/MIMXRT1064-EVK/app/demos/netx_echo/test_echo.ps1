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
    [string]$IP = "192.168.0.100",
    [int]$Port = 7
)

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host " NetX Duo Virtual Networking Verification" -ForegroundColor Cyan
Write-Host " Target Device: $IP (Port: $Port)" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

$AllPassed = $true

# ----------------------------------------------------
# Test 1: ICMP Ping
# ----------------------------------------------------
Write-Host "[Test 1/3] Testing ICMP Ping (Echo Request)..." -ForegroundColor Yellow
$PingSuccess = $false
try {
    $pingRes = Test-Connection -ComputerName $IP -Count 2 -Quiet -ErrorAction Stop
    if ($pingRes) {
        $PingSuccess = $true
    }
} catch {
    # Fallback to ping.exe
    $res = ping -n 2 -w 1000 $IP
    if ($LASTEXITCODE -eq 0) {
        $PingSuccess = $true
    }
}

if ($PingSuccess) {
    Write-Host "[PASS] ICMP Ping responded successfully from $IP" -ForegroundColor Green
} else {
    Write-Host "[FAIL] ICMP Ping timed out or failed to reach $IP" -ForegroundColor Red
    $AllPassed = $false
}
Write-Host ""

# ----------------------------------------------------
# Test 2: UDP Echo
# ----------------------------------------------------
Write-Host "[Test 2/3] Testing UDP Echo on port $Port..." -ForegroundColor Yellow
$UdpClient = New-Object System.Net.Sockets.UdpClient
$UdpClient.Client.ReceiveTimeout = 3000

$UdpMsg = "Hello ThreadX UDP Echo!"
$UdpBytes = [System.Text.Encoding]::ASCII.GetBytes($UdpMsg)

try {
    $UdpClient.Connect($IP, $Port)
    [void]$UdpClient.Send($UdpBytes, $UdpBytes.Length)
    Write-Host "Sent UDP:     '$UdpMsg'"

    $RemoteEndpoint = New-Object System.Net.IPEndPoint([System.Net.IPAddress]::Any, 0)
    $ReceivedBytes = $UdpClient.Receive([ref]$RemoteEndpoint)
    $ReceivedMsg = [System.Text.Encoding]::ASCII.GetString($ReceivedBytes)
    Write-Host "Received UDP: '$ReceivedMsg'"

    if ($ReceivedMsg -eq $UdpMsg) {
        Write-Host "[PASS] UDP Echo verified successfully!" -ForegroundColor Green
    } else {
        Write-Host "[FAIL] UDP payload mismatch: expected '$UdpMsg', got '$ReceivedMsg'" -ForegroundColor Red
        $AllPassed = $false
    }
} catch {
    Write-Host "[FAIL] UDP Echo failed: $_" -ForegroundColor Red
    $AllPassed = $false
} finally {
    $UdpClient.Close()
}
Write-Host ""

# ----------------------------------------------------
# Test 3: TCP Echo
# ----------------------------------------------------
Write-Host "[Test 3/3] Testing TCP Echo on port $Port..." -ForegroundColor Yellow
$TcpClient = $null
try {
    $TcpClient = New-Object System.Net.Sockets.TcpClient
    $TcpClient.ReceiveTimeout = 3000
    $TcpClient.SendTimeout = 3000
    $TcpClient.Connect($IP, $Port)

    $Stream = $TcpClient.GetStream()
    $Writer = New-Object System.IO.StreamWriter($Stream)
    $Reader = New-Object System.IO.StreamReader($Stream)

    $TcpMsg = "Hello ThreadX TCP Echo!"
    Write-Host "Sent TCP:     '$TcpMsg'"
    $Writer.WriteLine($TcpMsg)
    $Writer.Flush()

    $TcpResponse = $Reader.ReadLine()
    Write-Host "Received TCP: '$TcpResponse'"

    if ($TcpResponse -eq $TcpMsg) {
        Write-Host "[PASS] TCP Echo verified successfully!" -ForegroundColor Green
    } else {
        Write-Host "[FAIL] TCP payload mismatch: expected '$TcpMsg', got '$TcpResponse'" -ForegroundColor Red
        $AllPassed = $false
    }
} catch {
    Write-Host "[FAIL] TCP Echo failed: $_" -ForegroundColor Red
    $AllPassed = $false
} finally {
    if ($TcpClient) { $TcpClient.Close() }
}
Write-Host ""

# ----------------------------------------------------
# Summary
# ----------------------------------------------------
Write-Host "==========================================" -ForegroundColor Cyan
if ($AllPassed) {
    Write-Host " ALL TESTS PASSED! NetX Duo is fully verified." -ForegroundColor Green
} else {
    Write-Host " SOME TESTS FAILED. Verify Renode TAP and network connection." -ForegroundColor Red
}
Write-Host "==========================================" -ForegroundColor Cyan
