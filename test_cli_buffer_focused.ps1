# CLI Buffer Fix Test - Focused on Hardware
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$logFile = "CLI_Buffer_Test_Results_$timestamp.txt"

Write-Host "=== CLI BUFFER FIX TEST ===" -ForegroundColor Cyan
"=== CLI BUFFER FIX TEST ===" | Out-File $logFile
"Date: $(Get-Date)" | Out-File $logFile -Append
"Hardware: STM32H723 at 192.168.0.20:23" | Out-File $logFile -Append
"Buffer Fix: 128 → 512 bytes" | Out-File $logFile -Append
"" | Out-File $logFile -Append

$client = New-Object System.Net.Sockets.TcpClient
$testResults = @()

try {
    Write-Host "Connecting to hardware at 192.168.0.20:23..." -ForegroundColor Yellow
    $client.Connect("192.168.0.20", 23)
    $stream = $client.GetStream()
    $writer = New-Object System.IO.StreamWriter($stream)
    $reader = New-Object System.IO.StreamReader($stream)
    
    Write-Host "Connected! Waiting for system ready..." -ForegroundColor Green
    Start-Sleep -Seconds 3
    
    # Clear any existing buffer data
    while($stream.DataAvailable) {
        $reader.ReadLine() | Out-Null
    }
    
    "Connected successfully!" | Out-File $logFile -Append
    "" | Out-File $logFile -Append
    
    # Test commands specifically for buffer verification
    $bufferTests = @(
        @{name="CLI Buffer Test"; cmd="uavcan-test-buffer"; wait=5; minLength=400; marker="buffer fix is working"},
        @{name="UAVCAN HIL Test"; cmd="uavcan-test"; wait=6; minLength=250; marker="ALL BASIC TESTS PASSED"},
        @{name="Requirements Test"; cmd="uavcan-verify-requirements"; wait=8; minLength=500; marker="ALL REQUIREMENTS HAVE BASIC COMPLIANCE"},
        @{name="Status Command"; cmd="uavcan-status"; wait=3; minLength=100; marker="UAVCAN System Status"}
    )
    
    foreach ($test in $bufferTests) {
        Write-Host "`n=== Testing: $($test.name) ===" -ForegroundColor Yellow
        "=== $($test.name) ===" | Out-File $logFile -Append
        "Command: $($test.cmd)" | Out-File $logFile -Append
        "Expected Min Length: $($test.minLength) characters" | Out-File $logFile -Append
        "Expected Marker: $($test.marker)" | Out-File $logFile -Append
        
        # Send command
        Write-Host "  → Sending: $($test.cmd)" -ForegroundColor Gray
        $writer.WriteLine($test.cmd)
        $writer.Flush()
        
        # Wait for response
        Write-Host "  → Waiting $($test.wait) seconds for response..." -ForegroundColor Gray
        Start-Sleep -Seconds $test.wait
        
        # Collect response
        $response = ""
        $responseLines = 0
        
        while($stream.DataAvailable -and $responseLines -lt 50) {
            $line = $reader.ReadLine()
            if ($line) {
                $response += $line + "`r`n"
                $responseLines++
            }
        }
        
        # Analyze response
        $responseLength = $response.Length
        $hasMarker = $response -like "*$($test.marker)*"
        $meetsMinLength = $responseLength -ge $test.minLength
        $truncatedAt128 = $responseLength -eq 128 -and -not $hasMarker
        
        Write-Host "  ← Response Length: $responseLength characters" -ForegroundColor Gray
        Write-Host "  ← Lines Received: $responseLines" -ForegroundColor Gray
        Write-Host "  ← Has Expected Marker: $hasMarker" -ForegroundColor Gray
        Write-Host "  ← Meets Min Length: $meetsMinLength" -ForegroundColor Gray
        
        # Determine result
        if ($meetsMinLength -and $hasMarker -and -not $truncatedAt128) {
            Write-Host "  ✅ PASS: Complete output received" -ForegroundColor Green
            $result = "PASS"
        } else {
            Write-Host "  ❌ FAIL: Output appears truncated" -ForegroundColor Red
            $result = "FAIL"
            
            if ($truncatedAt128) {
                Write-Host "    - Truncated at 128 bytes (old buffer limit)" -ForegroundColor Red
            }
            if (-not $hasMarker) {
                Write-Host "    - Missing expected marker" -ForegroundColor Red
            }
            if (-not $meetsMinLength) {
                Write-Host "    - Response too short" -ForegroundColor Red
            }
        }
        
        # Log results
        "Response Length: $responseLength characters" | Out-File $logFile -Append
        "Has Expected Marker: $hasMarker" | Out-File $logFile -Append
        "Meets Min Length: $meetsMinLength" | Out-File $logFile -Append
        "Result: $result" | Out-File $logFile -Append
        "" | Out-File $logFile -Append
        
        # Store for summary
        $testResults += [PSCustomObject]@{
            Test = $test.name
            Command = $test.cmd
            Result = $result
            ResponseLength = $responseLength
            ExpectedMinLength = $test.minLength
            HasMarker = $hasMarker
            TruncatedAt128 = $truncatedAt128
        }
        
        # Show response preview
        if ($response.Length -gt 100) {
            $preview = $response.Substring(0, 100) + "..."
            Write-Host "  Preview: $preview" -ForegroundColor DarkGray
        } else {
            Write-Host "  Full Response: $response" -ForegroundColor DarkGray
        }
        
        "Response Preview:" | Out-File $logFile -Append
        $response | Out-File $logFile -Append
        "---" | Out-File $logFile -Append
        "" | Out-File $logFile -Append
    }
    
} catch {
    Write-Host "Connection Error: $($_.Exception.Message)" -ForegroundColor Red
    "Connection Error: $($_.Exception.Message)" | Out-File $logFile -Append
    
} finally {
    if ($client.Connected) {
        $client.Close()
    }
}

