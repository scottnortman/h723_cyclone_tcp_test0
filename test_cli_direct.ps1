# Direct CLI Test on Real Hardware
Write-Host "=== Direct CLI Buffer Test on Real Hardware ===" -ForegroundColor Cyan
Write-Host "Connecting to hardware at 192.168.0.20:23" -ForegroundColor White
Write-Host ""

try {
    # Create TCP client
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.Connect("192.168.0.20", 23)
    
    if ($tcpClient.Connected) {
        Write-Host "✅ Connected to hardware successfully" -ForegroundColor Green
        
        $stream = $tcpClient.GetStream()
        $writer = New-Object System.IO.StreamWriter($stream)
        $reader = New-Object System.IO.StreamReader($stream)
        
        $writer.AutoFlush = $true
        
        # Wait for initial prompt
        Start-Sleep -Seconds 2
        
        Write-Host ""
        Write-Host "Testing CLI Buffer Fix..." -ForegroundColor Cyan
        Write-Host ""
        
        # Test 1: New buffer test command
        Write-Host "Test 1: uavcan-test-buffer (NEW command to test buffer fix)" -ForegroundColor Yellow
        $writer.WriteLine("uavcan-test-buffer")
        Start-Sleep -Seconds 3
        
        $response1 = ""
        while ($stream.DataAvailable) {
            $response1 += $reader.ReadLine() + "`n"
        }
        
        Write-Host "Response length: $($response1.Length) characters" -ForegroundColor Gray
        if ($response1 -like "*buffer fix is working*") {
            Write-Host "✅ PASS: Buffer test shows complete output" -ForegroundColor Green
        } else {
            Write-Host "❌ FAIL: Buffer test may be truncated" -ForegroundColor Red
        }
        Write-Host "Response: $response1" -ForegroundColor DarkGray
        Write-Host ""
        
        # Test 2: Existing UAVCAN test command
        Write-Host "Test 2: uavcan-test (Should now show complete output)" -ForegroundColor Yellow
        $writer.WriteLine("uavcan-test")
        Start-Sleep -Seconds 3
        
        $response2 = ""
        while ($stream.DataAvailable) {
            $response2 += $reader.ReadLine() + "`n"
        }
        
        Write-Host "Response length: $($response2.Length) characters" -ForegroundColor Gray
        if ($response2 -like "*ALL BASIC TESTS PASSED*") {
            Write-Host "✅ PASS: UAVCAN test shows complete output" -ForegroundColor Green
        } else {
            Write-Host "❌ FAIL: UAVCAN test may be truncated" -ForegroundColor Red
        }
        Write-Host "Response: $response2" -ForegroundColor DarkGray
        Write-Host ""
        
        # Test 3: Requirements verification
        Write-Host "Test 3: uavcan-verify-requirements (Longest output)" -ForegroundColor Yellow
        $writer.WriteLine("uavcan-verify-requirements")
        Start-Sleep -Seconds 4
        
        $response3 = ""
        while ($stream.DataAvailable) {
            $response3 += $reader.ReadLine() + "`n"
        }
        
        Write-Host "Response length: $($response3.Length) characters" -ForegroundColor Gray
        if ($response3 -like "*ALL REQUIREMENTS HAVE BASIC COMPLIANCE*") {
            Write-Host "✅ PASS: Requirements test shows complete output" -ForegroundColor Green
        } else {
            Write-Host "❌ FAIL: Requirements test may be truncated" -ForegroundColor Red
        }
        Write-Host "Response: $response3" -ForegroundColor DarkGray
        Write-Host ""
        
        # Summary
        Write-Host "=== CLI Buffer Fix Test Results ===" -ForegroundColor Cyan
        $passCount = 0
        if ($response1 -like "*buffer fix is working*") { $passCount++ }
        if ($response2 -like "*ALL BASIC TESTS PASSED*") { $passCount++ }
        if ($response3 -like "*ALL REQUIREMENTS HAVE BASIC COMPLIANCE*") { $passCount++ }
        
        Write-Host "Tests Passed: $passCount / 3" -ForegroundColor $(if ($passCount -eq 3) { "Green" } else { "Red" })
        
        if ($passCount -eq 3) {
            Write-Host "🎉 CLI BUFFER FIX: SUCCESS" -ForegroundColor Green
            Write-Host "All commands now output complete text without truncation" -ForegroundColor Green
        } else {
            Write-Host "❌ CLI BUFFER FIX: NEEDS INVESTIGATION" -ForegroundColor Red
            Write-Host "Some commands may still be truncated" -ForegroundColor Red
        }
        
    } else {
        Write-Host "❌ Failed to connect to hardware" -ForegroundColor Red
    }
    
} catch {
    Write-Host "❌ Connection error: $($_.Exception.Message)" -ForegroundColor Red
    
} finally {
    if ($tcpClient -and $tcpClient.Connected) {
        $tcpClient.Close()
    }
}

Write-Host ""
Write-Host "=== Direct Hardware Test Complete ===" -ForegroundColor Cyan