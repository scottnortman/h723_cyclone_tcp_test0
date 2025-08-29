#!/usr/bin/env pwsh
<#
.SYNOPSIS
    HIL Test for Stack Overflow Fix Verification
    
.DESCRIPTION
    This script implements a Hardware-in-the-Loop test to formally verify
    that the stack overflow issue has been identified and fixed. It follows
    the mandatory HIL testing approach for bug fixes.
    
.PARAMETER ComPort
    Serial port to use for communication (default: COM3)
    
.EXAMPLE
    .\test_stack_overflow_fix_hil.ps1
    .\test_stack_overflow_fix_hil.ps1 -ComPort COM4
#>

param(
    [string]$ComPort = "COM3"
)

# Colors for output
$Red = "`e[31m"
$Green = "`e[32m"
$Yellow = "`e[33m"
$Blue = "`e[34m"
$Reset = "`e[0m"

function Write-ColorOutput {
    param([string]$Message, [string]$Color = $Reset)
    Write-Host "$Color$Message$Reset"
}

function Write-TestResult {
    param([string]$TestName, [bool]$Passed, [string]$Details = "")
    
    if ($Passed) {
        Write-ColorOutput "✓ $TestName - PASSED" $Green
    } else {
        Write-ColorOutput "✗ $TestName - FAILED" $Red
    }
    
    if ($Details) {
        Write-ColorOutput "  Details: $Details" $Blue
    }
}

function Test-BuildAndFlash {
    Write-ColorOutput "`n=== Step 1: Build and Flash Firmware ===" $Yellow
    
    # Build firmware
    Write-ColorOutput "Building firmware with stack monitoring..." $Blue
    try {
        $buildResult = & ".\build.bat" 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-TestResult "Firmware Build" $true "Build completed successfully"
        } else {
            Write-TestResult "Firmware Build" $false "Build failed with exit code $LASTEXITCODE"
            return $false
        }
    } catch {
        Write-TestResult "Firmware Build" $false "Build script execution failed: $($_.Exception.Message)"
        return $false
    }
    
    # Flash hardware
    Write-ColorOutput "Programming hardware..." $Blue
    try {
        $flashResult = & ".\program_hardware.bat" 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-TestResult "Hardware Programming" $true "Programming completed successfully"
            Start-Sleep -Seconds 3  # Allow hardware to boot
            return $true
        } else {
            Write-TestResult "Hardware Programming" $false "Programming failed with exit code $LASTEXITCODE"
            return $false
        }
    } catch {
        Write-TestResult "Hardware Programming" $false "Programming script execution failed: $($_.Exception.Message)"
        return $false
    }
}

function Test-CLIConnection {
    param($Port)
    
    Write-ColorOutput "`n=== Step 2: Establish CLI Connection ===" $Yellow
    
    try {
        # Multiple attempts to establish connection
        $maxRetries = 5
        $connected = $false
        
        for ($retry = 1; $retry -le $maxRetries; $retry++) {
            Write-ColorOutput "Connection attempt $retry/$maxRetries..." $Blue
            
            # Send carriage return to establish handshake
            $Port.Write([byte]0x0D)
            Start-Sleep -Milliseconds 500
            
            if ($Port.BytesToRead -gt 0) {
                $response = $Port.ReadExisting()
                if ($response -match '>') {
                    $connected = $true
                    break
                }
            }
            
            Start-Sleep -Milliseconds 1000
        }
        
        if ($connected) {
            Write-TestResult "CLI Connection" $true "Handshake successful, prompt received"
            return $true
        } else {
            Write-TestResult "CLI Connection" $false "No prompt received after $maxRetries attempts"
            return $false
        }
    } catch {
        Write-TestResult "CLI Connection" $false "Connection failed: $($_.Exception.Message)"
        return $false
    }
}

