#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Basic Telnet Connectivity Test
.DESCRIPTION
    Test basic telnet connectivity and initial response
#>

Write-Host "=== Basic Telnet Connectivity Test ===" -ForegroundColor Blue

# Step 1: Check if port 23 is open
Write-Host "`n--- Step 1: Port Connectivity ---" -ForegroundColor Yellow
try {
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $result = $tcpClient.BeginConnect("192.168.0.20", 23, $null, $null)
    $success = $result.AsyncWaitHandle.WaitOne(5000, $false)
    
    if ($success) {
        Write-Host "✅ Port 23 is open and accepting connections" -ForegroundColor Green
        $tcpClient.EndConnect($result)
    } else {
        Write-Host "❌ Port 23 connection timeout" -ForegroundColor Red
        $tcpClient.Close()
        exit 1
    }
    $tcpClient.Close()
} catch {
    Write-Host "❌ Port 23 connection failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

# Step 2: Test basic connection and any initial data
Write-Host "`n--- Step 2: Basic Connection Test ---" -ForegroundColor Yellow
try {
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.ReceiveTimeout = 10000
    $tcpClient.SendTimeout = 5000
    
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    Write-Host "✅ TCP connection established" -ForegroundColor Green
    
    # Wait for any initial banner or data
    Write-Host "Waiting 5 seconds for any initial data..." -ForegroundColor Gray
    Start-Sleep -Seconds 5
    
    $buffer = New-Object byte[] 2048
    $initialData = ""
    
    if ($stream.DataAvailable) {
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        $initialData = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
        Write-Host "Initial data received ($bytesRead bytes):" -ForegroundColor Cyan
        Write-Host "'$initialData'" -ForegroundColor Gray
    } else {
        Write-Host "No initial data received" -ForegroundColor Yellow
    }
    
    # Step 3: Send carriage return and check response
    Write-Host "`n--- Step 3: Handshake Test ---" -ForegroundColor Yellow
    Write-Host "Sending carriage return..." -ForegroundColor Gray
    
    $crBytes = [System.Text.Encoding]::ASCII.GetBytes("`r")
    $stream.Write($crBytes, 0, $crBytes.Length)
    $stream.Flush()
    
    # Wait for response with multiple checks
    $response = ""
    for ($i = 1; $i -le 20; $i++) {
        Start-Sleep -Milliseconds 250
        
        if ($stream.DataAvailable) {
            $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
            $newData = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
            $response += $newData
            Write-Host "  Check $i`: Received $bytesRead bytes" -ForegroundColor Gray
        }
        
        # Check if we got a prompt
        if ($response -match '>') {
            Write-Host "✅ Prompt detected!" -ForegroundColor Green
            break
        }
    }
    
    Write-Host "Total response: '$response'" -ForegroundColor Cyan
    Write-Host "Response length: $($response.Length) characters" -ForegroundColor Cyan
    
    if ($response -match '>') {
        Write-Host "✅ Telnet handshake successful - CLI is responding" -ForegroundColor Green
    } else {
        Write-Host "❌ No prompt received - CLI may not be active" -ForegroundColor Red
    }
    
    $stream.Close()
    $tcpClient.Close()
    
} catch {
    Write-Host "❌ Connection test failed: $($_.Exception.Message)" -ForegroundColor Red
}

# Step 4: Check via serial if telnet task is running
Write-Host "`n--- Step 4: Check Telnet Task Status via Serial ---" -ForegroundColor Yellow
try {
    $port = New-Object System.IO.Ports.SerialPort
    $port.PortName = "COM3"
    $port.BaudRate = 115200
    $port.DataBits = 8
    $port.Parity = [System.IO.Ports.Parity]::None
    $port.StopBits = [System.IO.Ports.StopBits]::One
    $port.ReadTimeout = 3000
    $port.WriteTimeout = 3000
    
    $port.Open()
    
    # Serial handshake
    Start-Sleep -Milliseconds 500
    if ($port.BytesToRead -gt 0) { $port.ReadExisting() | Out-Null }
    $port.Write([char]13)
    Start-Sleep -Milliseconds 300
    if ($port.BytesToRead -gt 0) { $port.ReadExisting() | Out-Null }
    
    # Check task list
    $port.WriteLine("stack-info")
    Start-Sleep -Milliseconds 2000
    
    $taskResponse = ""
    if ($port.BytesToRead -gt 0) {
        $taskResponse = $port.ReadExisting()
    }
    
    if ($taskResponse -match "CmdDualTelnet|TelnetCLI") {
        Write-Host "✅ Telnet task is running" -ForegroundColor Green
        
        # Show telnet task line
        $lines = $taskResponse -split "`n"
        $telnetLine = $lines | Where-Object { $_ -match "Telnet|CmdDual" }
        if ($telnetLine) {
            Write-Host "Telnet task: $telnetLine" -ForegroundColor Cyan
        }
    } else {
        Write-Host "❌ No telnet task found in task list" -ForegroundColor Red
        Write-Host "Task list response:" -ForegroundColor Gray
        Write-Host $taskResponse -ForegroundColor DarkGray
    }
    
    $port.Close()
    
} catch {
    Write-Host "❌ Serial check failed: $($_.Exception.Message)" -ForegroundColor Red
    if ($port -and $port.IsOpen) { $port.Close() }
}

Write-Host "`n=== Basic Telnet Connectivity Test Complete ===" -ForegroundColor Blue