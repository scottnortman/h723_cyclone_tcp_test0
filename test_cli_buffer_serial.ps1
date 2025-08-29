#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Real Serial CLI Buffer Test
.DESCRIPTION
    Tests CLI buffer fix using actual serial communication with hardware
.NOTES
    Requires .NET System.IO.Ports for serial communication
#>

param(
    [string]$ComPort = "COM3",
    [int]$BaudRate = 115200,
    [int]$CommandTimeout = 5000
)

Write-Host "=== REAL SERIAL CLI Buffer Test ===" -ForegroundColor Cyan
Write-Host "Testing actual hardware via serial connection" -ForegroundColor White
Write-Host ""

# Load .NET serial port assembly
Add-Type -AssemblyName System.IO.Ports

# Test tracking
$TestResults = @()
$TotalTests = 0
$PassedTests = 0
$FailedTests = 0

function Send-SerialCommand {
    param(
        [System.IO.Ports.SerialPort]$Port,
        [string]$Command,
        [int]$TimeoutMs = 5000
    )
    
    try {
        # Clear any existing data
        $Port.DiscardInBuffer()
        $Port.DiscardOutBuffer()
        
        # Send command
        Write-Host "  → Sending: $Command" -ForegroundColor Gray
        $Port.WriteLine($Command)
        
        # Read response with timeout
        $response = ""
        $startTime = Get-Date
        $endTime = $startTime.AddMilliseconds($TimeoutMs)
        
        while ((Get-Date) -lt $endTime) {
            if ($Port.BytesToRead -gt 0) {
                $data = $Port.ReadExisting()
                $response += $data
                
                # Check if we have a complete response (ends with prompt or specific marker)
                if ($response -match ">\s*$" -or $response -like "*Note:*") {
                    break
                }
            }
            Start-Sleep -Milliseconds 50
        }
        
        Write-Host "  ← Received: $($response.Length) characters" -ForegroundColor Gray
        return $response.Trim()
        
    } catch {
        Write-Host "  ❌ Serial error: $($_.Exception.Message)" -ForegroundColor Red
        return $null
    }
}

