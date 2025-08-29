# CLI Buffer Fix HIL Test Script
# This script performs Hardware-in-the-Loop testing to verify the CLI buffer truncation fix

param(
    [string]$ComPort = "COM3",
    [int]$BaudRate = 115200,
    [int]$TimeoutMs = 5000
)

Write-Host "CLI Buffer Fix HIL Test" -ForegroundColor Cyan
Write-Host "======================" -ForegroundColor Cyan
Write-Host "Testing CLI buffer truncation fix on actual hardware"
Write-Host "Port: $ComPort, Baud: $BaudRate"
Write-Host ""

# Test results tracking
$testResults = @()
$overallPass = $true

function Test-CLICommand {
    param(
        [System.IO.Ports.SerialPort]$Port,
        [string]$Command,
        [string]$TestName,
        [int]$MinExpectedLength,
        [string]$ExpectedEndMarker = ""
    )
    
    Write-Host "Testing: $TestName" -ForegroundColor Yellow
    Write-Host "Command: $Command"
    
    try {
        # Send command
        $Port.WriteLine($Command)
        Start-Sleep -Milliseconds 100
        
        # Read response with timeout
        $response = ""
        $startTime = Get-Date
        
        while (((Get-Date) - $startTime).TotalMilliseconds -lt $TimeoutMs) {
            if ($Port.BytesToRead -gt 0) {
                $response += $Port.ReadExisting()
                
                # Check if we have a complete response (ends with prompt)
                if ($response -match '>\s*$') {
                    break
                }
            }
            Start-Sleep -Milliseconds 50
        }
        
        # Clean up response (remove command echo and prompt)
        $cleanResponse = $response -replace "^.*?$Command\s*", "" -replace '>\s*$', ""
        $cleanResponse = $cleanResponse.Trim()
        
        Write-Host "Response Length: $($cleanResponse.Length) characters"
        
        # Test 1: Minimum length check
        $lengthPass = $cleanResponse.Length -ge $MinExpectedLength
        Write-Host "Length Test: $(if ($lengthPass) { 'PASS' } else { 'FAIL' }) (Expected: >= $MinExpectedLength, Got: $($cleanResponse.Length))"
        
        # Test 2: End marker check (if specified)
        $markerPass = $true
        if ($ExpectedEndMarker -ne "") {
            $markerPass = $cleanResponse -match [regex]::Escape($ExpectedEndMarker)
            Write-Host "End Marker Test: $(if ($markerPass) { 'PASS' } else { 'FAIL' }) (Looking for: '$ExpectedEndMarker')"
        }
        
        # Test 3: Status line check
        $statusPass = $cleanResponse -match "Status:\s*(PASS|FAIL)"
        Write-Host "Status Format Test: $(if ($statusPass) { 'PASS' } else { 'FAIL' })"
        
        # Test 4: Error line check
        $errorPass = $cleanResponse -match "Error:\s*"
        Write-Host "Error Format Test: $(if ($errorPass) { 'PASS' } else { 'FAIL' })"
        
        # Test 5: No truncation at 128 characters
        $truncationPass = $cleanResponse.Length -gt 128 -or $cleanResponse -notmatch "^.{128}$"
        Write-Host "No 128-byte Truncation: $(if ($truncationPass) { 'PASS' } else { 'FAIL' })"
        
        $overallTestPass = $lengthPass -and $markerPass -and $statusPass -and $errorPass -and $truncationPass
        
        $testResult = @{
            TestName = $TestName
            Command = $Command
            ResponseLength = $cleanResponse.Length
            MinExpectedLength = $MinExpectedLength
            LengthPass = $lengthPass
            MarkerPass = $markerPass
            StatusPass = $statusPass
            ErrorPass = $errorPass
            TruncationPass = $truncationPass
            OverallPass = $overallTestPass
            Response = $cleanResponse
        }
        
        Write-Host "Overall Result: $(if ($overallTestPass) { 'PASS' } else { 'FAIL' })" -ForegroundColor $(if ($overallTestPass) { 'Green' } else { 'Red' })
        Write-Host ""
        
        return $testResult
    }
    catch {
        Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
        return @{
            TestName = $TestName
            Command = $Command
            OverallPass = $false
            Error = $_.Exception.Message
        }
    }
}

function Connect-CLI {
    param([System.IO.Ports.SerialPort]$Port)
    
    Write-Host "Establishing CLI connection..." -ForegroundColor Yellow
    
    # Try multiple carriage returns as per steering docs
    $maxRetries = 3
    $connected = $false
    
    for ($i = 0; $i -lt $maxRetries -and -not $connected; $i++) {
        Write-Host "Connection attempt $($i + 1)..."
        
        # Send carriage return
        $Port.Write([byte]0x0D)
        Start-Sleep -Milliseconds 200
        
        # Check for prompt
        if ($Port.BytesToRead -gt 0) {
            $response = $Port.ReadExisting()
            if ($response -match '>') {
                $connected = $true
                Write-Host "CLI connection established!" -ForegroundColor Green
            }
        }
    }
    
    if (-not $connected) {
        throw "Failed to establish CLI connection after $maxRetries attempts"
    }
    
    # Clear any remaining data
    Start-Sleep -Milliseconds 100
    if ($Port.BytesToRead -gt 0) {
        $Port.ReadExisting() | Out-Null
    }
}

