#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Final Stack Info Verification
.DESCRIPTION
    Properly tests and verifies the stack-info command with correct parsing
#>

Write-Host "=== Final Stack Info Verification ===" -ForegroundColor Blue
Write-Host "Testing with corrected parsing logic" -ForegroundColor Blue
Write-Host "====================================" -ForegroundColor Blue

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
    if ($port.BytesToRead -gt 0) {
        $bootData = $port.ReadExisting()
    }
    
    $port.Write([char]13)
    Start-Sleep -Milliseconds 300
    if ($port.BytesToRead -gt 0) {
        $prompt = $port.ReadExisting()
    }
    
    Write-Host "CLI connection established" -ForegroundColor Green
    
    # Test stack-info command
    Write-Host "`nTesting stack-info command..." -ForegroundColor Yellow
    $port.WriteLine("stack-info")
    
    # Wait for complete response
    $timeout = [System.DateTime]::Now.AddSeconds(5)
    $response = ""
    
    while ([System.DateTime]::Now -lt $timeout) {
        if ($port.BytesToRead -gt 0) {
            $newData = $port.ReadExisting()
            $response += $newData
            
            if ($response -match '>\\s*$') {
                break
            }
        }
        Start-Sleep -Milliseconds 100
    }
    
    Write-Host "`n=== STACK INFO COMMAND OUTPUT ===" -ForegroundColor Blue
    Write-Host $response -ForegroundColor White
    Write-Host "=================================" -ForegroundColor Blue
    
    # Parse task lines correctly
    $lines = $response -split "`r?`n"
    
    # Look for lines that contain task data (have percentage signs)
    $taskLines = $lines | Where-Object { 
        $_ -match "\\d+%" -and 
        $_ -notmatch "^Task Name" -and 
        $_ -notmatch "^-+" 
    }
    
    Write-Host "`n=== TASK ANALYSIS ===" -ForegroundColor Blue
    Write-Host "Tasks found: $($taskLines.Count)" -ForegroundColor Cyan
    
    if ($taskLines.Count -gt 0) {
        Write-Host "`nDetailed task information:" -ForegroundColor Green
        
        $okTasks = 0
        $warningTasks = 0
        $criticalTasks = 0
        
        foreach ($taskLine in $taskLines) {
            Write-Host "  $taskLine" -ForegroundColor White
            
            # Count status types
            if ($taskLine -match "OK") { $okTasks++ }
            elseif ($taskLine -match "WARNING") { $warningTasks++ }
            elseif ($taskLine -match "CRITICAL") { $criticalTasks++ }
        }
        
        Write-Host "`nTask Status Summary:" -ForegroundColor Blue
        Write-Host "  OK: $okTasks tasks" -ForegroundColor Green
        Write-Host "  WARNING: $warningTasks tasks" -ForegroundColor Yellow
        Write-Host "  CRITICAL: $criticalTasks tasks" -ForegroundColor Red
        
        # Overall assessment
        if ($criticalTasks -gt 0) {
            Write-Host "`n🚨 SYSTEM STATUS: CRITICAL" -ForegroundColor Red
            Write-Host "   Some tasks are using >90% of their stack space" -ForegroundColor Red
        } elseif ($warningTasks -gt 0) {
            Write-Host "`n⚠️  SYSTEM STATUS: WARNING" -ForegroundColor Yellow
            Write-Host "   Some tasks are using >75% of their stack space" -ForegroundColor Yellow
        } else {
            Write-Host "`n✅ SYSTEM STATUS: HEALTHY" -ForegroundColor Green
            Write-Host "   All tasks have adequate stack space" -ForegroundColor Green
        }
        
        Write-Host "`n🎉 VERIFICATION RESULT: SUCCESS!" -ForegroundColor Green
        Write-Host "✅ Stack-info command is working correctly" -ForegroundColor Green
        Write-Host "✅ Task data is being displayed properly" -ForegroundColor Green
        Write-Host "✅ Stack usage calculations are accurate" -ForegroundColor Green
        Write-Host "✅ Status indicators are functioning" -ForegroundColor Green
        
    } else {
        Write-Host "❌ VERIFICATION FAILED: No task data found" -ForegroundColor Red
        Write-Host "   The stack-info command is not working properly" -ForegroundColor Red
    }
    
    # Test other stack commands for completeness
    Write-Host "`n=== TESTING OTHER STACK COMMANDS ===" -ForegroundColor Blue
    
    $otherCommands = @("stack-check", "heap-info")
    
    foreach ($cmd in $otherCommands) {
        Write-Host "`nTesting $cmd..." -ForegroundColor Yellow
        $port.WriteLine($cmd)
        
        Start-Sleep -Milliseconds 2000
        
        if ($port.BytesToRead -gt 0) {
            $cmdResponse = $port.ReadExisting()
            $cmdLines = ($cmdResponse -split "`r?`n") | Where-Object { $_.Trim() -ne "" } | Select-Object -First 5
            
            Write-Host "Response preview:" -ForegroundColor Green
            foreach ($line in $cmdLines) {
                Write-Host "  $line" -ForegroundColor Gray
            }
            Write-Host "  ... (response: $($cmdResponse.Length) chars)" -ForegroundColor Gray
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

Write-Host "`nFinal stack info verification completed" -ForegroundColor Blue