# Generate summary
Write-Host "`n=== CLI BUFFER TEST SUMMARY ===" -ForegroundColor Cyan
"=== CLI BUFFER TEST SUMMARY ===" | Out-File $logFile -Append

$totalTests = $testResults.Count
$passedTests = ($testResults | Where-Object { $_.Result -eq "PASS" }).Count
$failedTests = $totalTests - $passedTests

Write-Host "Total Tests: $totalTests" -ForegroundColor White
Write-Host "Passed: $passedTests" -ForegroundColor Green
Write-Host "Failed: $failedTests" -ForegroundColor $(if ($failedTests -eq 0) { "Green" } else { "Red" })

"Total Tests: $totalTests" | Out-File $logFile -Append
"Passed: $passedTests" | Out-File $logFile -Append
"Failed: $failedTests" | Out-File $logFile -Append
"" | Out-File $logFile -Append

if ($testResults.Count -gt 0) {
    Write-Host "`nDetailed Results:" -ForegroundColor White
    $testResults | Format-Table -AutoSize
    
    "Detailed Results:" | Out-File $logFile -Append
    $testResults | Format-Table -AutoSize | Out-File $logFile -Append
}

# Final assessment
if ($failedTests -eq 0) {
    Write-Host "`n🎉 CLI BUFFER FIX: SUCCESS" -ForegroundColor Green
    Write-Host "✅ All commands output complete text without truncation" -ForegroundColor Green
    Write-Host "✅ 512-byte buffer fix is working correctly" -ForegroundColor Green
    Write-Host "✅ No evidence of 128-byte truncation limit" -ForegroundColor Green
    
    "FINAL RESULT: SUCCESS" | Out-File $logFile -Append
    "All CLI commands now output complete text without truncation" | Out-File $logFile -Append
    "512-byte buffer fix is working correctly" | Out-File $logFile -Append
    
} else {
    Write-Host "`n❌ CLI BUFFER FIX: ISSUES DETECTED" -ForegroundColor Red
    Write-Host "❌ Some commands still show truncation" -ForegroundColor Red
    Write-Host "🔧 Buffer size may need further adjustment" -ForegroundColor Yellow
    
    "FINAL RESULT: ISSUES DETECTED" | Out-File $logFile -Append
    "Some commands still show truncation - further investigation needed" | Out-File $logFile -Append
}

Write-Host "`nTest results saved to: $logFile" -ForegroundColor Cyan
Write-Host "=== CLI BUFFER TEST COMPLETE ===" -ForegroundColor Cyan