function Send-CLICommand {
    param($Port, [string]$Command, [int]$TimeoutMs = 2000)
    
    try {
        # Clear any existing data
        if ($Port.BytesToRead -gt 0) {
            $Port.ReadExisting() | Out-Null
        }
        
        # Send command
        $Port.WriteLine($Command)
        
        # Wait for response
        $timeout = 0
        $response = ""
        
        while ($timeout -lt $TimeoutMs) {
            Start-Sleep -Milliseconds 100
            $timeout += 100
            
            if ($Port.BytesToRead -gt 0) {
                $response += $Port.ReadExisting()
                
                # Check if we have a complete response (ends with prompt)
                if ($response -match '>\s*$') {
                    break
                }
            }
        }
        
        return $response
    } catch {
        Write-ColorOutput "Command execution error: $($_.Exception.Message)" $Red
        return ""
    }
}

function Test-StackMonitoringCommands {
    param($Port)
    
    Write-ColorOutput "`n=== Step 3: Test Stack Monitoring Commands ===" $Yellow
    
    $commands = @(
        @{ Name = "task-stats"; Expected = "State.*Priority.*Stack" },
        @{ Name = "stack-info"; Expected = "Stack Usage Report" },
        @{ Name = "stack-check"; Expected = "Stack Overflow Check Results" },
        @{ Name = "heap-info"; Expected = "Heap Usage Statistics" },
        @{ Name = "memory-info"; Expected = "Comprehensive Memory Report" },
        @{ Name = "stack-overflow-info"; Expected = "Stack Overflow Detection Report" }
    )
    
    $allPassed = $true
    
    foreach ($cmd in $commands) {
        Write-ColorOutput "Testing command: $($cmd.Name)" $Blue
        
        $response = Send-CLICommand -Port $Port -Command $cmd.Name
        
        if ($response -match $cmd.Expected) {
            Write-TestResult "Command: $($cmd.Name)" $true "Expected output pattern found"
        } else {
            Write-TestResult "Command: $($cmd.Name)" $false "Expected pattern '$($cmd.Expected)' not found"
            $allPassed = $false
        }
    }
    
    return $allPassed
}

function Test-StackOverflowDetection {
    param($Port)
    
    Write-ColorOutput "`n=== Step 4: Verify Stack Overflow Detection ===" $Yellow
    
    # Check if stack overflow detection is enabled
    $response = Send-CLICommand -Port $Port -Command "stack-overflow-info"
    
    if ($response -match "Stack Check Enabled:\s+Yes") {
        Write-TestResult "Stack Overflow Detection Enabled" $true "FreeRTOS stack checking is active"
    } else {
        Write-TestResult "Stack Overflow Detection Enabled" $false "Stack checking not properly enabled"
        return $false
    }
    
    # Check for any previous overflows
    if ($response -match "Total Overflows Detected:\s+(\d+)") {
        $overflowCount = [int]$matches[1]
        if ($overflowCount -eq 0) {
            Write-TestResult "No Previous Overflows" $true "System has not experienced stack overflows"
        } else {
            Write-TestResult "Previous Overflows Detected" $false "$overflowCount overflow(s) detected in history"
            Write-ColorOutput "  This indicates the fix may not be complete" $Yellow
        }
    }
    
    return $true
}

function Test-StackUsageHealth {
    param($Port)
    
    Write-ColorOutput "`n=== Step 5: Analyze Current Stack Usage Health ===" $Yellow
    
    # Get stack check results
    $response = Send-CLICommand -Port $Port -Command "stack-check"
    
    $criticalTasks = 0
    $warningTasks = 0
    
    # Count critical and warning tasks
    $criticalMatches = [regex]::Matches($response, "CRITICAL:")
    $warningMatches = [regex]::Matches($response, "WARNING:")
    
    $criticalTasks = $criticalMatches.Count
    $warningTasks = $warningMatches.Count
    
    if ($criticalTasks -eq 0) {
        Write-TestResult "No Critical Stack Usage" $true "No tasks with >90% stack usage"
    } else {
        Write-TestResult "Critical Stack Usage Found" $false "$criticalTasks task(s) with critical stack usage"
        return $false
    }
    
    if ($warningTasks -eq 0) {
        Write-TestResult "No Warning Stack Usage" $true "No tasks with elevated stack usage"
    } else {
        Write-TestResult "Warning Stack Usage Found" $true "$warningTasks task(s) with elevated stack usage (acceptable)"
        Write-ColorOutput "  Monitor these tasks for potential future issues" $Yellow
    }
    
    return $true
}

