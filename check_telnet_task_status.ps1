#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Check Telnet Task Status via Serial
.DESCRIPTION
    Use serial interface to check if telnet task is running
#>

Write-Host "=== Checking Telnet Task Status via Serial ===" -ForegroundColor Blue

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
    
    # Check task list
    Write-Host "`nChecking task list..." -ForegroundColor Yellow
    $port.WriteLine("stack-info")
    Start-Sleep -Milliseconds 3000
    
    $response = ""
    if ($port.BytesToRead -gt 0) {
        $response = $port.ReadExisting()
    }
    
    Write-Host "Task List:" -ForegroundColor Cyan
    Write-Host $response -ForegroundColor Gray
    
    # Check for telnet task
    if ($response -match "CmdDualTelnet") {
        Write-Host "✅ CmdDualTelnet task is running" -ForegroundColor Green
        
        # Extract telnet task info
        $lines = $response -split "`n"
        $telnetLine = $lines | Where-Object { $_ -match "CmdDualTelnet" }
        if ($telnetLine) {
            Write-Host "Telnet task details: $telnetLine" -ForegroundColor Cyan
        }
    } else {
        Write-Host "❌ CmdDualTelnet task not found" -ForegroundColor Red
    }
    
    # Check network status
    Write-Host "`nChecking network status..." -ForegroundColor Yellow
    $port.WriteLine("uavcan-status")
    Start-Sleep -Milliseconds 2000
    
    $netResponse = ""
    if ($port.BytesToRead -gt 0) {
        $netResponse = $port.ReadExisting()
    }
    
    Write-Host "Network Status:" -ForegroundColor Cyan
    Write-Host $netResponse -ForegroundColor Gray
    
    $port.Close()
    
} catch {
    Write-Host "❌ Error: $($_.Exception.Message)" -ForegroundColor Red
    if ($port -and $port.IsOpen) { $port.Close() }
}

Write-Host "`n=== Task Status Check Complete ===" -ForegroundColor Blue