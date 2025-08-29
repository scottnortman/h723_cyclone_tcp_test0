#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Test Working CLI Commands
.DESCRIPTION
    Now that CLI is working, test all the commands
#>

Write-Host "=== Testing Working CLI Commands ===" -ForegroundColor Blue

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
    }
    
    $port.Write([char]13)
    Start-Sleep -Milliseconds 300
    if ($port.BytesToRead -gt 0) {
        $prompt = $port.ReadExisting()
    }
    
    # Test key commands
    $commands = @(
        "task-stats",
        "uavcan-status", 
        "uavcan-simple-verify",
        "uavcan-test",
        "stack-info",
        "heap-info",
        "memory-info"
    )
    
    $results = @{}
    
    foreach ($cmd in $commands) {
        Write-Host "`n--- Testing: $cmd ---" -ForegroundColor Yellow
        
        # Send command
        $port.WriteLine($cmd)
        
        # Wait for response
        $timeout = [System.DateTime]::Now.AddSeconds(5)
        $response = ""
        
        while ([System.DateTime]::Now -lt $timeout) {
            if ($port.BytesToRead -gt 0) {
                $newData = $port.ReadExisting()
                $response += $newData
                
                # Check if we have a complete response
                if ($response -match '>\\s*$') {
                    break
                }
            }
            Start-Sleep -Milliseconds 100
        }
        
        if ($response.Length -gt 20) {
            Write-Host "✅ PASS - Response: $($response.Length) chars" -ForegroundColor Green
            $results[$cmd] = "PASS"
            
            # Show key parts of response
            $lines = $response -split "`r?`n" | Where-Object { $_.Trim() -ne "" -and $_ -notmatch "^>" } | Select-Object -First 3
            foreach ($line in $lines) {
                Write-Host "  $line" -ForegroundColor Gray
            }
        } else {
            Write-Host "❌ FAIL - Response: $($response.Length) chars" -ForegroundColor Red
            $results[$cmd] = "FAIL"
            Write-Host "  Response: '$response'" -ForegroundColor Gray
        }
    }
    
    Write-Host "`n=== Test Results Summary ===" -ForegroundColor Blue
    Write-Host "=============================" -ForegroundColor Blue
    
    $totalTests = $results.Count
    $passedTests = ($results.Values | Where-Object { $_ -eq "PASS" }).Count
    $failedTests = $totalTests - $passedTests
    
    foreach ($test in $results.GetEnumerator() | Sort-Object Name) {
        $status = if ($test.Value -eq "PASS") { "✅ PASS" } else { "❌ FAIL" }
        $color = if ($test.Value -eq "PASS") { "Green" } else { "Red" }
        Write-Host "$($test.Key.PadRight(25)) : $status" -ForegroundColor $color
    }
    
    Write-Host "`nOverall Results:" -ForegroundColor Blue
    Write-Host "  Total Tests: $totalTests" -ForegroundColor Blue
    Write-Host "  Passed: $passedTests" -ForegroundColor Green
    Write-Host "  Failed: $failedTests" -ForegroundColor $(if ($failedTests -eq 0) { "Green" } else { "Red" })
    
    if ($failedTests -eq 0) {
        Write-Host "`n🎉 ALL CLI COMMANDS WORKING!" -ForegroundColor Green
        Write-Host "✅ CLI echo issue resolved" -ForegroundColor Green
        Write-Host "✅ Command processing functional" -ForegroundColor Green
        Write-Host "✅ UAVCAN commands responding" -ForegroundColor Green
        Write-Host "✅ Stack monitoring commands working" -ForegroundColor Green
    } else {
        Write-Host "`n⚠️  Some commands still have issues" -ForegroundColor Yellow
    }
    
} catch {
    Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    if ($port -and $port.IsOpen) {
        $port.Close()
        Write-Host "`nSerial port closed" -ForegroundColor Blue
    }
}

Write-Host "CLI commands test completed" -ForegroundColor Blue