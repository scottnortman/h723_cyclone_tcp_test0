# Simple CLI Buffer Test
Write-Host "=== CLI BUFFER FIX TEST ===" -ForegroundColor Cyan
Write-Host "Testing CLI buffer fix on hardware" -ForegroundColor White
Write-Host ""

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
    
    # Clear buffer
    while($stream.DataAvailable) {
        $reader.ReadLine() | Out-Null
    }
    
    # Test the new buffer test command
    Write-Host "`nTesting: CLI Buffer Test Command" -ForegroundColor Yellow
    Write-Host "  → Sending: uavcan-test-buffer" -ForegroundColor Gray
    $writer.WriteLine("uavcan-test-buffer")
    $writer.Flush()
    
    Start-Sleep -Seconds 5
    
    $response = ""
    $responseLines = 0
    
    while($stream.DataAvailable -and $responseLines -lt 50) {
        $line = $reader.ReadLine()
        if ($line) {
            $response += $line + "`r`n"
            $responseLines++
        }
    }
    
    $responseLength = $response.Length
    $hasMarker = $response -like "*buffer fix is working*"
    $meetsMinLength = $responseLength -ge 400
    
    Write-Host "  ← Response Length: $responseLength characters" -ForegroundColor Gray
    Write-Host "  ← Lines Received: $responseLines" -ForegroundColor Gray
    Write-Host "  ← Has Success Marker: $hasMarker" -ForegroundColor Gray
    Write-Host "  ← Meets Min Length (400): $meetsMinLength" -ForegroundColor Gray
    
    if ($meetsMinLength -and $hasMarker) {
        Write-Host "  ✅ PASS: CLI buffer fix is working!" -ForegroundColor Green
        $bufferTestResult = "PASS"
    } else {
        Write-Host "  ❌ FAIL: CLI buffer may still be truncated" -ForegroundColor Red
        $bufferTestResult = "FAIL"
        
        if ($responseLength -eq 128) {
            Write-Host "    - Output truncated at 128 bytes (old limit)" -ForegroundColor Red
        }
    }
    
    # Show response preview
    if ($response.Length -gt 200) {
        $preview = $response.Substring(0, 100) + "`n...`n" + $response.Substring($response.Length - 100)
        Write-Host "  Response Preview:" -ForegroundColor DarkGray
        Write-Host $preview -ForegroundColor DarkGray
    } else {
        Write-Host "  Full Response:" -ForegroundColor DarkGray
        Write-Host $response -ForegroundColor DarkGray
    }
    
    # Test one more command for comparison
    Write-Host "`nTesting: UAVCAN HIL Test Command" -ForegroundColor Yellow
    Write-Host "  → Sending: uavcan-test" -ForegroundColor Gray
    $writer.WriteLine("uavcan-test")
    $writer.Flush()
    
    Start-Sleep -Seconds 6
    
    $response2 = ""
    $responseLines2 = 0
    
    while($stream.DataAvailable -and $responseLines2 -lt 50) {
        $line = $reader.ReadLine()
        if ($line) {
            $response2 += $line + "`r`n"
            $responseLines2++
        }
    }
    
    $responseLength2 = $response2.Length
    $hasMarker2 = $response2 -like "*ALL BASIC TESTS PASSED*"
    $meetsMinLength2 = $responseLength2 -ge 250
    
    Write-Host "  ← Response Length: $responseLength2 characters" -ForegroundColor Gray
    Write-Host "  ← Has Success Marker: $hasMarker2" -ForegroundColor Gray
    Write-Host "  ← Meets Min Length (250): $meetsMinLength2" -ForegroundColor Gray
    
    if ($meetsMinLength2 -and $hasMarker2) {
        Write-Host "  ✅ PASS: UAVCAN test output complete" -ForegroundColor Green
        $hilTestResult = "PASS"
    } else {
        Write-Host "  ❌ FAIL: UAVCAN test output may be truncated" -ForegroundColor Red
        $hilTestResult = "FAIL"
    }
    
} catch {
    Write-Host "Connection Error: $($_.Exception.Message)" -ForegroundColor Red
    $bufferTestResult = "ERROR"
    $hilTestResult = "ERROR"
    
} finally {
    if ($client.Connected) {
        $client.Close()
    }
}

# Summary
Write-Host "`n=== CLI BUFFER TEST SUMMARY ===" -ForegroundColor Cyan
Write-Host "Buffer Configuration:" -ForegroundColor White
Write-Host "  Old Buffer Size: 128 bytes" -ForegroundColor Gray
Write-Host "  New Buffer Size: 512 bytes" -ForegroundColor Green
Write-Host "  Improvement: 4x increase" -ForegroundColor Green
Write-Host ""

Write-Host "Test Results:" -ForegroundColor White
Write-Host "  CLI Buffer Test: $bufferTestResult" -ForegroundColor $(if ($bufferTestResult -eq "PASS") { "Green" } else { "Red" })
Write-Host "  UAVCAN HIL Test: $hilTestResult" -ForegroundColor $(if ($hilTestResult -eq "PASS") { "Green" } else { "Red" })
Write-Host ""

if ($bufferTestResult -eq "PASS" -and $hilTestResult -eq "PASS") {
    Write-Host "🎉 CLI BUFFER FIX: SUCCESS" -ForegroundColor Green
    Write-Host "✅ All commands output complete text without truncation" -ForegroundColor Green
    Write-Host "✅ 512-byte buffer fix is working correctly" -ForegroundColor Green
} else {
    Write-Host "❌ CLI BUFFER FIX: ISSUES DETECTED" -ForegroundColor Red
    Write-Host "🔧 Some commands may still be truncated" -ForegroundColor Yellow
}

Write-Host "`n=== TEST COMPLETE ===" -ForegroundColor Cyan