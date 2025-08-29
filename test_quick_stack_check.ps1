#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Quick Stack Status Check
.DESCRIPTION
    Quick check of current stack status after optimization
#>

Write-Host "=== Quick Stack Status Check ===" -ForegroundColor Blue

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
    Write-Host "Serial port opened" -ForegroundColor Green
    
    # Handshake
    Start-Sleep -Seconds 2
    if ($port.BytesToRead -gt 0) { $port.ReadExisting() | Out-Null }
    $port.Write([char]13)
    Start-Sleep -Milliseconds 300
    if ($port.BytesToRead -gt 0) { $port.ReadExisting() | Out-Null }
    
    # Check heap
    Write-Host "`n--- Heap Status ---" -ForegroundColor Yellow
    $port.WriteLine("heap-info")
    Start-Sleep -Milliseconds 1500
    
    if ($port.BytesToRead -gt 0) {
        $heapResponse = $port.ReadExisting()
        Write-Host $heapResponse -ForegroundColor Gray
        
        if ($heapResponse -match "Total Heap Size:\s+(\d+) bytes") {
            $heapSize = [int]$matches[1]
            $heapKB = [math]::Round($heapSize/1024, 0)
            Write-Host "✅ Heap: $heapKB KB" -ForegroundColor Green
        }
    }
    
    # Check stacks
    Write-Host "`n--- Stack Status ---" -ForegroundColor Yellow
    $port.WriteLine("stack-info")
    Start-Sleep -Milliseconds 2000
    
    if ($port.BytesToRead -gt 0) {
        $stackResponse = $port.ReadExisting()
        Write-Host $stackResponse -ForegroundColor Gray
        
        # Count status types
        $stackLines = ($stackResponse -split "`n") | Where-Object { $_ -match "%" }
        $warningTasks = ($stackLines | Where-Object { $_ -match "WARNING" }).Count
        $criticalTasks = ($stackLines | Where-Object { $_ -match "CRITICAL" }).Count
        $okTasks = ($stackLines | Where-Object { $_ -match "OK" }).Count
        
        Write-Host "`n--- Summary ---" -ForegroundColor Blue
        Write-Host "OK: $okTasks tasks" -ForegroundColor Green
        Write-Host "WARNING: $warningTasks tasks" -ForegroundColor Yellow
        Write-Host "CRITICAL: $criticalTasks tasks" -ForegroundColor Red
        
        if ($criticalTasks -eq 0 -and $warningTasks -eq 0) {
            Write-Host "🎉 ALL TASKS OPTIMIZED!" -ForegroundColor Green
        } elseif ($criticalTasks -eq 0) {
            Write-Host "✅ No critical issues, some warnings remain" -ForegroundColor Yellow
        } else {
            Write-Host "❌ Critical issues need attention" -ForegroundColor Red
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

Write-Host "Quick check completed" -ForegroundColor Blue