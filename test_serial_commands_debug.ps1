#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Serial Commands Debug Test
.DESCRIPTION
    Tests CLI commands via serial to debug command issues
#>

Write-Host "=== Serial Commands Debug Test ===" -ForegroundColor Blue

try {
    Write-Host "Opening serial port COM3..." -ForegroundColor Blue
    
    $port = New-Object System.IO.Ports.SerialPort
    $port.PortName = "COM3"
    $port.BaudRate = 115200
    $port.DataBits = 8
    $port.Parity = [System.IO.Ports.Parity]::None
    $port.StopBits = [System.IO.Ports.StopBits]::One
    $port.ReadTimeout = 5000
    $port.WriteTimeout = 3000
    
    $port.Open()
    Write-Host "Serial port opened successfully" -ForegroundColor Green
    
    # Wait for boot and flush
    Start-Sleep -Seconds 3
    if ($port.BytesToRead -gt 0) {
        $bootData = $port.ReadExisting()
        Write-Host "Flushed boot data: $($bootData.Length) bytes" -ForegroundColor Gray
    }
    
    # Establish CLI connection
    $port.Write([char]13)
    Start-Sleep -Milliseconds 300
    
    if ($port.BytesToRead -gt 0) {
        $prompt = $port.ReadExisting()
        Write-Host "CLI prompt: '$prompt'" -ForegroundColor Green
    }
    
    # Test commands one by one
    $commands = @("help", "task-stats", "uavcan-status", "stack-info")
    
    foreach ($cmd in $commands) {
        Write-Host "`n--- Testing: $cmd ---" -ForegroundColor Yellow
        
        # Send command
        $port.WriteLine($cmd)
        Write-Host "Sent: $cmd" -ForegroundColor Cyan
        
        # Wait for response with timeout
        $timeout = [System.DateTime]::Now.AddSeconds(5)
        $response = ""
        
        while ([System.DateTime]::Now -lt $timeout) {
            if ($port.BytesToRead -gt 0) {
                $newData = $port.ReadExisting()
                $response += $newData
                Write-Host "Received chunk: '$newData'" -ForegroundColor Gray
                
                # Check if we have a complete response (ends with prompt)
                if ($response -match '>\\s*$') {
                    break
                }
            }
            Start-Sleep -Milliseconds 100
        }
        
        if ($response.Length -gt 0) {
            Write-Host "Full response ($($response.Length) chars):" -ForegroundColor Green
            Write-Host "---START---" -ForegroundColor Magenta
            Write-Host $response -ForegroundColor White
            Write-Host "---END---" -ForegroundColor Magenta
        } else {
            Write-Host "No response received" -ForegroundColor Red
        }
    }
    
} catch {
    Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    if ($port -and $port.IsOpen) {
        $port.Close()
        Write-Host "`nSerial port closed" -ForegroundColor Blue
    }
}

Write-Host "Serial commands debug test completed" -ForegroundColor Blue