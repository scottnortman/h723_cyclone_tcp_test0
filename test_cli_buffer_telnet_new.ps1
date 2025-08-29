# CLI Buffer Test via Telnet - New Connection
param(
    [string]$TargetIP = "192.168.0.20",
    [int]$TelnetPort = 23,
    [int]$TimeoutSeconds = 10
)

Write-Host "=== CLI Buffer Test via Telnet ===" -ForegroundColor Cyan
Write-Host "Testing CLI buffer fix on actual hardware" -ForegroundColor White
Write-Host ""

Write-Host "Target: $TargetIP`:$TelnetPort" -ForegroundColor Gray
Write-Host "Timeout: $TimeoutSeconds seconds" -ForegroundColor Gray
Write-Host ""

# Test results
$TestResults = @()
$TotalTests = 0
$PassedTests = 0
$FailedTests = 0

function Send-TelnetCommand {
    param(
        [string]$Command,
        [string]$TargetIP,
        [int]$Port,
        [int]$TimeoutMs = 10000
    )
    
    try {
        Write-Host "  → Connecting to $TargetIP`:$Port..." -ForegroundColor Gray
        
        $tcpClient = New-Object System.Net.Sockets.TcpClient
        $tcpClient.ReceiveTimeout = $TimeoutMs
        $tcpClient.SendTimeout = $TimeoutMs
        
        # Try to connect
        $connectTask = $tcpClient.ConnectAsync($TargetIP, $Port)
        if (-not $connectTask.Wait($TimeoutMs)) {
            throw "Connection timeout"
        }
        
        if (-not $tcpClient.Connected) {
            throw "Failed to connect"
        }
        
        Write-Host "  → Connected successfully" -ForegroundColor Gray
        
        $stream = $tcpClient.GetStream()
        $writer = New-Object System.IO.StreamWriter($stream)
        $reader = New-Object System.IO.StreamReader($stream)
        
        $writer.AutoFlush = $true
        
        # Wait a moment for connection to stabilize
        Start-Sleep -Milliseconds 500
        
        # Send command
        Write-Host "  → Sending: $Command" -ForegroundColor Gray
        $writer.WriteLine($Command)
        
        # Read response with timeout
        $response = ""
        $startTime = Get-Date
        $endTime = $startTime.AddMilliseconds($TimeoutMs)
        
        while ((Get-Date) -lt $endTime) {
            if ($stream.DataAvailable) {
                $data = $reader.ReadLine()
                if ($data) {
                    $response += $data + "`r`n"
                }
            }
            
            # Check if we have a reasonable response
            if ($response.Length -gt 50 -and ($response -like "*Note:*" -or $response -like "*>*")) {
                break
            }
            
            Start-Sleep -Milliseconds 100
        }
        
        Write-Host "  ← Received: $($response.Length) characters" -ForegroundColor Gray
        
        # Clean up
        $reader.Close()
        $writer.Close()
        $stream.Close()
        $tcpClient.Close()
        
        return $response.Trim()
        
    } catch {
        Write-Host "  ❌ Telnet error: $($_.Exception.Message)" -ForegroundColor Red
        
        # Clean up on error
        if ($reader) { $reader.Close() }
        if ($writer) { $writer.Close() }
        if ($stream) { $stream.Close() }
        if ($tcpClient) { $tcpClient.Close() }
        
        return $null
    }
}

function Test-CLIBufferCommand {
    param(
        [string]$Command,
        [string]$Description,
        [int]$MinExpectedLength,
        [string]$ExpectedContent
    )
    
    $global:TotalTests++
    Write-Host "Test $global:TotalTests`: $Description" -ForegroundColor Yellow
    
    try {
        $response = Send-TelnetCommand -Command $Command -TargetIP $TargetIP -Port $TelnetPort -TimeoutMs ($TimeoutSeconds * 1000)
        
        if ($null -eq $response) {
            Write-Host "  ❌ FAIL: No response received" -ForegroundColor Red
            $global:FailedTests++
            return
        }
        
        # Analyze response
        $responseLength = $response.Length
        $hasExpectedContent = $response -like "*$ExpectedContent*"
        $meetsMinLength = $responseLength -ge $MinExpectedLength
        
        # Check for truncation at old 128-byte limit
        $truncatedAt128 = $responseLength -eq 128 -and -not $hasExpectedContent
        $appearsComplete = $hasExpectedContent -and $meetsMinLength
        
        Write-Host "  Response Length: $responseLength characters" -ForegroundColor Gray
        Write-Host "  Expected Min: $MinExpectedLength characters" -ForegroundColor Gray
        Write-Host "  Has Expected Content: $hasExpectedContent" -ForegroundColor Gray
        
        # Show preview of response
        if ($responseLength -gt 100) {
            $preview = $response.Substring(0, [Math]::Min(50, $responseLength)) + "..." + 
                      $response.Substring([Math]::Max(0, $responseLength - 50))
            Write-Host "  Preview: $preview" -ForegroundColor DarkGray
        } else {
            Write-Host "  Full Response: $response" -ForegroundColor DarkGray
        }
        
        if ($appearsComplete -and -not $truncatedAt128) {
            Write-Host "  ✅ PASS: Complete response received, no truncation" -ForegroundColor Green
            $global:PassedTests++
            $result = "PASS"
        } else {
            Write-Host "  ❌ FAIL: Response appears truncated or incomplete" -ForegroundColor Red
            $global:FailedTests++
            $result = "FAIL"
            
            if ($truncatedAt128) {
                Write-Host "    - Truncated at 128 bytes (old buffer limit)" -ForegroundColor Red
            }
            if (-not $hasExpectedContent) {
                Write-Host "    - Missing expected content: $ExpectedContent" -ForegroundColor Red
            }
            if (-not $meetsMinLength) {
                Write-Host "    - Response too short" -ForegroundColor Red
            }
        }
        
        $global:TestResults += [PSCustomObject]@{
            Command = $Command
            Description = $Description
            Result = $result
            ResponseLength = $responseLength
            ExpectedMinLength = $MinExpectedLength
            HasExpectedContent = $hasExpectedContent
            TruncatedAt128 = $truncatedAt128
        }
        
    } catch {
        Write-Host "  ❌ ERROR: $($_.Exception.Message)" -ForegroundColor Red
        $global:FailedTests++
    }
    
    Write-Host ""
}

