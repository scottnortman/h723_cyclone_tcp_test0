#!/usr/bin/env pwsh
<#
.SYNOPSIS
    STM32H723ZG Memory Capacity Analysis
.DESCRIPTION
    Analyzes current memory usage and calculates available stack capacity
#>

Write-Host "=== STM32H723ZG Memory Capacity Analysis ===" -ForegroundColor Blue
Write-Host "Analyzing current usage and available stack capacity" -ForegroundColor Blue
Write-Host "====================================================" -ForegroundColor Blue

try {
    Write-Host "`nOpening serial port COM3..." -ForegroundColor Blue
    
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
    
    # Establish connection
    Start-Sleep -Seconds 3
    if ($port.BytesToRead -gt 0) { $port.ReadExisting() | Out-Null }
    $port.Write([char]13)
    Start-Sleep -Milliseconds 300
    if ($port.BytesToRead -gt 0) { $port.ReadExisting() | Out-Null }
    
    Write-Host "CLI connection established" -ForegroundColor Green
    
    # Get memory information
    Write-Host "`n=== GATHERING MEMORY DATA ===" -ForegroundColor Yellow
    
    # Get comprehensive memory info
    Write-Host "`nGetting memory-info..." -ForegroundColor Cyan
    $port.WriteLine("memory-info")
    Start-Sleep -Milliseconds 2000
    
    $memoryResponse = ""
    if ($port.BytesToRead -gt 0) {
        $memoryResponse = $port.ReadExisting()
    }
    
    # Get stack info
    Write-Host "Getting stack-info..." -ForegroundColor Cyan
    $port.WriteLine("stack-info")
    Start-Sleep -Milliseconds 2000
    
    $stackResponse = ""
    if ($port.BytesToRead -gt 0) {
        $stackResponse = $port.ReadExisting()
    }
    
    # Get heap info
    Write-Host "Getting heap-info..." -ForegroundColor Cyan
    $port.WriteLine("heap-info")
    Start-Sleep -Milliseconds 2000
    
    $heapResponse = ""
    if ($port.BytesToRead -gt 0) {
        $heapResponse = $port.ReadExisting()
    }
    
    Write-Host "`n=== STM32H723ZG SPECIFICATIONS ===" -ForegroundColor Blue
    Write-Host "MCU: STM32H723ZGT6" -ForegroundColor White
    Write-Host "Total Flash: 1,024 KB (1 MB)" -ForegroundColor White
    Write-Host "Total RAM: 564 KB" -ForegroundColor White
    Write-Host "  - DTCM RAM: 128 KB (fastest)" -ForegroundColor Gray
    Write-Host "  - AXI SRAM: 512 KB" -ForegroundColor Gray
    Write-Host "  - SRAM1: 128 KB" -ForegroundColor Gray
    Write-Host "  - SRAM2: 128 KB" -ForegroundColor Gray
    Write-Host "  - SRAM3: 32 KB" -ForegroundColor Gray
    Write-Host "  - SRAM4: 64 KB" -ForegroundColor Gray
    Write-Host "  - Backup SRAM: 4 KB" -ForegroundColor Gray
    
    Write-Host "`n=== CURRENT MEMORY USAGE ===" -ForegroundColor Blue
    
    # Parse heap information
    if ($heapResponse -match "Total Heap Size:\s+(\d+) bytes") {
        $totalHeap = [int]$matches[1]
        Write-Host "Heap Configuration:" -ForegroundColor Yellow
        Write-Host "  Total Heap Size: $totalHeap bytes ($([math]::Round($totalHeap/1024, 1)) KB)" -ForegroundColor White
    }
    
    if ($heapResponse -match "Currently Used:\s+(\d+) bytes") {
        $usedHeap = [int]$matches[1]
        $freeHeap = $totalHeap - $usedHeap
        $heapUsagePercent = [math]::Round(($usedHeap * 100) / $totalHeap, 1)
        Write-Host "  Used Heap: $usedHeap bytes ($([math]::Round($usedHeap/1024, 1)) KB) - $heapUsagePercent%" -ForegroundColor White
        Write-Host "  Free Heap: $freeHeap bytes ($([math]::Round($freeHeap/1024, 1)) KB)" -ForegroundColor Green
    }
    
    # Parse stack information
    Write-Host "`nCurrent Stack Allocation:" -ForegroundColor Yellow
    $stackLines = ($stackResponse -split "`n") | Where-Object { $_ -match "%" -and $_ -match "(CmdDualSerial|IDLE|RED|GRN|TCP/IP|SerialRx)" }
    
    $totalStackAllocated = 0
    $totalStackUsed = 0
    
    foreach ($line in $stackLines) {
        $parts = $line.Trim() -split '\s+'
        if ($parts.Count -ge 6) {
            $taskName = $parts[0]
            $stackSize = [int]$parts[1]
            $used = [int]$parts[2]
            $free = [int]$parts[3]
            $percentage = $parts[4]
            $status = $parts[5]
            
            $totalStackAllocated += $stackSize
            $totalStackUsed += $used
            
            Write-Host "  $taskName`: $stackSize bytes allocated, $used used ($percentage)" -ForegroundColor White
        }
    }
    
    Write-Host "`nStack Summary:" -ForegroundColor Yellow
    Write-Host "  Total Stack Allocated: $totalStackAllocated bytes ($([math]::Round($totalStackAllocated/1024, 1)) KB)" -ForegroundColor White
    Write-Host "  Total Stack Used: $totalStackUsed bytes ($([math]::Round($totalStackUsed/1024, 1)) KB)" -ForegroundColor White
    Write-Host "  Total Stack Free: $($totalStackAllocated - $totalStackUsed) bytes ($([math]::Round(($totalStackAllocated - $totalStackUsed)/1024, 1)) KB)" -ForegroundColor Green
    
    Write-Host "`n=== MEMORY CAPACITY ANALYSIS ===" -ForegroundColor Blue
    
    # Calculate total RAM usage
    $totalRAM = 564 * 1024  # 564 KB in bytes
    $currentRAMUsage = $totalHeap + $totalStackAllocated
    $availableRAM = $totalRAM - $currentRAMUsage
    
    Write-Host "Total RAM Analysis:" -ForegroundColor Yellow
    Write-Host "  Total Available RAM: $totalRAM bytes ($([math]::Round($totalRAM/1024, 1)) KB)" -ForegroundColor White
    Write-Host "  Current Usage:" -ForegroundColor White
    Write-Host "    Heap: $totalHeap bytes ($([math]::Round($totalHeap/1024, 1)) KB)" -ForegroundColor Gray
    Write-Host "    Stacks: $totalStackAllocated bytes ($([math]::Round($totalStackAllocated/1024, 1)) KB)" -ForegroundColor Gray
    Write-Host "    Other (BSS, Data, etc.): ~$([math]::Round(($totalRAM - $totalHeap - $totalStackAllocated - $availableRAM)/1024, 1)) KB" -ForegroundColor Gray
    Write-Host "  Available for Stack Expansion: $availableRAM bytes ($([math]::Round($availableRAM/1024, 1)) KB)" -ForegroundColor Green
    
    Write-Host "`n=== STACK EXPANSION RECOMMENDATIONS ===" -ForegroundColor Blue
    
    # Calculate safe stack increases
    $safeStackIncrease = [math]::Floor($availableRAM * 0.7)  # Use 70% of available RAM safely
    $conservativeIncrease = [math]::Floor($availableRAM * 0.5)  # Conservative 50%
    
    Write-Host "Stack Expansion Options:" -ForegroundColor Yellow
    Write-Host "  Conservative (50% of available): $conservativeIncrease bytes ($([math]::Round($conservativeIncrease/1024, 1)) KB)" -ForegroundColor Green
    Write-Host "  Recommended (70% of available): $safeStackIncrease bytes ($([math]::Round($safeStackIncrease/1024, 1)) KB)" -ForegroundColor Yellow
    Write-Host "  Maximum (90% of available): $([math]::Floor($availableRAM * 0.9)) bytes ($([math]::Round(($availableRAM * 0.9)/1024, 1)) KB)" -ForegroundColor Red
    
    Write-Host "`nSuggested Stack Size Increases:" -ForegroundColor Yellow
    
    # Current WARNING tasks that need more stack
    $warningTasks = @(
        @{Name="IDLE"; Current=512; Usage=79},
        @{Name="RED"; Current=1024; Usage=90},
        @{Name="GRN"; Current=1024; Usage=90},
        @{Name="SerialRx"; Current=2048; Usage=77}
    )
    
    $totalIncrease = 0
    foreach ($task in $warningTasks) {
        $recommended = 0
        if ($task.Usage -ge 90) {
            $recommended = [math]::Ceiling($task.Current * 1.5)  # 50% increase for critical
        } elseif ($task.Usage -ge 75) {
            $recommended = [math]::Ceiling($task.Current * 1.25)  # 25% increase for warning
        }
        
        $increase = $recommended - $task.Current
        $totalIncrease += $increase
        
        Write-Host "  $($task.Name): $($task.Current) → $recommended bytes (+$increase bytes)" -ForegroundColor White
    }
    
    Write-Host "`nTotal Recommended Increase: $totalIncrease bytes ($([math]::Round($totalIncrease/1024, 1)) KB)" -ForegroundColor Cyan
    
    if ($totalIncrease -le $conservativeIncrease) {
        Write-Host "✅ FEASIBLE: Recommended increases fit within conservative limits" -ForegroundColor Green
    } elseif ($totalIncrease -le $safeStackIncrease) {
        Write-Host "⚠️  ACCEPTABLE: Recommended increases fit within safe limits" -ForegroundColor Yellow
    } else {
        Write-Host "❌ EXCESSIVE: Recommended increases exceed safe limits" -ForegroundColor Red
        Write-Host "   Consider reducing heap size or optimizing code" -ForegroundColor Red
    }
    
    Write-Host "`n=== OPTIMIZATION OPPORTUNITIES ===" -ForegroundColor Blue
    Write-Host "Heap Optimization:" -ForegroundColor Yellow
    Write-Host "  Current heap usage: $heapUsagePercent% ($([math]::Round($usedHeap/1024, 1)) KB used of $([math]::Round($totalHeap/1024, 1)) KB)" -ForegroundColor White
    if ($heapUsagePercent -lt 80) {
        $heapReduction = [math]::Floor($freeHeap * 0.5)
        Write-Host "  Potential heap reduction: $heapReduction bytes ($([math]::Round($heapReduction/1024, 1)) KB)" -ForegroundColor Green
        Write-Host "  This would free up additional space for stacks" -ForegroundColor Green
    }
    
} catch {
    Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    if ($port -and $port.IsOpen) {
        $port.Close()
        Write-Host "`nSerial port closed" -ForegroundColor Blue
    }
}

Write-Host "`nMemory capacity analysis completed" -ForegroundColor Blue