function Test-SerialCLICommand {
    param(
        [System.IO.Ports.SerialPort]$Port,
        [string]$Command,
        [string]$Description,
        [int]$MinExpectedLength,
        [string]$ExpectedContent
    )
    
    $global:TotalTests++
    Write-Host "Test $global:TotalTests`: $Description" -ForegroundColor Yellow
    
    try {
        $response = Send-SerialCommand -Port $Port -Command $Command -TimeoutMs $CommandTimeout
        
        if ($null -eq $response) {
            Write-Host "  ❌ FAIL: No response received" -ForegroundColor Red
            $global:FailedTests++
            return
        }
        
        # Analyze response
        $responseLength = $response.Length
        $hasExpectedContent = $response -like "*$ExpectedContent*"
        $meetsMinLength = $responseLength -ge $MinExpectedLength
        
        # Check for truncation indicators
        $truncatedAt128 = $responseLength -eq 128 -and -not $hasExpectedContent
        $appearsComplete = $hasExpectedContent -and $meetsMinLength
        
        Write-Host "  Response Length: $responseLength characters" -ForegroundColor Gray
        Write-Host "  Expected Min: $MinExpectedLength characters" -ForegroundColor Gray
        Write-Host "  Has Expected Content: $hasExpectedContent" -ForegroundColor Gray
        
        # Show first and last parts of response for verification
        if ($responseLength -gt 100) {
            $preview = $response.Substring(0, [Math]::Min(50, $responseLength)) + "..." + 
                      $response.Substring([Math]::Max(0, $responseLength - 50))
            Write-Host "  Preview: $preview" -ForegroundColor DarkGray
        } else {
            Write-Host "  Full Response: $response" -ForegroundColor DarkGray
        }
        
        if ($appearsComplete -and -not $truncatedAt128) {
            Write-Host "  ✅ PASS: Complete response received" -ForegroundColor Green
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

# Step 1: Build and program firmware
Write-Host "Step 1: Build and program firmware..." -ForegroundColor Cyan
Write-Host "  Building with CLI buffer fix (512 bytes)..." -ForegroundColor Gray

try {
    # Build firmware
    if (Test-Path "build.bat") {
        Write-Host "  Executing build.bat..." -ForegroundColor Gray
        $buildResult = & .\build.bat 2>&1
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed with exit code $LASTEXITCODE"
        }
        Write-Host "  ✅ Build successful" -ForegroundColor Green
    } else {
        Write-Host "  ⚠️  build.bat not found, assuming build is current" -ForegroundColor Yellow
    }
    
    # Program hardware
    if (Test-Path "program_hardware.bat") {
        Write-Host "  Executing program_hardware.bat..." -ForegroundColor Gray
        $progResult = & .\program_hardware.bat 2>&1
        if ($LASTEXITCODE -ne 0) {
            throw "Programming failed with exit code $LASTEXITCODE"
        }
        Write-Host "  ✅ Programming successful" -ForegroundColor Green
    } else {
        Write-Host "  ⚠️  program_hardware.bat not found, assuming hardware is programmed" -ForegroundColor Yellow
    }
    
} catch {
    Write-Host "  ❌ Build/Program failed: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "Cannot proceed without successful build and programming" -ForegroundColor Red
    exit 1
}
Write-Host ""

# Step 2: Connect to hardware
Write-Host "Step 2: Connecting to hardware..." -ForegroundColor Cyan
Write-Host "  Serial Port: $ComPort" -ForegroundColor Gray
Write-Host "  Baud Rate: $BaudRate" -ForegroundColor Gray

try {
    $serialPort = New-Object System.IO.Ports.SerialPort
    $serialPort.PortName = $ComPort
    $serialPort.BaudRate = $BaudRate
    $serialPort.Parity = [System.IO.Ports.Parity]::None
    $serialPort.DataBits = 8
    $serialPort.StopBits = [System.IO.Ports.StopBits]::One
    $serialPort.Handshake = [System.IO.Ports.Handshake]::None
    $serialPort.ReadTimeout = $CommandTimeout
    $serialPort.WriteTimeout = $CommandTimeout
    
    Write-Host "  Opening serial connection..." -ForegroundColor Gray
    $serialPort.Open()
    
    if ($serialPort.IsOpen) {
        Write-Host "  ✅ Serial connection established" -ForegroundColor Green
    } else {
        throw "Failed to open serial port"
    }
    
    # Wait for system boot
    Write-Host "  Waiting 3 seconds for system initialization..." -ForegroundColor Gray
    Start-Sleep -Seconds 3
    
    # Send initial command to verify connection
    Write-Host "  Testing connection with help command..." -ForegroundColor Gray
    $helpResponse = Send-SerialCommand -Port $serialPort -Command "help" -TimeoutMs 3000
    
    if ($null -ne $helpResponse -and $helpResponse.Length -gt 10) {
        Write-Host "  ✅ CLI connection verified" -ForegroundColor Green
    } else {
        Write-Host "  ⚠️  CLI connection may be limited" -ForegroundColor Yellow
    }
    
} catch {
    Write-Host "  ❌ Connection failed: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "Check that:" -ForegroundColor Yellow
    Write-Host "  - Hardware is connected to $ComPort" -ForegroundColor Yellow
    Write-Host "  - No other applications are using the port" -ForegroundColor Yellow
    Write-Host "  - Hardware is powered and running" -ForegroundColor Yellow
    exit 1
}
Write-Host ""

# Step 3: Test CLI commands
Write-Host "Step 3: Testing CLI commands for buffer integrity..." -ForegroundColor Cyan

try {
    # Test the new buffer test command
    Test-SerialCLICommand -Port $serialPort -Command "uavcan-test-buffer" -Description "CLI Buffer Test Command" -MinExpectedLength 400 -ExpectedContent "buffer fix is working"
    
    # Test existing commands that were being truncated
    Test-SerialCLICommand -Port $serialPort -Command "uavcan-test" -Description "UAVCAN HIL Test Command" -MinExpectedLength 250 -ExpectedContent "ALL BASIC TESTS PASSED"
    
    Test-SerialCLICommand -Port $serialPort -Command "uavcan-verify-requirements" -Description "Requirements Verification Command" -MinExpectedLength 500 -ExpectedContent "ALL REQUIREMENTS HAVE BASIC COMPLIANCE"
    
    Test-SerialCLICommand -Port $serialPort -Command "uavcan-status" -Description "UAVCAN Status Command" -MinExpectedLength 100 -ExpectedContent "UAVCAN System Status"
    
} finally {
    # Always close the serial port
    if ($serialPort.IsOpen) {
        Write-Host "Closing serial connection..." -ForegroundColor Gray
        $serialPort.Close()
    }
    $serialPort.Dispose()
}

# Generate results
Write-Host "=== REAL HARDWARE SERIAL TEST RESULTS ===" -ForegroundColor Cyan
Write-Host ""

Write-Host "Hardware Configuration:" -ForegroundColor White
Write-Host "  Serial Port: $ComPort" -ForegroundColor Gray
Write-Host "  Baud Rate: $BaudRate" -ForegroundColor Gray
Write-Host "  Command Timeout: $CommandTimeout ms" -ForegroundColor Gray
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
    Write-Host "🎉 REAL HARDWARE SERIAL TEST: SUCCESS" -ForegroundColor Green
    Write-Host "✅ CLI buffer fix verified on actual hardware via serial" -ForegroundColor Green
    Write-Host "✅ All commands output complete text without truncation" -ForegroundColor Green
    Write-Host "✅ 512-byte buffer successfully resolves truncation issue" -ForegroundColor Green
    $exitCode = 0
} else {
    Write-Host "❌ REAL HARDWARE SERIAL TEST: FAILED" -ForegroundColor Red
    Write-Host "❌ Some commands still show truncation on hardware" -ForegroundColor Red
    Write-Host "🔧 Further investigation required" -ForegroundColor Yellow
    $exitCode = 1
}

Write-Host ""
Write-Host "=== REAL HARDWARE SERIAL TEST COMPLETE ===" -ForegroundColor Cyan

exit $exitCode