try {
    # Step 1: Build and flash firmware
    Write-Host "Step 1: Building firmware..." -ForegroundColor Cyan
    $buildResult = & ".\build.bat"
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE"
    }
    Write-Host "Build successful!" -ForegroundColor Green
    
    Write-Host "Step 2: Flashing hardware..." -ForegroundColor Cyan
    $flashResult = & ".\program_hardware.bat"
    if ($LASTEXITCODE -ne 0) {
        throw "Flash failed with exit code $LASTEXITCODE"
    }
    Write-Host "Flash successful!" -ForegroundColor Green
    
    # Step 3: Wait for hardware to boot
    Write-Host "Step 3: Waiting for hardware to boot..." -ForegroundColor Cyan
    Start-Sleep -Seconds 3
    
    # Step 4: Connect to serial port
    Write-Host "Step 4: Connecting to serial port..." -ForegroundColor Cyan
    $port = New-Object System.IO.Ports.SerialPort
    $port.PortName = $ComPort
    $port.BaudRate = $BaudRate
    $port.DataBits = 8
    $port.Parity = [System.IO.Ports.Parity]::None
    $port.StopBits = [System.IO.Ports.StopBits]::One
    $port.ReadTimeout = $TimeoutMs
    $port.WriteTimeout = $TimeoutMs
    
    $port.Open()
    Write-Host "Serial port opened successfully!" -ForegroundColor Green
    
    # Step 5: Establish CLI connection
    Connect-CLI -Port $port
    
    # Step 6: Execute CLI buffer tests
    Write-Host "Step 5: Executing CLI buffer tests..." -ForegroundColor Cyan
    Write-Host ""
    
    # Test 1: Buffer test command (should be longest output)
    $testResults += Test-CLICommand -Port $port -Command "uavcan-test-buffer" -TestName "CLI Buffer Test" -MinExpectedLength 500 -ExpectedEndMarker "END_MARKER: Buffer test completed successfully"
    
    # Test 2: Requirements verification (should be long output)
    $testResults += Test-CLICommand -Port $port -Command "uavcan-verify-requirements" -TestName "Requirements Verification" -MinExpectedLength 400
    
    # Test 3: HIL test (should exceed old 128-byte limit)
    $testResults += Test-CLICommand -Port $port -Command "uavcan-test" -TestName "HIL Test" -MinExpectedLength 300
    
    # Test 4: System test
    $testResults += Test-CLICommand -Port $port -Command "uavcan-system-test" -TestName "System Test" -MinExpectedLength 300
    
    # Test 5: Status command
    $testResults += Test-CLICommand -Port $port -Command "uavcan-status" -TestName "Status Command" -MinExpectedLength 200
    
    # Test 6: Simple verify
    $testResults += Test-CLICommand -Port $port -Command "uavcan-simple-verify" -TestName "Simple Verify" -MinExpectedLength 150
    
}
catch {
    Write-Host "CRITICAL ERROR: $($_.Exception.Message)" -ForegroundColor Red
    $overallPass = $false
}
finally {
    # Clean up
    if ($port -and $port.IsOpen) {
        $port.Close()
        $port.Dispose()
    }
}

# Step 7: Generate test report
Write-Host "CLI Buffer Fix HIL Test Results" -ForegroundColor Cyan
Write-Host "===============================" -ForegroundColor Cyan

$passCount = 0
$failCount = 0

foreach ($result in $testResults) {
    $status = if ($result.OverallPass) { "PASS" } else { "FAIL" }
    $color = if ($result.OverallPass) { "Green" } else { "Red" }
    
    Write-Host "$($result.TestName): $status" -ForegroundColor $color
    
    if ($result.OverallPass) {
        $passCount++
    } else {
        $failCount++
        $overallPass = $false
    }
}

Write-Host ""
Write-Host "Summary:" -ForegroundColor Cyan
Write-Host "  Total Tests: $($testResults.Count)"
Write-Host "  Passed: $passCount" -ForegroundColor Green
Write-Host "  Failed: $failCount" -ForegroundColor $(if ($failCount -gt 0) { 'Red' } else { 'Green' })
Write-Host ""

if ($overallPass) {
    Write-Host "CLI BUFFER FIX HIL TEST: PASS" -ForegroundColor Green
    Write-Host "All CLI commands output complete text without truncation" -ForegroundColor Green
    exit 0
} else {
    Write-Host "CLI BUFFER FIX HIL TEST: FAIL" -ForegroundColor Red
    Write-Host "One or more CLI commands failed buffer tests" -ForegroundColor Red
    exit 1
}