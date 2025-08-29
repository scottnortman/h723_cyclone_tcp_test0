#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Simple Memory Analysis
.DESCRIPTION
    Analyzes STM32H723ZG memory capacity for additional stack allocation
#>

Write-Host "=== STM32H723ZG Memory Analysis ===" -ForegroundColor Blue

# STM32H723ZG Specifications
Write-Host "`nSTM32H723ZG Memory Specifications:" -ForegroundColor Yellow
Write-Host "  FLASH: 1024 KB" -ForegroundColor Gray
Write-Host "  Total RAM: 564 KB" -ForegroundColor Gray
Write-Host "    ITCMRAM: 64 KB" -ForegroundColor Gray
Write-Host "    DTCMRAM: 128 KB" -ForegroundColor Gray
Write-Host "    RAM_D1: 320 KB (main RAM)" -ForegroundColor Gray
Write-Host "    RAM_D2: 32 KB" -ForegroundColor Gray
Write-Host "    RAM_D3: 16 KB" -ForegroundColor Gray

# Current usage from build output and tests
Write-Host "`nCurrent Memory Usage:" -ForegroundColor Yellow
Write-Host "  Program data: 25 KB" -ForegroundColor Gray
Write-Host "  BSS data: 61 KB" -ForegroundColor Gray
Write-Host "  FreeRTOS heap: 32 KB" -ForegroundColor Gray
Write-Host "  Main stack: 2 KB" -ForegroundColor Gray
Write-Host "  Task stacks: ~10 KB" -ForegroundColor Gray
Write-Host "  Total used: ~130 KB" -ForegroundColor Cyan

# Available memory
$mainRAM = 320
$currentUsed = 130
$available = $mainRAM - $currentUsed

Write-Host "`nAvailable Memory:" -ForegroundColor Green
Write-Host "  In main RAM (RAM_D1): $available KB" -ForegroundColor Green
Write-Host "  In DTCMRAM: 128 KB (unused)" -ForegroundColor Green
Write-Host "  In RAM_D2: 32 KB (unused)" -ForegroundColor Green
Write-Host "  In RAM_D3: 16 KB (unused)" -ForegroundColor Green
Write-Host "  Total additional: $($available + 128 + 32 + 16) KB" -ForegroundColor Green

# Get current stack usage from hardware
try {
    Write-Host "`nGetting current stack usage..." -ForegroundColor Cyan
    
    $port = New-Object System.IO.Ports.SerialPort
    $port.PortName = "COM3"
    $port.BaudRate = 115200
    $port.DataBits = 8
    $port.Parity = [System.IO.Ports.Parity]::None
    $port.StopBits = [System.IO.Ports.StopBits]::One
    $port.ReadTimeout = 3000
    $port.WriteTimeout = 3000
    
    $port.Open()
    
    # Connect
    Start-Sleep -Seconds 2
    if ($port.BytesToRead -gt 0) { $port.ReadExisting() | Out-Null }
    $port.Write([char]13)
    Start-Sleep -Milliseconds 300
    if ($port.BytesToRead -gt 0) { $port.ReadExisting() | Out-Null }
    
    # Get stack info
    $port.WriteLine("stack-info")
    Start-Sleep -Milliseconds 2000
    
    if ($port.BytesToRead -gt 0) {
        $response = $port.ReadExisting()
        Write-Host "`nCurrent Task Stack Usage:" -ForegroundColor Yellow
        
        $lines = $response -split "`n"
        foreach ($line in $lines) {
            if ($line -match "%" -and ($line -match "CmdDualSerial|IDLE|RED|GRN|TCP/IP|SerialRx")) {
                Write-Host "  $($line.Trim())" -ForegroundColor White
            }
        }
    }
    
    $port.Close()
    
} catch {
    Write-Host "Could not get hardware data: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host "`nStack Increase Recommendations:" -ForegroundColor Blue
Write-Host "  For WARNING tasks (79-90% usage):" -ForegroundColor Yellow
Write-Host "    IDLE: 512 -> 1024 bytes (+512)" -ForegroundColor Gray
Write-Host "    RED: 1024 -> 1536 bytes (+512)" -ForegroundColor Gray
Write-Host "    GRN: 1024 -> 1536 bytes (+512)" -ForegroundColor Gray
Write-Host "    SerialRx: 2048 -> 2560 bytes (+512)" -ForegroundColor Gray
Write-Host "  Total increase: 2048 bytes (2 KB)" -ForegroundColor Cyan

Write-Host "`nMemory Capacity Summary:" -ForegroundColor Green
Write-Host "  Available in main RAM: ~190 KB" -ForegroundColor Green
Write-Host "  Available in other regions: 176 KB" -ForegroundColor Green
Write-Host "  Total additional capacity: ~366 KB" -ForegroundColor Green
Write-Host "  Recommended increase: 2 KB (easily accommodated)" -ForegroundColor Green

Write-Host "`nConclusion:" -ForegroundColor Blue
Write-Host "  You have PLENTY of memory available!" -ForegroundColor Green
Write-Host "  Safe to increase all WARNING task stacks" -ForegroundColor Green
Write-Host "  Could add many more tasks if needed" -ForegroundColor Green

Write-Host "`nMemory analysis completed" -ForegroundColor Blue