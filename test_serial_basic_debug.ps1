#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Basic Serial Debug Test
.DESCRIPTION
    Tests basic serial connectivity to check if hardware is responding
#>

Write-Host "=== Basic Serial Debug Test ===" -ForegroundColor Blue

try {
    Write-Host "Opening serial port COM3..." -ForegroundColor Blue
    
    $port = New-Object System.IO.Ports.SerialPort
    $port.PortName = "COM3"
    $port.BaudRate = 115200
    $port.DataBits = 8
    $port.Parity = [System.IO.Ports.Parity]::None
    $port.StopBits = [System.IO.Ports.StopBits]::One
    $port.ReadTimeout = 3000
    $port.WriteTimeout = 3000
    
    $port.Open()
    Write-Host "Serial port opened successfully" -ForegroundColor Green
    
    # Wait for any boot messages
    Start-Sleep -Seconds 3
    
    # Check for any data
    if ($port.BytesToRead -gt 0) {
        $bootData = $port.ReadExisting()
        Write-Host "Boot messages received ($($bootData.Length) chars):" -ForegroundColor Green
        Write-Host $bootData -ForegroundColor Gray
    } else {
        Write-Host "No boot messages received" -ForegroundColor Yellow
    }
    
    # Try to flush and send handshake
    Write-Host "`nAttempting CLI handshake..." -ForegroundColor Blue
    
    # Flush any remaining data
    if ($port.BytesToRead -gt 0) {
        $staleData = $port.ReadExisting()
        Write-Host "Flushed $($staleData.Length) bytes of stale data" -ForegroundColor Gray
    }
    
    # Send carriage return
    $port.Write([char]13)
    Write-Host "Sent carriage return" -ForegroundColor Gray
    
    Start-Sleep -Milliseconds 500
    
    # Check for response
    if ($port.BytesToRead -gt 0) {
        $response = $port.ReadExisting()
        Write-Host "Handshake response: '$response'" -ForegroundColor Green
        
        if ($response -match '>') {
            Write-Host "CLI prompt detected!" -ForegroundColor Green
            
            # Try help command
            Write-Host "`nTesting help command..." -ForegroundColor Blue
            $port.WriteLine("help")
            Start-Sleep -Milliseconds 2000
            
            if ($port.BytesToRead -gt 0) {
                $helpResponse = $port.ReadExisting()
                Write-Host "Help response:" -ForegroundColor Green
                Write-Host $helpResponse -ForegroundColor Gray
            } else {
                Write-Host "No response to help command" -ForegroundColor Red
            }
        } else {
            Write-Host "No CLI prompt found in response" -ForegroundColor Yellow
        }
    } else {
        Write-Host "No response to handshake" -ForegroundColor Red
        Write-Host "Hardware may be frozen - requires reflash" -ForegroundColor Red
    }
    
} catch {
    Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    if ($port -and $port.IsOpen) {
        $port.Close()
        Write-Host "Serial port closed" -ForegroundColor Blue
    }
}

Write-Host "Basic serial debug test completed" -ForegroundColor Blue