# Test hardware connectivity first
Write-Host "Step 1: Testing hardware connectivity..." -ForegroundColor Cyan
try {
    $connectTest = Send-TelnetCommand -Command "help" -TargetIP $TargetIP -Port $TelnetPort -TimeoutMs 5000
    if ($connectTest -and $connectTest.Length -gt 10) {
        Write-Host "✅ Hardware is responding via telnet" -ForegroundColor Green
    } else {
        Write-Host "❌ Hardware not responding properly" -ForegroundColor Red
        Write-Host "Check that hardware is powered and network connected" -ForegroundColor Yellow
        exit 1
    }
} catch {
    Write-Host "❌ Cannot connect to hardware: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "Possible issues:" -ForegroundColor Yellow
    Write-Host "  - Hardware not powered or connected" -ForegroundColor Yellow
    Write-Host "  - Network configuration issues" -ForegroundColor Yellow
    Write-Host "  - Existing telnet sessions blocking connection" -ForegroundColor Yellow
    exit 1
}
Write-Host ""

# Test CLI commands for buffer integrity
Write-Host "Step 2: Testing CLI commands for buffer integrity..." -ForegroundColor Cyan

# Test the new buffer test command
Test-CLIBufferCommand -Command "uavcan-test-buffer" -Description "CLI Buffer Test Command" -MinExpectedLength 400 -ExpectedContent "buffer fix is working"

# Test existing commands that were being truncated
Test-CLIBufferCommand -Command "uavcan-test" -Description "UAVCAN HIL Test Command" -MinExpectedLength 250 -ExpectedContent "ALL BASIC TESTS PASSED"

Test-CLIBufferCommand -Command "uavcan-verify-requirements" -Description "Requirements Verification Command" -MinExpectedLength 500 -ExpectedContent "ALL REQUIREMENTS HAVE BASIC COMPLIANCE"

Test-CLIBufferCommand -Command "uavcan-status" -Description "UAVCAN Status Command" -MinExpectedLength 100 -ExpectedContent "UAVCAN System Status"

# Generate results
Write-Host "=== CLI BUFFER TEST RESULTS ===" -ForegroundColor Cyan
Write-Host ""

Write-Host "Hardware Configuration:" -ForegroundColor White
Write-Host "  Target: $TargetIP`:$TelnetPort" -ForegroundColor Gray
Write-Host "  Connection: Telnet" -ForegroundColor Gray
Write-Host "  Timeout: $TimeoutSeconds seconds" -ForegroundColor Gray
Write-Host ""

Write-Host "Buffer Configuration:" -ForegroundColor White
Write-Host "  Old Buffer Size: 128 bytes" -ForegroundColor Gray
Write-Host "  New Buffer Size: 512 bytes" -ForegroundColor Green
Write-Host "  Improvement: 4x increase" -ForegroundColor Green
Write-Host ""

Write-Host "Test Summary:" -ForegroundColor White
Write-Host "  Total Tests: $TotalTests" -ForegroundColor Gray
Write-Host "  Passed: $PassedTests" -ForegroundColor Green
Write-Host "  Failed: $FailedTests" -ForegroundColor $(if ($FailedTests -eq 0) { "Green" } else { "Red" })
Write-Host ""

if ($TestResults.Count -gt 0) {
    Write-Host "Detailed Results:" -ForegroundColor White
    $TestResults | Format-Table -AutoSize
}

# Final assessment
if ($FailedTests -eq 0) {
    Write-Host "🎉 CLI BUFFER TEST: SUCCESS" -ForegroundColor Green
    Write-Host "✅ CLI buffer fix verified on actual hardware" -ForegroundColor Green
    Write-Host "✅ All commands output complete text without truncation" -ForegroundColor Green
    Write-Host "✅ 512-byte buffer successfully resolves truncation issue" -ForegroundColor Green
    $exitCode = 0
} else {
    Write-Host "❌ CLI BUFFER TEST: FAILED" -ForegroundColor Red
    Write-Host "❌ Some commands still show truncation on hardware" -ForegroundColor Red
    Write-Host "🔧 Further investigation required" -ForegroundColor Yellow
    $exitCode = 1
}

Write-Host ""
Write-Host "=== CLI BUFFER TEST COMPLETE ===" -ForegroundColor Cyan

exit $exitCode