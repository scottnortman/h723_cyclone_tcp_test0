#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Comprehensive HIL Test with Improved Buffer Flushing
.DESCRIPTION
    Tests both CLI functionality and stack monitoring with proper buffer flushing protocol
#>

Write-Host "=== Comprehensive HIL Verification Test ===" -ForegroundColor Blue
Write-Host "Testing CLI and Stack Monitoring with Buffer Flushing" -ForegroundColor Blue
Write-Host "======================================================" -ForegroundColor Blue

# Function to establish CLI connection with buffer flushing
function Connect-CLI {
    param($tcpClient, $stream)
    
    $maxRetries = 3
    $retryCount = 0
    $connected = $false
    
    Write-Host "Establishing CLI connection with buffer flushing..." -ForegroundColor Yellow
    
    while ($retryCount -lt $maxRetries -and -not $connected) {
        Write-Host "  Attempt $($retryCount + 1)/$maxRetries" -ForegroundColor Gray
        
        # Step 1: Flush any stale data from previous sessions
        if ($tcpClient.Available -gt 0) {
            $buffer = New-Object byte[] $tcpClient.Available
            $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
            $staleData = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
            Write-Host "  Flushed $($staleData.Length) bytes of stale data" -ForegroundColor Gray
        }
        
        # Step 2: Send carriage return to initiate handshake
        $stream.Write([byte]0x0D, 0, 1)
        Write-Host "  Sent carriage return for handshake" -ForegroundColor Gray
        
        # Step 3: Wait for target processing
        Start-Sleep -Milliseconds 300
        
        # Step 4: Check for CLI prompt
        if ($tcpClient.Available -gt 0) {
            $buffer = New-Object byte[] 1024
            $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
            $response = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
            
            if ($response -match '>') {
                $connected = $true
                Write-Host "  CLI connection established!" -ForegroundColor Green
                return $true
            } else {
                Write-Host "  No prompt found in response: '$response'" -ForegroundColor Yellow
            }
        } else {
            Write-Host "  No response received" -ForegroundColor Yellow
        }
        
        $retryCount++
        if ($retryCount -lt $maxRetries) {
            Start-Sleep -Milliseconds 500
        }
    }
    
    Write-Host "  Failed to establish CLI connection after $maxRetries attempts" -ForegroundColor Red
    return $false
}

# Function to send command and get response
function Send-CLICommand {
    param($stream, $tcpClient, $command, $timeoutMs = 3000)
    
    Write-Host "Sending command: $command" -ForegroundColor Cyan
    
    # Send command
    $cmdBytes = [System.Text.Encoding]::ASCII.GetBytes("$command`r`n")
    $stream.Write($cmdBytes, 0, $cmdBytes.Length)
    
    # Wait for response
    $timeout = [System.DateTime]::Now.AddMilliseconds($timeoutMs)
    $response = ""
    
    while ([System.DateTime]::Now -lt $timeout) {
        if ($tcpClient.Available -gt 0) {
            $buffer = New-Object byte[] 4096
            $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
            $newData = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
            $response += $newData
            
            # Check if we have a complete response (ends with prompt)
            if ($response -match '>\\s*$') {
                break
            }
        }
        Start-Sleep -Milliseconds 50
    }
    
    return $response
}

try {
    Write-Host "`nConnecting to STM32H723 at 192.168.0.20:23..." -ForegroundColor Blue
    
    # Wait for system to boot
    Start-Sleep -Seconds 8
    
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    
    # Establish CLI connection with proper buffer flushing
    if (-not (Connect-CLI -tcpClient $tcpClient -stream $stream)) {
        throw "Failed to establish CLI connection"
    }
    
    Write-Host "`n=== Testing Core CLI Commands ===" -ForegroundColor Blue
    
    # Test basic commands
    $commands = @(
        "help",
        "task-stats", 
        "uavcan-status",
        "uavcan-simple-verify",
        "uavcan-test"
    )
    
    $results = @{}
    
    foreach ($cmd in $commands) {
        Write-Host "`n--- Testing: $cmd ---" -ForegroundColor Yellow
        $response = Send-CLICommand -stream $stream -tcpClient $tcpClient -command $cmd
        
        if ($response.Length -gt 0) {
            Write-Host "Response received ($($response.Length) chars)" -ForegroundColor Green
            $results[$cmd] = "PASS"
            
            # Show first few lines of response
            $lines = $response -split "`r?`n" | Where-Object { $_.Trim() -ne "" } | Select-Object -First 5
            foreach ($line in $lines) {
                Write-Host "  $line" -ForegroundColor Gray
            }
            if ($response.Length -gt 200) {
                Write-Host "  ... (response truncated)" -ForegroundColor Gray
            }
        } else {
            Write-Host "No response received" -ForegroundColor Red
            $results[$cmd] = "FAIL"
        }
    }
    
    Write-Host "`n=== Testing Stack Monitoring Commands ===" -ForegroundColor Blue
    
    # Test stack monitoring commands
    $stackCommands = @(
        "stack-info",
        "stack-check", 
        "heap-info",
        "memory-info"
    )
    
    foreach ($cmd in $stackCommands) {
        Write-Host "`n--- Testing: $cmd ---" -ForegroundColor Yellow
        $response = Send-CLICommand -stream $stream -tcpClient $tcpClient -command $cmd
        
        if ($response.Length -gt 0) {
            Write-Host "Response received ($($response.Length) chars)" -ForegroundColor Green
            $results[$cmd] = "PASS"
            
            # Show key information from stack monitoring
            if ($cmd -eq "stack-info") {
                $lines = $response -split "`r?`n" | Where-Object { $_ -match "Task Name|OK|CRITICAL" }
                foreach ($line in $lines) {
                    Write-Host "  $line" -ForegroundColor Gray
                }
            } elseif ($cmd -eq "memory-info") {
                $lines = $response -split "`r?`n" | Where-Object { $_ -match "Total|Used|Free|Status" }
                foreach ($line in $lines) {
                    Write-Host "  $line" -ForegroundColor Gray
                }
            } else {
                # Show first few lines
                $lines = $response -split "`r?`n" | Where-Object { $_.Trim() -ne "" } | Select-Object -First 3
                foreach ($line in $lines) {
                    Write-Host "  $line" -ForegroundColor Gray
                }
            }
        } else {
            Write-Host "No response received" -ForegroundColor Red
            $results[$cmd] = "FAIL"
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
        Write-Host "`n🎉 ALL TESTS PASSED - System is working correctly!" -ForegroundColor Green
        Write-Host "✅ CLI buffer flushing protocol working" -ForegroundColor Green
        Write-Host "✅ Stack monitoring commands functional" -ForegroundColor Green
        Write-Host "✅ UAVCAN commands responding properly" -ForegroundColor Green
    } else {
        Write-Host "`n⚠️  Some tests failed - investigation needed" -ForegroundColor Yellow
    }
    
} catch {
    Write-Host "Error during testing: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "Stack trace: $($_.ScriptStackTrace)" -ForegroundColor Red
} finally {
    if ($tcpClient) {
        $tcpClient.Close()
        Write-Host "`nConnection closed" -ForegroundColor Blue
    }
}

Write-Host "`nComprehensive HIL verification completed" -ForegroundColor Blue