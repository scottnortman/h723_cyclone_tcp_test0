#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Test Stack Info Fix
.DESCRIPTION
    Tests the fixed stack-info command to verify it shows task data
#>

Write-Host "=== Testing Fixed Stack Info Command ===" -ForegroundColor Blue

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
    
    # Wait for boot and establish connection
    Start-Sleep -Seconds 3
    if ($port.BytesToRead -gt 0) {
        $bootData = $port.ReadExisting()
        Write-Host "Boot data flushed: $($bootData.Length) bytes" -ForegroundColor Gray
    }
    
    $port.Write([char]13)
    Start-Sleep -Milliseconds 300
    if ($port.BytesToRead -gt 0) {
        $prompt = $port.ReadExisting()
        Write-Host "CLI prompt established" -ForegroundColor Green
    }
    
    # Test stack-info command
    Write-Host "`nTesting stack-info command..." -ForegroundColor Yellow
    $port.WriteLine("stack-info")
    
    # Wait for response with longer timeout
    $timeout = [System.DateTime]::Now.AddSeconds(8)
    $response = ""
    
    while ([System.DateTime]::Now -lt $timeout) {
        if ($port.BytesToRead -gt 0) {
            $newData = $port.ReadExisting()
            $response += $newData
            Write-Host "Received chunk: $($newData.Length) chars" -ForegroundColor Cyan
            
            # Check if we have a complete response
            if ($response -match '>\\s*$') {
                break
            }
        }
        Start-Sleep -Milliseconds 200
    }
    
    Write-Host "`nStack Info Response ($($response.Length) chars):" -ForegroundColor Green
    Write-Host "================================================" -ForegroundColor Green
    Write-Host $response -ForegroundColor White
    Write-Host "================================================" -ForegroundColor Green
    
    # Analyze the response
    $lines = $response -split "`r?`n" | Where-Object { $_.Trim() -ne "" }
    $taskLines = $lines | Where-Object { $_ -match "\\s+\\d+\\s+\\d+\\s+\\d+\\s+\\d+%" }
    
    Write-Host "`nAnalysis:" -ForegroundColor Blue
    Write-Host "  Total response lines: $($lines.Count)" -ForegroundColor Gray
    Write-Host "  Task data lines found: $($taskLines.Count)" -ForegroundColor Gray
    
    if ($taskLines.Count -gt 0) {
        Write-Host "  ✅ SUCCESS: Task data is now showing!" -ForegroundColor Green
        Write-Host "  Task details found:" -ForegroundColor Green
        foreach ($taskLine in $taskLines) {
            Write-Host "    $taskLine" -ForegroundColor Cyan
        }
    } else {
        Write-Host "  ❌ ISSUE: No task data lines found" -ForegroundColor Red
        Write-Host "  Response may still have issues" -ForegroundColor Red
    }
    
    # Test other stack commands
    Write-Host "`nTesting other stack commands..." -ForegroundColor Yellow
    
    $stackCommands = @("stack-check", "memory-info")
    
    foreach ($cmd in $stackCommands) {
        Write-Host "`n--- Testing: $cmd ---" -ForegroundColor Cyan
        $port.WriteLine($cmd)
        
        Start-Sleep -Milliseconds 2000
        
        if ($port.BytesToRead -gt 0) {
            $cmdResponse = $port.ReadExisting()
            Write-Host "Response length: $($cmdResponse.Length) chars" -ForegroundColor Green
            
            # Show first few lines
            $cmdLines = $cmdResponse -split "`r?`n" | Where-Object { $_.Trim() -ne "" } | Select-Object -First 5
            foreach ($line in $cmdLines) {
                Write-Host "  $line" -ForegroundColor Gray
            }
        } else {
            Write-Host "No response" -ForegroundColor Red
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

Write-Host "Stack info fix test completed" -ForegroundColor Blue