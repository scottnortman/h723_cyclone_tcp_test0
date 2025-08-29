#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Compare Serial vs Telnet Command Responses
.DESCRIPTION
    Test the same commands on both interfaces to identify differences
#>

Write-Host "=== Serial vs Telnet Command Comparison ===" -ForegroundColor Blue

$testCommands = @("help", "heap-info", "stack-info", "memory-info", "uavcan-status")

# Test Serial Interface
Write-Host "`n=== SERIAL INTERFACE TEST ===" -ForegroundColor Yellow

try {
    $port = New-Object System.IO.Ports.SerialPort
    $port.PortName = "COM3"
    $port.BaudRate = 115200
    $port.DataBits = 8
    $port.Parity = [System.IO.Ports.Parity]::None
    $port.StopBits = [System.IO.Ports.StopBits]::One
    $port.ReadTimeout = 5000
    $port.WriteTimeout = 3000
    
    $port.Open()
    Write-Host "✅ Serial port opened" -ForegroundColor Green
    
    # Handshake
    Start-Sleep -Seconds 1
    if ($port.BytesToRead -gt 0) { $port.ReadExisting() | Out-Null }
    $port.Write([char]13)
    Start-Sleep -Milliseconds 300
    if ($port.BytesToRead -gt 0) { $port.ReadExisting() | Out-Null }
    
    Write-Host "✅ Serial CLI connected" -ForegroundColor Green
    
    foreach ($cmd in $testCommands) {
        Write-Host "`nTesting '$cmd' via SERIAL:" -ForegroundColor Cyan
        $port.WriteLine($cmd)
        Start-Sleep -Milliseconds 2000
        
        $response = ""
        if ($port.BytesToRead -gt 0) {
            $response = $port.ReadExisting()
        }
        
        if ($response.Length -gt 10) {
            Write-Host "  ✅ SUCCESS - $($response.Length) characters" -ForegroundColor Green
        } else {
            Write-Host "  ❌ FAILED - $($response.Length) characters" -ForegroundColor Red
        }
    }
    
    $port.Close()
    
} catch {
    Write-Host "❌ Serial test error: $($_.Exception.Message)" -ForegroundColor Red
    if ($port -and $port.IsOpen) { $port.Close() }
}

# Test Telnet Interface
Write-Host "`n=== TELNET INTERFACE TEST ===" -ForegroundColor Yellow

try {
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.ReceiveTimeout = 10000
    $tcpClient.SendTimeout = 5000
    
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    Write-Host "✅ Telnet connection established" -ForegroundColor Green
    
    Start-Sleep -Seconds 2
    
    # Initial handshake
    $buffer = New-Object byte[] 4096
    $crBytes = [System.Text.Encoding]::ASCII.GetBytes("`r")
    $stream.Write($crBytes, 0, $crBytes.Length)
    $stream.Flush()
    Start-Sleep -Milliseconds 500
    
    foreach ($cmd in $testCommands) {
        Write-Host "`nTesting '$cmd' via TELNET:" -ForegroundColor Cyan
        
        # Send command
        $cmdBytes = [System.Text.Encoding]::ASCII.GetBytes("$cmd`r")
        $stream.Write($cmdBytes, 0, $cmdBytes.Length)
        $stream.Flush()
        
        # Wait for response
        Start-Sleep -Seconds 3
        
        $response = ""
        $attempts = 0
        while ($attempts -lt 15) {
            if ($stream.DataAvailable) {
                $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
                $newData = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
                $response += $newData
            }
            Start-Sleep -Milliseconds 200
            $attempts++
        }
        
        if ($response.Length -gt 10) {
            Write-Host "  ✅ SUCCESS - $($response.Length) characters" -ForegroundColor Green
        } else {
            Write-Host "  ❌ FAILED - $($response.Length) characters" -ForegroundColor Red
            if ($response.Length -gt 0) {
                Write-Host "    Raw response: '$response'" -ForegroundColor DarkGray
            }
        }
    }
    
} catch {
    Write-Host "❌ Telnet test error: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    if ($stream) {
        $stream.Close()
        $stream.Dispose()
    }
    if ($tcpClient) {
        $tcpClient.Close()
        $tcpClient.Dispose()
    }
}

Write-Host "`n=== COMPARISON COMPLETE ===" -ForegroundColor Blue