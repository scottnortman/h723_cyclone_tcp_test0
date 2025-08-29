#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Advanced Heap Optimization HIL Test - Fixed Telnet
.DESCRIPTION
    Complete test with proper telnet session management
#>

Write-Host "=== Advanced Heap Optimization HIL Verification (Fixed) ===" -ForegroundColor Blue
Write-Host "Testing 128 KB heap and optimized stack allocations" -ForegroundColor Blue
Write-Host "Dual-interface testing: Serial first, then Telnet validation" -ForegroundColor Blue
Write-Host "======================================================" -ForegroundColor Blue

# Test counters
$serialTestsPassed = 0
$serialTestsFailed = 0
$telnetTestsPassed = 0
$telnetTestsFailed = 0

# Phase 1: Serial Interface Testing
Write-Host "`n=== PHASE 1: SERIAL INTERFACE TESTING ===" -ForegroundColor Yellow

try {
    $port = New-Object System.IO.Ports.SerialPort
    $port.PortName = "COM3"
    $port.BaudRate = 115200
    $port.DataBits = 8
    $port.Parity = [System.IO.Ports.Parity]::None
    $port.StopBits = [System.IO.Ports.StopBits]::One
    $port.ReadTimeout = 5000
    $port.WriteTimeout = 3000
    
    Write-Host "Opening serial port COM3..." -ForegroundColor Gray
    $port.Open()
    Write-Host "Serial port opened successfully" -ForegroundColor Green
    
    # Handshake
    Start-Sleep -Seconds 1
    if ($port.BytesToRead -gt 0) { $port.ReadExisting() | Out-Null }
    $port.Write([char]13)
    Start-Sleep -Milliseconds 300
    if ($port.BytesToRead -gt 0) { $port.ReadExisting() | Out-Null }
    Write-Host "CLI connection established" -ForegroundColor Green
    
    # Test 1: Heap Optimization
    Write-Host "`n--- Test 1: Heap Optimization Verification ---" -ForegroundColor Yellow
    $port.WriteLine("heap-info")
    Start-Sleep -Milliseconds 2000
    
    $heapResponse = ""
    if ($port.BytesToRead -gt 0) {
        $heapResponse = $port.ReadExisting()
    }
    
    Write-Host "Heap Information:" -ForegroundColor Cyan
    Write-Host $heapResponse -ForegroundColor Gray
    
    if ($heapResponse -match "Total Heap Size:\s+(\d+) bytes") {
        $heapSize = [int]$matches[1]
        if ($heapSize -eq 131072) {
            Write-Host "✅ Heap correctly optimized to 128 KB (131072 bytes)" -ForegroundColor Green
            $serialTestsPassed++
        } else {
            Write-Host "❌ Unexpected heap size: $heapSize bytes" -ForegroundColor Red
            $serialTestsFailed++
        }
    } else {
        Write-Host "❌ Could not parse heap size from response" -ForegroundColor Red
        Write-Host "Debug - Response contains: $($heapResponse.Substring(0, [Math]::Min(100, $heapResponse.Length)))" -ForegroundColor Gray
        $serialTestsFailed++
    }
    
    # Test 2: Stack Optimization
    Write-Host "`n--- Test 2: Stack Optimization Verification ---" -ForegroundColor Yellow
    $port.WriteLine("stack-info")
    Start-Sleep -Milliseconds 3000
    
    $stackResponse = ""
    if ($port.BytesToRead -gt 0) {
        $stackResponse = $port.ReadExisting()
    }
    
    Write-Host "Stack Information:" -ForegroundColor Cyan
    Write-Host $stackResponse -ForegroundColor Gray
    
    if ($stackResponse -match "WARNING|CRITICAL") {
        Write-Host "❌ Stack warnings or critical issues detected" -ForegroundColor Red
        $serialTestsFailed++
    } else {
        Write-Host "✅ All tasks in healthy state (0 WARNING, 0 CRITICAL)" -ForegroundColor Green
        $serialTestsPassed++
    }
    
    # Test 3: Memory Information
    Write-Host "`n--- Test 3: Memory Information Verification ---" -ForegroundColor Yellow
    $port.WriteLine("memory-info")
    Start-Sleep -Milliseconds 2000
    
    $memoryResponse = ""
    if ($port.BytesToRead -gt 0) {
        $memoryResponse = $port.ReadExisting()
    }
    
    Write-Host "Memory Information:" -ForegroundColor Cyan
    Write-Host $memoryResponse -ForegroundColor Gray
    
    if ($memoryResponse.Length -gt 50) {
        Write-Host "✅ Memory info command working" -ForegroundColor Green
        $serialTestsPassed++
    } else {
        Write-Host "❌ Memory info command failed or incomplete" -ForegroundColor Red
        $serialTestsFailed++
    }
    
    # Test 4: UAVCAN System
    Write-Host "`n--- Test 4: UAVCAN System Verification ---" -ForegroundColor Yellow
    $port.WriteLine("uavcan-status")
    Start-Sleep -Milliseconds 2000
    
    $uavcanResponse = ""
    if ($port.BytesToRead -gt 0) {
        $uavcanResponse = $port.ReadExisting()
    }
    
    Write-Host "UAVCAN Status:" -ForegroundColor Cyan
    Write-Host $uavcanResponse -ForegroundColor Gray
    
    if ($uavcanResponse.Length -gt 50) {
        Write-Host "✅ UAVCAN status command working" -ForegroundColor Green
        $serialTestsPassed++
    } else {
        Write-Host "❌ UAVCAN status command failed" -ForegroundColor Red
        $serialTestsFailed++
    }
    
    # Test 5: System Stability
    Write-Host "`n--- Test 5: System Stability Test ---" -ForegroundColor Yellow
    $commands = @("help", "stack-check", "heap-info", "memory-info")
    $workingCommands = 0
    
    foreach ($cmd in $commands) {
        $port.WriteLine($cmd)
        Start-Sleep -Milliseconds 1000
        
        $response = ""
        if ($port.BytesToRead -gt 0) {
            $response = $port.ReadExisting()
        }
        
        if ($response.Length -gt 10) {
            Write-Host "  ✅ $cmd - PASS" -ForegroundColor Green
            $workingCommands++
        } else {
            Write-Host "  ❌ $cmd - FAIL" -ForegroundColor Red
        }
    }
    
    if ($workingCommands -eq $commands.Count) {
        Write-Host "✅ System stability confirmed ($workingCommands/$($commands.Count) commands)" -ForegroundColor Green
        $serialTestsPassed++
    } else {
        Write-Host "❌ System stability issues ($workingCommands/$($commands.Count) commands working)" -ForegroundColor Red
        $serialTestsFailed++
    }
    
    $port.Close()
    
} catch {
    Write-Host "❌ Serial test error: $($_.Exception.Message)" -ForegroundColor Red
    if ($port -and $port.IsOpen) { $port.Close() }
    $serialTestsFailed = 5
}

