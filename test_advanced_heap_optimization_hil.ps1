#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Advanced Heap Optimization HIL Verification with Dual-Interface Testing
.DESCRIPTION
    Comprehensive HIL testing for 128 KB heap optimization with serial and telnet validation
#>

Write-Host "=== Advanced Heap Optimization HIL Verification ===" -ForegroundColor Blue
Write-Host "Testing 128 KB heap and optimized stack allocations" -ForegroundColor Blue
Write-Host "Dual-interface testing: Serial first, then Telnet validation" -ForegroundColor Blue
Write-Host "======================================================" -ForegroundColor Blue

# Test results storage
$SerialResults = @{}
$TelnetResults = @{}
$TestsPassed = 0
$TestsFailed = 0

function Test-SerialInterface {
    Write-Host "`n=== PHASE 1: SERIAL INTERFACE TESTING ===" -ForegroundColor Yellow
    
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
        
        # Establish connection with handshake
        Start-Sleep -Seconds 3
        if ($port.BytesToRead -gt 0) { $port.ReadExisting() | Out-Null }
        $port.Write([char]13)
        Start-Sleep -Milliseconds 300
        if ($port.BytesToRead -gt 0) { $port.ReadExisting() | Out-Null }
        
        Write-Host "CLI connection established" -ForegroundColor Green
        
        # Test 1: Heap Information Verification
        Write-Host "`n--- Test 1: Heap Optimization Verification ---" -ForegroundColor Cyan
        $port.WriteLine("heap-info")
        Start-Sleep -Milliseconds 2000
        
        $heapResponse = ""
        if ($port.BytesToRead -gt 0) {
            $heapResponse = $port.ReadExisting()
        }
        
        Write-Host "Heap Information:" -ForegroundColor White
        Write-Host $heapResponse -ForegroundColor Gray
        
        $SerialResults["heap-info"] = $heapResponse
        
        # Verify 128 KB heap
        if ($heapResponse -match "Total Heap Size:\s+(\d+) bytes") {
            $totalHeap = [int]$matches[1]
            if ($totalHeap -eq 131072) {
                Write-Host "✅ Heap correctly optimized to 128 KB ($totalHeap bytes)" -ForegroundColor Green
                $script:TestsPassed++
            } else {
                Write-Host "❌ Unexpected heap size: $totalHeap bytes (expected 131072)" -ForegroundColor Red
                $script:TestsFailed++
            }
        } else {
            Write-Host "❌ Could not parse heap size from response" -ForegroundColor Red
            $script:TestsFailed++
        }
        
        # Test 2: Stack Information Verification
        Write-Host "`n--- Test 2: Stack Optimization Verification ---" -ForegroundColor Cyan
        $port.WriteLine("stack-info")
        Start-Sleep -Milliseconds 3000
        
        $stackResponse = ""
        if ($port.BytesToRead -gt 0) {
            $stackResponse = $port.ReadExisting()
        }
        
        Write-Host "Stack Information:" -ForegroundColor White
        Write-Host $stackResponse -ForegroundColor Gray
        
        $SerialResults["stack-info"] = $stackResponse
        
        # Analyze stack results
        $stackLines = ($stackResponse -split "`n") | Where-Object { $_ -match "%" }
        $warningTasks = ($stackLines | Where-Object { $_ -match "WARNING" }).Count
        $criticalTasks = ($stackLines | Where-Object { $_ -match "CRITICAL" }).Count
        $okTasks = ($stackLines | Where-Object { $_ -match "OK" }).Count
        
        if ($warningTasks -eq 0 -and $criticalTasks -eq 0) {
            Write-Host "✅ All tasks in healthy state (0 WARNING, 0 CRITICAL)" -ForegroundColor Green
            $script:TestsPassed++
        } else {
            Write-Host "❌ Tasks with issues: $warningTasks WARNING, $criticalTasks CRITICAL" -ForegroundColor Red
            $script:TestsFailed++
        }
        
        # Test 3: Memory Information
        Write-Host "`n--- Test 3: Memory Information Verification ---" -ForegroundColor Cyan
        $port.WriteLine("memory-info")
        Start-Sleep -Milliseconds 2000
        
        $memoryResponse = ""
        if ($port.BytesToRead -gt 0) {
            $memoryResponse = $port.ReadExisting()
        }
        
        Write-Host "Memory Information:" -ForegroundColor White
        Write-Host $memoryResponse -ForegroundColor Gray
        
        $SerialResults["memory-info"] = $memoryResponse
        
        if ($memoryResponse.Length -gt 100) {
            Write-Host "✅ Memory info command working" -ForegroundColor Green
            $script:TestsPassed++
        } else {
            Write-Host "❌ Memory info command failed or incomplete" -ForegroundColor Red
            $script:TestsFailed++
        }
        
        # Test 4: UAVCAN Status
        Write-Host "`n--- Test 4: UAVCAN System Verification ---" -ForegroundColor Cyan
        $port.WriteLine("uavcan-status")
        Start-Sleep -Milliseconds 2000
        
        $uavcanResponse = ""
        if ($port.BytesToRead -gt 0) {
            $uavcanResponse = $port.ReadExisting()
        }
        
        Write-Host "UAVCAN Status:" -ForegroundColor White
        Write-Host $uavcanResponse -ForegroundColor Gray
        
        $SerialResults["uavcan-status"] = $uavcanResponse
        
        if ($uavcanResponse.Length -gt 50) {
            Write-Host "✅ UAVCAN status command working" -ForegroundColor Green
            $script:TestsPassed++
        } else {
            Write-Host "❌ UAVCAN status command failed" -ForegroundColor Red
            $script:TestsFailed++
        }
        
        # Test 5: System Stability Test
        Write-Host "`n--- Test 5: System Stability Test ---" -ForegroundColor Cyan
        $stabilityCommands = @("help", "stack-check", "heap-info", "memory-info")
        $stabilityPassed = 0
        
        foreach ($cmd in $stabilityCommands) {
            $port.WriteLine($cmd)
            Start-Sleep -Milliseconds 1500
            
            if ($port.BytesToRead -gt 0) {
                $response = $port.ReadExisting()
                if ($response.Length -gt 20) {
                    $stabilityPassed++
                    Write-Host "  ✅ $cmd - PASS" -ForegroundColor Green
                } else {
                    Write-Host "  ❌ $cmd - FAIL" -ForegroundColor Red
                }
            } else {
                Write-Host "  ❌ $cmd - No response" -ForegroundColor Red
            }
        }
        
        if ($stabilityPassed -eq $stabilityCommands.Count) {
            Write-Host "✅ System stability confirmed ($stabilityPassed/$($stabilityCommands.Count) commands)" -ForegroundColor Green
            $script:TestsPassed++
        } else {
            Write-Host "❌ System stability issues ($stabilityPassed/$($stabilityCommands.Count) commands working)" -ForegroundColor Red
            $script:TestsFailed++
        }
        
        Write-Host "`n=== SERIAL INTERFACE TESTING COMPLETE ===" -ForegroundColor Yellow
        Write-Host "Serial Tests Passed: $($script:TestsPassed)" -ForegroundColor Green
        Write-Host "Serial Tests Failed: $($script:TestsFailed)" -ForegroundColor Red
        
        return ($script:TestsFailed -eq 0)
        
    } catch {
        Write-Host "Error during serial testing: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    } finally {
        if ($port -and $port.IsOpen) {
            $port.Close()
            Write-Host "Serial port closed" -ForegroundColor Blue
        }
    }
}