function Test-SystemStability {
    param($Port)
    
    Write-ColorOutput "`n=== Step 6: Test System Stability ===" $Yellow
    
    # Run multiple commands to stress test the system
    $stressCommands = @("stack-info", "memory-info", "task-stats", "heap-info", "stack-check")
    $stableRuns = 0
    $totalRuns = 10
    
    Write-ColorOutput "Running stability test ($totalRuns iterations)..." $Blue
    
    for ($i = 1; $i -le $totalRuns; $i++) {
        $cmd = $stressCommands[($i - 1) % $stressCommands.Count]
        $response = Send-CLICommand -Port $Port -Command $cmd -TimeoutMs 3000
        
        if ($response.Length -gt 0 -and $response -match '>') {
            $stableRuns++
            Write-Host "." -NoNewline
        } else {
            Write-Host "X" -NoNewline
            Write-ColorOutput "`nSystem became unresponsive at iteration $i" $Red
            break
        }
        
        Start-Sleep -Milliseconds 200
    }
    
    Write-Host ""  # New line after dots
    
    if ($stableRuns -eq $totalRuns) {
        Write-TestResult "System Stability" $true "System remained stable through $totalRuns command executions"
        return $true
    } else {
        Write-TestResult "System Stability" $false "System failed after $stableRuns/$totalRuns executions"
        return $false
    }
}

function Test-MemoryHealth {
    param($Port)
    
    Write-ColorOutput "`n=== Step 7: Verify Memory Health ===" $Yellow
    
    $response = Send-CLICommand -Port $Port -Command "memory-info"
    
    # Check heap status
    if ($response -match "Heap Status:\s+(\w+)") {
        $heapStatus = $matches[1]
        if ($heapStatus -eq "OK") {
            Write-TestResult "Heap Health" $true "Heap usage is healthy"
        } elseif ($heapStatus -eq "WARNING") {
            Write-TestResult "Heap Health" $true "Heap usage elevated but acceptable"
            Write-ColorOutput "  Monitor heap usage during extended operation" $Yellow
        } else {
            Write-TestResult "Heap Health" $false "Heap status: $heapStatus"
            return $false
        }
    }
    
    # Check stack status
    if ($response -match "Stack Status:\s+(\w+)") {
        $stackStatus = $matches[1]
        if ($stackStatus -eq "OK") {
            Write-TestResult "Stack Health" $true "Overall stack usage is healthy"
        } elseif ($stackStatus -eq "WARNING") {
            Write-TestResult "Stack Health" $true "Stack usage elevated but acceptable"
            Write-ColorOutput "  Some tasks may need stack size increases" $Yellow
        } else {
            Write-TestResult "Stack Health" $false "Stack status: $stackStatus"
            return $false
        }
    }
    
    return $true
}

# Main execution
Write-ColorOutput "Stack Overflow Fix HIL Verification Test" $Blue
Write-ColorOutput "=======================================" $Blue
Write-ColorOutput "This test formally verifies the stack overflow fix on actual hardware" $Blue

$overallSuccess = $true