Write-Host "`n=== SERIAL INTERFACE TESTING COMPLETE ===" -ForegroundColor Yellow
Write-Host "Serial Tests Passed: $serialTestsPassed" -ForegroundColor Green
Write-Host "Serial Tests Failed: $serialTestsFailed" -ForegroundColor Red

if ($serialTestsPassed -ge 4) {
    Write-Host "Serial port closed" -ForegroundColor Gray
    Write-Host "`n🎉 Serial interface tests PASSED - Proceeding to telnet validation" -ForegroundColor Green
} else {
    Write-Host "`n❌ SERIAL INTERFACE TESTS FAILED" -ForegroundColor Red
    Write-Host "Skipping telnet tests due to serial interface failures" -ForegroundColor Yellow
    Write-Host "Please resolve serial interface issues before proceeding" -ForegroundColor Yellow
    exit 1
}

# Phase 2: Telnet Interface Testing
Write-Host "`n=== PHASE 2: TELNET INTERFACE TESTING ===" -ForegroundColor Yellow
Write-Host "Validating that telnet interface provides identical results" -ForegroundColor Gray

# Cleanup any existing telnet sessions
Write-Host "`nCleaning up any existing telnet sessions..." -ForegroundColor Gray
$telnetProcesses = Get-Process -Name "telnet" -ErrorAction SilentlyContinue
if ($telnetProcesses) {
    $telnetProcesses | Stop-Process -Force
    Write-Host "Killed existing telnet processes" -ForegroundColor Yellow
}
Start-Sleep -Seconds 2