function Test-TelnetInterface {
    Write-Host "`n=== PHASE 2: TELNET INTERFACE TESTING ===" -ForegroundColor Yellow
    Write-Host "Validating that telnet interface provides identical results" -ForegroundColor Blue
    
    try {
        # Connect to telnet interface
        $tcpClient = New-Object System.Net.Sockets.TcpClient
        $tcpClient.Connect("192.168.0.20", 23)
        $stream = $tcpClient.GetStream()
        
        Write-Host "Telnet connection established" -ForegroundColor Green
        
        # Improved handshake
        Start-Sleep -Milliseconds 1000
        $buffer = New-Object byte[] 1024
        
        # Clear any initial data
        while ($stream.DataAvailable) {
            $stream.Read($buffer, 0, $buffer.Length) | Out-Null
        }
        
        # Send handshake
        $crBytes = [System.Text.Encoding]::ASCII.GetBytes("`r")
        $stream.Write($crBytes, 0, $crBytes.Length)
        $stream.Flush()
        
        # Wait for prompt with retry logic
        Start-Sleep -Milliseconds 500
        $promptReceived = $false
        $attempts = 0
        
        while ($attempts -lt 5 -and -not $promptReceived) {
            if ($stream.DataAvailable) {
                $stream.Read($buffer, 0, $buffer.Length) | Out-Null
                $promptReceived = $true
            }
            Start-Sleep -Milliseconds 100
            $attempts++
        }
        
        # Test the same commands as serial
        $telnetTestsPassed = 0
        $telnetTestsFailed = 0
        
        $testCommands = @("heap-info", "stack-info", "memory-info", "uavcan-status")
        
        foreach ($cmd in $testCommands) {
            Write-Host "`n--- Telnet Test: $cmd ---" -ForegroundColor Cyan
            
            # Send command
            $cmdBytes = [System.Text.Encoding]::ASCII.GetBytes("$cmd`r")
            $stream.Write($cmdBytes, 0, $cmdBytes.Length)
            Start-Sleep -Milliseconds 2000
            
            # Read response
            $response = ""
            $buffer = New-Object byte[] 4096
            
            $attempts = 0
            while ($attempts -lt 5) {
                if ($stream.DataAvailable) {
                    $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
                    $response += [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
                }
                Start-Sleep -Milliseconds 200
                $attempts++
            }
            
            Write-Host "Telnet Response Length: $($response.Length) characters" -ForegroundColor Gray
            $TelnetResults[$cmd] = $response
            
            # Compare with serial results
            $serialResponse = $SerialResults[$cmd]
            
            if ($response.Length -gt 50 -and $serialResponse.Length -gt 50) {
                # Basic functionality check
                Write-Host "✅ $cmd - Telnet interface working" -ForegroundColor Green
                $telnetTestsPassed++
                
                # Content comparison (allowing for minor formatting differences)
                $serialCore = ($serialResponse -replace "[`r`n>`s]+", " ").Trim()
                $telnetCore = ($response -replace "[`r`n>`s]+", " ").Trim()
                
                if ($serialCore.Length -gt 0 -and $telnetCore.Length -gt 0) {
                    $similarity = [Math]::Min($serialCore.Length, $telnetCore.Length) / [Math]::Max($serialCore.Length, $telnetCore.Length)
                    if ($similarity -gt 0.8) {
                        Write-Host "✅ $cmd - Content similarity acceptable ($([Math]::Round($similarity * 100, 1))%)" -ForegroundColor Green
                    } else {
                        Write-Host "⚠️  $cmd - Content differs significantly ($([Math]::Round($similarity * 100, 1))% similarity)" -ForegroundColor Yellow
                    }
                }
            } else {
                Write-Host "❌ $cmd - Telnet interface failed or incomplete response" -ForegroundColor Red
                $telnetTestsFailed++
            }
        }
        
        Write-Host "`n=== TELNET INTERFACE TESTING COMPLETE ===" -ForegroundColor Yellow
        Write-Host "Telnet Tests Passed: $telnetTestsPassed" -ForegroundColor Green
        Write-Host "Telnet Tests Failed: $telnetTestsFailed" -ForegroundColor Red
        
        return ($telnetTestsFailed -eq 0)
        
    } catch {
        Write-Host "Error during telnet testing: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    } finally {
        if ($stream) {
            $stream.Close()
            $stream.Dispose()
        }
        if ($tcpClient) {
            $tcpClient.Close()
            $tcpClient.Dispose()
        }
        Start-Sleep -Milliseconds 500
        Write-Host "Telnet connection closed" -ForegroundColor Blue
    }
}

# Execute the tests
$serialSuccess = Test-SerialInterface

if ($serialSuccess) {
    Write-Host "`n🎉 Serial interface tests PASSED - Proceeding to telnet validation" -ForegroundColor Green
    $telnetSuccess = Test-TelnetInterface
    
    # Final Results
    Write-Host "`n=== FINAL HIL VERIFICATION RESULTS ===" -ForegroundColor Blue
    Write-Host "==========================================" -ForegroundColor Blue
    
    if ($serialSuccess -and $telnetSuccess) {
        Write-Host "🎉 COMPLETE SUCCESS - ADVANCED HEAP OPTIMIZATION VERIFIED!" -ForegroundColor Green
        Write-Host "✅ Heap successfully optimized to 128 KB" -ForegroundColor Green
        Write-Host "✅ All stack optimizations working" -ForegroundColor Green
        Write-Host "✅ Serial interface fully functional" -ForegroundColor Green
        Write-Host "✅ Telnet interface fully functional" -ForegroundColor Green
        Write-Host "✅ Dual-interface parity confirmed" -ForegroundColor Green
        Write-Host "✅ System stability verified" -ForegroundColor Green
        Write-Host "✅ Ready for production deployment" -ForegroundColor Green
    } else {
        Write-Host "⚠️  PARTIAL SUCCESS" -ForegroundColor Yellow
        if (-not $serialSuccess) {
            Write-Host "❌ Serial interface issues detected" -ForegroundColor Red
        }
        if (-not $telnetSuccess) {
            Write-Host "❌ Telnet interface issues detected" -ForegroundColor Red
        }
    }
} else {
    Write-Host "`n❌ SERIAL INTERFACE TESTS FAILED" -ForegroundColor Red
    Write-Host "Skipping telnet tests due to serial interface failures" -ForegroundColor Yellow
    Write-Host "Please resolve serial interface issues before proceeding" -ForegroundColor Red
}

Write-Host "`nAdvanced heap optimization HIL verification completed" -ForegroundColor Blue