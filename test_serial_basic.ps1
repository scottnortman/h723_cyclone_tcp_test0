#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Basic Serial Connection Test
#>

param([string]$ComPort = "COM3")

Write-Host "Testing basic serial connection to $ComPort..." -ForegroundColor Blue

try {
    $port = New-Object System.IO.Ports.SerialPort $ComPort, 115200, None, 8, One
    $port.ReadTimeout = 2000
    $port.WriteTimeout = 2000
    
    Write-Host "Opening serial port..." -ForegroundColor Blue
    $port.Open()
    
    Write-Host "Port opened successfully" -ForegroundColor Green
    
    # Send multiple carriage returns
    Write-Host "Sending handshake..." -ForegroundColor Blue
    for ($i = 0; $i -lt 5; $i++) {
        $port.Write([byte]0x0D)
        Start-Sleep -Milliseconds 200
    }
    
    Start-Sleep -Milliseconds 1000
    
    # Check for any response
    if ($port.BytesToRead -gt 0) {
        $response = $port.ReadExisting()
        Write-Host "Response received:" -ForegroundColor Green
        Write-Host $response
        
        if ($response -match '>') {
            Write-Host "CLI prompt detected!" -ForegroundColor Green
            
            # Try a simple command
            Write-Host "Sending 'help' command..." -ForegroundColor Blue
            $port.WriteLine("help")
            Start-Sleep -Milliseconds 2000
            
            if ($port.BytesToRead -gt 0) {
                $helpResponse = $port.ReadExisting()
                Write-Host "Help response:" -ForegroundColor Green
                Write-Host $helpResponse
            } else {
                Write-Host "No response to help command" -ForegroundColor Yellow
            }
        } else {
            Write-Host "No CLI prompt found in response" -ForegroundColor Yellow
        }
    } else {
        Write-Host "No response from hardware" -ForegroundColor Red
        Write-Host "Hardware may not be booting or serial connection issue" -ForegroundColor Red
    }
    
} catch {
    Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    if ($port -and $port.IsOpen) {
        $port.Close()
        Write-Host "Serial port closed" -ForegroundColor Blue
    }
}

Write-Host "Basic serial test completed" -ForegroundColor Blue