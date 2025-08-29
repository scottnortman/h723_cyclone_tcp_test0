#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Verbose Stack Info Analysis Test
.DESCRIPTION
    Tests stack-info command with full output display and proper parsing
#>

Write-Host "=== Verbose Stack Info Analysis Test ===" -ForegroundColor Blue
Write-Host "Testing with full output display and proper parsing" -ForegroundColor Blue
Write-Host "===================================================" -ForegroundColor Blue

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
        Write-Host "Boot data flushed: $($bootData.Length) bytes" -ForegroundColor Gray
    }
    
    $port.Write([char]13)
    Start-Sleep -Milliseconds 300
    if ($port.BytesToRead -gt 0) {
        $prompt = $port.ReadExisting()
        Write-Host "CLI prompt established: '$prompt'" -ForegroundColor Green
    }
    
    # Test stack-info command with verbose output
    Write-Host "`n=== TESTING STACK-INFO COMMAND ===" -ForegroundColor Yellow
    Write-Host "Sending command: stack-info" -ForegroundColor Cyan
    
    $port.WriteLine("stack-info")
    
    # Wait for complete response
    $timeout = [System.DateTime]::Now.AddSeconds(8)
    $response = ""
    $chunks = @()
    
    Write-Host "`nReceiving response chunks:" -ForegroundColor Gray
    while ([System.DateTime]::Now -lt $timeout) {
        if ($port.BytesToRead -gt 0) {
            $newData = $port.ReadExisting()
            $response += $newData
            $chunks += $newData
            Write-Host "  Chunk $($chunks.Count): $($newData.Length) chars" -ForegroundColor Gray
            
            # Check if we have a complete response
            if ($response -match '>\\s*$') {
                Write-Host "  Complete response detected (found prompt)" -ForegroundColor Green
                break
            }
        }
        Start-Sleep -Milliseconds 200
    }
    
    Write-Host "`n=== FULL STACK-INFO RESPONSE ===" -ForegroundColor Blue
    Write-Host "Total response length: $($response.Length) characters" -ForegroundColor Blue
    Write-Host "Response chunks received: $($chunks.Count)" -ForegroundColor Blue
    Write-Host "Raw response:" -ForegroundColor Blue
    Write-Host "----------------------------------------" -ForegroundColor Magenta
    Write-Host $response -ForegroundColor White
    Write-Host "----------------------------------------" -ForegroundColor Magenta
    
    # Parse the response line by line
    Write-Host "`n=== DETAILED RESPONSE ANALYSIS ===" -ForegroundColor Blue
    $lines = $response -split "`r?`n"
    Write-Host "Total lines in response: $($lines.Count)" -ForegroundColor Blue
    
    Write-Host "`nAll lines with content:" -ForegroundColor Yellow
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if ($line.Trim() -ne "") {
            Write-Host "  Line $($i+1): '$line'" -ForegroundColor Gray
        }
    }
    
    # Look for task data lines with multiple parsing approaches
    Write-Host "`n=== TASK DATA PARSING ===" -ForegroundColor Blue
    
    # Method 1: Look for lines with task names and numbers
    $taskLines1 = $lines | Where-Object { 
        $_.Trim() -ne "" -and 
        $_ -notmatch "^Stack Usage Report" -and 
        $_ -notmatch "^Task Name" -and 
        $_ -notmatch "^-+" -and 
        $_ -notmatch "^Total Tasks" -and 
        $_ -notmatch "^>" -and
        $_ -match "\\w+.*\\d+.*\\d+.*\\d+" 
    }
    
    Write-Host "Method 1 - Lines with task name and numbers:" -ForegroundColor Yellow
    Write-Host "  Found $($taskLines1.Count) potential task lines" -ForegroundColor Cyan
    foreach ($taskLine in $taskLines1) {
        Write-Host "    '$taskLine'" -ForegroundColor White
    }
    
    # Method 2: Look for lines with percentage signs
    $taskLines2 = $lines | Where-Object { $_ -match "\\d+%" }
    
    Write-Host "`nMethod 2 - Lines with percentage signs:" -ForegroundColor Yellow
    Write-Host "  Found $($taskLines2.Count) lines with percentages" -ForegroundColor Cyan
    foreach ($taskLine in $taskLines2) {
        Write-Host "    '$taskLine'" -ForegroundColor White
    }
    
    # Method 3: Look for lines with specific task names we know exist
    $knownTasks = @("CmdDualSerial", "IDLE", "RED", "GRN", "TCP/IP", "SerialRx", "SerialTx")
    $taskLines3 = $lines | Where-Object { 
        $line = $_
        $knownTasks | ForEach-Object { if ($line -match $_) { return $true } }
    }
    
    Write-Host "`nMethod 3 - Lines with known task names:" -ForegroundColor Yellow
    Write-Host "  Found $($taskLines3.Count) lines with known task names" -ForegroundColor Cyan
    foreach ($taskLine in $taskLines3) {
        Write-Host "    '$taskLine'" -ForegroundColor White
    }
    
    # Method 4: Look for lines that match the expected format more precisely
    $taskLines4 = $lines | Where-Object { 
        $_ -match "^\\s*\\w+\\s+\\d+\\s+\\d+\\s+\\d+\\s+\\d+%\\s+(OK|WARNING|CRITICAL)" 
    }
    
    Write-Host "`nMethod 4 - Lines matching expected task format:" -ForegroundColor Yellow
    Write-Host "  Found $($taskLines4.Count) lines matching task format" -ForegroundColor Cyan
    foreach ($taskLine in $taskLines4) {
        Write-Host "    '$taskLine'" -ForegroundColor White
    }
    
    # Determine the best parsing method
    $bestTaskLines = $taskLines2  # Use percentage method as it's most reliable
    
    Write-Host "`n=== FINAL ANALYSIS ===" -ForegroundColor Blue
    if ($bestTaskLines.Count -gt 0) {
        Write-Host "SUCCESS: Stack-info command is working!" -ForegroundColor Green
        Write-Host "  Tasks found: $($bestTaskLines.Count)" -ForegroundColor Green
        Write-Host "  Task details:" -ForegroundColor Green
        
        foreach ($taskLine in $bestTaskLines) {
            # Parse task information
            if ($taskLine -match "(\\w+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)%\\s+(\\w+)") {
                $taskName = $matches[1]
                $stackSize = $matches[2]
                $used = $matches[3]
                $free = $matches[4]
                $percentage = $matches[5]
                $status = $matches[6]
                
                Write-Host "    Task: $taskName" -ForegroundColor Cyan
                Write-Host "      Stack Size: $stackSize bytes" -ForegroundColor Gray
                Write-Host "      Used: $used bytes" -ForegroundColor Gray
                Write-Host "      Free: $free bytes" -ForegroundColor Gray
                Write-Host "      Usage: $percentage%" -ForegroundColor Gray
                Write-Host "      Status: $status" -ForegroundColor $(if ($status -eq "OK") { "Green" } elseif ($status -eq "WARNING") { "Yellow" } else { "Red" })
            } else {
                Write-Host "    Raw: $taskLine" -ForegroundColor White
            }
        }
        
        # Check for warnings
        $warningTasks = $bestTaskLines | Where-Object { $_ -match "WARNING" }
        $criticalTasks = $bestTaskLines | Where-Object { $_ -match "CRITICAL" }
        
        if ($criticalTasks.Count -gt 0) {
            Write-Host "`n  ALERT: $($criticalTasks.Count) tasks in CRITICAL state!" -ForegroundColor Red
        } elseif ($warningTasks.Count -gt 0) {
            Write-Host "`n  WARNING: $($warningTasks.Count) tasks in WARNING state" -ForegroundColor Yellow
        } else {
            Write-Host "`n  All tasks in healthy state" -ForegroundColor Green
        }
        
    } else {
        Write-Host "ISSUE: No task data found in response" -ForegroundColor Red
        Write-Host "  This indicates the stack-info command may not be working properly" -ForegroundColor Red
        Write-Host "  Response length: $($response.Length) chars" -ForegroundColor Red
        Write-Host "  Check the raw response above for clues" -ForegroundColor Red
    }
    
} catch {
    Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "Stack trace: $($_.ScriptStackTrace)" -ForegroundColor Red
} finally {
    if ($port -and $port.IsOpen) {
        $port.Close()
        Write-Host "`nSerial port closed" -ForegroundColor Blue
    }
}

Write-Host "`nVerbose stack info analysis completed" -ForegroundColor Blue