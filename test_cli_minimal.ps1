#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Minimal CLI Test
.DESCRIPTION
    Tests the most basic CLI functionality to isolate the issue
#>

Write-Host "=== Minimal CLI Test ===" -ForegroundColor Blue

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
    
    # Wait for boot and flush
    Start-Sleep -Seconds 3
    if ($port.BytesToRead -gt 0) {
        $bootData = $port.ReadExisting()
        Write-Host "Boot data length: $($bootData.Length)" -ForegroundColor Gray
    }
    
    # Send handshake
    Write-Host "Sending handshake..." -ForegroundColor Blue
    $port.Write([char]13)
    Start-Sleep -Milliseconds 500
    
    if ($port.BytesToRead -gt 0) {
        $prompt = $port.ReadExisting()
        Write-Host "Prompt response: '$prompt'" -ForegroundColor Green
    }
    
    # Test just pressing enter (should give prompt)
    Write-Host "Testing empty command (just Enter)..." -ForegroundColor Blue
    $port.Write([char]13)
    Start-Sleep -Milliseconds 1000
    
    if ($port.BytesToRead -gt 0) {
        $response = $port.ReadExisting()
        Write-Host "Empty command response: '$response'" -ForegroundColor Yellow
    } else {
        Write-Host "No response to empty command" -ForegroundColor Red
    }
    
    # Test invalid command
    Write-Host "Testing invalid command..." -ForegroundColor Blue
    $port.WriteLine("invalid-command-test")
    Start-Sleep -Milliseconds 2000
    
    if ($port.BytesToRead -gt 0) {
        $response = $port.ReadExisting()
        Write-Host "Invalid command response:" -ForegroundColor Yellow
        Write-Host "'$response'" -ForegroundColor White
        
        if ($response -match "not recognised" -or $response -match "Command not found") {
            Write-Host "✅ CLI is processing commands (error message received)" -ForegroundColor Green
        } else {
            Write-Host "❌ CLI may not be processing commands properly" -ForegroundColor Red
        }
    } else {
        Write-Host "❌ No response to invalid command - CLI not processing" -ForegroundColor Red
    }
    
    # Test help command with very long timeout
    Write-Host "Testing help command with extended timeout..." -ForegroundColor Blue
    $port.WriteLine("help")
    
    $timeout = [System.DateTime]::Now.AddSeconds(10)
    $response = ""
    
    while ([System.DateTime]::Now -lt $timeout) {
        if ($port.BytesToRead -gt 0) {
            $newData = $port.ReadExisting()
            $response += $newData
            Write-Host "Received: '$newData'" -ForegroundColor Cyan
        }
        Start-Sleep -Milliseconds 200
    }
    
    if ($response.Length -gt 10) {
        Write-Host "✅ Help command worked - received $($response.Length) chars" -ForegroundColor Green
    } else {
        Write-Host "❌ Help command failed - only received $($response.Length) chars" -ForegroundColor Red
    }
    
} catch {
    Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    if ($port -and $port.IsOpen) {
        $port.Close()
        Write-Host "Serial port closed" -ForegroundColor Blue
    }
}

Write-Host "Minimal CLI test completed" -ForegroundColor Blue