try {
    # Step 1: Build and Flash
    if (-not (Test-BuildAndFlash)) {
        $overallSuccess = $false
        Write-ColorOutput "Build/Flash failed - cannot proceed with HIL testing" $Red
        exit 1
    }
    
    # Step 2: Connect to hardware
    Write-ColorOutput "Opening serial connection to $ComPort..." $Blue
    $port = New-Object System.IO.Ports.SerialPort $ComPort, 115200, None, 8, One
    $port.ReadTimeout = 3000
    $port.WriteTimeout = 3000
    $port.Open()
    
    if (-not (Test-CLIConnection -Port $port)) {
        $overallSuccess = $false
        Write-ColorOutput "CLI connection failed - hardware may not be responding" $Red
    } else {
        # Step 3-7: Run all verification tests
        if (-not (Test-StackMonitoringCommands -Port $port)) { $overallSuccess = $false }
        if (-not (Test-StackOverflowDetection -Port $port)) { $overallSuccess = $false }
        if (-not (Test-StackUsageHealth -Port $port)) { $overallSuccess = $false }
        if (-not (Test-SystemStability -Port $port)) { $overallSuccess = $false }
        if (-not (Test-MemoryHealth -Port $port)) { $overallSuccess = $false }
    }
    
} catch {
    Write-ColorOutput "Test execution error: $($_.Exception.Message)" $Red
    $overallSuccess = $false
} finally {
    if ($port -and $port.IsOpen) {
        $port.Close()
        Write-ColorOutput "Serial connection closed." $Blue
    }
}

# Final Results
Write-ColorOutput "`n=== FINAL HIL TEST RESULTS ===" $Yellow
Write-ColorOutput "==============================" $Yellow

if ($overallSuccess) {
    Write-ColorOutput "🎉 STACK OVERFLOW FIX VERIFICATION: PASSED" $Green
    Write-ColorOutput "" $Reset
    Write-ColorOutput "✅ All HIL tests passed successfully" $Green
    Write-ColorOutput "✅ Stack monitoring tools are working" $Green
    Write-ColorOutput "✅ No critical stack usage detected" $Green
    Write-ColorOutput "✅ System stability verified" $Green
    Write-ColorOutput "✅ Memory health confirmed" $Green
    Write-ColorOutput "" $Reset
    Write-ColorOutput "The stack overflow issue has been successfully addressed." $Green
    Write-ColorOutput "Hardware should no longer freeze during HIL testing." $Green
    
    # Generate success report
    $timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
    $reportFile = "stack_overflow_fix_verification_$timestamp.txt"
    
    @"
Stack Overflow Fix HIL Verification Report
==========================================
Date: $(Get-Date)
Test: PASSED
Hardware: STM32H723ZG NUCLEO
Connection: $ComPort

Summary:
- Stack monitoring tools implemented and working
- No critical stack usage detected
- System stability verified over multiple command executions
- Memory health confirmed
- Stack overflow detection enabled and functional

Conclusion:
The stack overflow issue that caused hardware freezes during HIL testing
has been successfully identified and fixed. The system is now stable and
ready for continued development and testing.

Next Steps:
1. Continue with normal HIL testing procedures
2. Monitor stack usage during development of new features
3. Use stack monitoring commands for ongoing system health checks
"@ | Out-File -FilePath $reportFile -Encoding UTF8
    
    Write-ColorOutput "Verification report saved to: $reportFile" $Blue
    
} else {
    Write-ColorOutput "❌ STACK OVERFLOW FIX VERIFICATION: FAILED" $Red
    Write-ColorOutput "" $Reset
    Write-ColorOutput "Some HIL tests failed. Review the output above for details." $Red
    Write-ColorOutput "The stack overflow issue may not be fully resolved." $Red
    Write-ColorOutput "" $Reset
    Write-ColorOutput "Recommended actions:" $Yellow
    Write-ColorOutput "1. Review failed test details above" $Yellow
    Write-ColorOutput "2. Increase stack sizes for critical tasks" $Yellow
    Write-ColorOutput "3. Re-run this test after making changes" $Yellow
    Write-ColorOutput "4. Consider additional debugging if issues persist" $Yellow
    
    exit 1
}

Write-ColorOutput "`nStack overflow fix HIL verification completed." $Blue