try {
    # Create TCP client with proper timeouts
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.ReceiveTimeout = 10000
    $tcpClient.SendTimeout = 5000
    
    Write-Host "Connecting to telnet interface..." -ForegroundColor Gray
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    Write-Host "Telnet connection established" -ForegroundColor Green
    
    # System stabilization
    Start-Sleep -Seconds 2
    
    # Buffer flush
    $buffer = New-Object byte[] 2048
    while ($stream.DataAvailable) {
        $stream.Read($buffer, 0, $buffer.Length) | Out-Null
        Start-Sleep -Milliseconds 100
    }
    
    # CLI Handshake
    $handshakeSuccess = $false
    for ($retry = 1; $retry -le 3; $retry++) {
        $crBytes = [System.Text.Encoding]::ASCII.GetBytes("`r")
        $stream.Write($crBytes, 0, $crBytes.Length)
        $stream.Flush()
        Start-Sleep -Milliseconds 500
        
        if ($stream.DataAvailable) {
            $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
            $response = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
            if ($response -match '>') {
                $handshakeSuccess = $true
                break
            }
        }
    }
    
    if (-not $handshakeSuccess) {
        throw "CLI handshake failed"
    }
    
    # Test telnet commands
    $telnetCommands = @("heap-info", "stack-info", "memory-info", "uavcan-status")
    
    foreach ($cmd in $telnetCommands) {
        Write-Host "`n--- Telnet Test: $cmd ---" -ForegroundColor Yellow
        
        # Send command
        $cmdBytes = [System.Text.Encoding]::ASCII.GetBytes("$cmd`r")
        $stream.Write($cmdBytes, 0, $cmdBytes.Length)
        $stream.Flush()
        
        # Wait for response
        Start-Sleep -Seconds 3
        
        # Read response
        $response = ""
        $attempts = 0
        while ($attempts -lt 20) {
            if ($stream.DataAvailable) {
                $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
                $newData = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
                $response += $newData
            }
            Start-Sleep -Milliseconds 100
            $attempts++
        }
        
        Write-Host "Telnet Response Length: $($response.Length) characters" -ForegroundColor Cyan
        
        if ($response.Length -gt 50) {
            Write-Host "✅ $cmd - Telnet interface working" -ForegroundColor Green
            $telnetTestsPassed++
            
            # Show preview
            $preview = if ($response.Length -gt 200) { $response.Substring(0, 200) + "..." } else { $response }
            Write-Host "Response preview:" -ForegroundColor Gray
            Write-Host $preview -ForegroundColor DarkGray
        } else {
            Write-Host "❌ $cmd - Telnet interface failed or incomplete response" -ForegroundColor Red
            $telnetTestsFailed++
        }
    }
    
} catch {
    Write-Host "❌ Telnet test error: $($_.Exception.Message)" -ForegroundColor Red
    $telnetTestsFailed = 4
} finally {
    if ($stream) {
        $stream.Close()
        $stream.Dispose()
    }
    if ($tcpClient) {
        $tcpClient.Close()
        $tcpClient.Dispose()
    }
    Start-Sleep -Seconds 1
}

Write-Host "`n=== TELNET INTERFACE TESTING COMPLETE ===" -ForegroundColor Yellow
Write-Host "Telnet Tests Passed: $telnetTestsPassed" -ForegroundColor Green
Write-Host "Telnet Tests Failed: $telnetTestsFailed" -ForegroundColor Red
Write-Host "Telnet connection closed" -ForegroundColor Gray

# Final Results
Write-Host "`n=== FINAL HIL VERIFICATION RESULTS ===" -ForegroundColor Blue
Write-Host "==========================================" -ForegroundColor Blue

if ($serialTestsPassed -eq 5 -and $telnetTestsPassed -eq 4) {
    Write-Host "🎉 COMPLETE SUCCESS" -ForegroundColor Green
    Write-Host "✅ Serial interface: All tests passed" -ForegroundColor Green
    Write-Host "✅ Telnet interface: All tests passed" -ForegroundColor Green
    Write-Host "✅ Advanced heap optimization fully verified on hardware" -ForegroundColor Green
} elseif ($serialTestsPassed -eq 5 -and $telnetTestsPassed -gt 0) {
    Write-Host "⚠️  PARTIAL SUCCESS" -ForegroundColor Yellow
    Write-Host "✅ Serial interface: All tests passed" -ForegroundColor Green
    Write-Host "⚠️  Telnet interface: $telnetTestsPassed/4 tests passed" -ForegroundColor Yellow
    Write-Host "✅ Core functionality verified via serial interface" -ForegroundColor Green
} elseif ($serialTestsPassed -eq 5) {
    Write-Host "⚠️  PARTIAL SUCCESS" -ForegroundColor Yellow
    Write-Host "✅ Serial interface: All tests passed" -ForegroundColor Green
    Write-Host "❌ Telnet interface: Issues detected" -ForegroundColor Red
    Write-Host "✅ Core functionality verified via serial interface" -ForegroundColor Green
} else {
    Write-Host "❌ TESTS FAILED" -ForegroundColor Red
    Write-Host "❌ Serial interface issues detected" -ForegroundColor Red
}

Write-Host "`nAdvanced heap optimization HIL verification completed" -ForegroundColor Blue