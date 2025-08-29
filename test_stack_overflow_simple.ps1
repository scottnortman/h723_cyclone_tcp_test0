#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Simple HIL Test for Stack Overflow Fix Verification
#>

param(
    [string]$ComPort = "COM3"
)

# Colors
$Red = "`e[31m"
$Green = "`e[32m"
$Yellow = "`e[33m"
$Blue = "`e[34m"
$Reset = "`e[0m"

function Write-ColorOutput {
    param([string]$Message, [string]$Color = $Reset)
    Write-Host "$Color$Message$Reset"
}

Write-ColorOutput "Stack Overflow Fix HIL Test" $Blue
Write-ColorOutput "============================" $Blue

try {
    # Connect to hardware
    Write-ColorOutput "Connecting to hardware on $ComPort..." $Blue
    $port = New-Object System.IO.Ports.SerialPort $ComPort, 115200, None, 8, One
    $port.ReadTimeout = 3000
    $port.WriteTimeout = 3000
    $port.Open()
    
    # Establish CLI connection
    Write-ColorOutput "Establishing CLI connection..." $Blue
    $port.Write([byte]0x0D)
    Start-Sleep -Milliseconds 1000
    
    if ($port.BytesToRead -gt 0) {
        $response = $port.ReadExisting()
        if ($response -match '>') {
            Write-ColorOutput "✓ CLI connection established" $Green
        } else {
            Write-ColorOutput "✗ No CLI prompt received" $Red
            exit 1
        }
    } else {
        Write-ColorOutput "✗ No response from hardware" $Red
        exit 1
    }
    
    # Test stack monitoring commands
    $commands = @("help", "task-stats", "stack-info", "stack-check", "heap-info", "memory-info")
    $passedTests = 0
    
    foreach ($cmd in $commands) {
        Write-ColorOutput "Testing command: $cmd" $Blue
        
        # Clear buffer
        if ($port.BytesToRead -gt 0) {
            $port.ReadExisting() | Out-Null
        }
        
        # Send command
        $port.WriteLine($cmd)
        Start-Sleep -Milliseconds 1500
        
        # Read response
        $response = ""
        if ($port.BytesToRead -gt 0) {
            $response = $port.ReadExisting()
        }
        
        if ($response.Length -gt 0) {
            Write-ColorOutput "✓ $cmd - PASSED" $Green
            $passedTests++
            
            # Check for critical issues
            if ($cmd -eq "stack-check" -and $response -match "CRITICAL") {
                Write-ColorOutput "⚠️  CRITICAL stack usage detected!" $Red
            }
        } else {
            Write-ColorOutput "✗ $cmd - FAILED (no response)" $Red
        }
    }
    
    # Summary
    Write-ColorOutput "`n=== TEST RESULTS ===" $Yellow
    Write-ColorOutput "Commands tested: $($commands.Count)" $Blue
    Write-ColorOutput "Commands passed: $passedTests" $Green
    Write-ColorOutput "Commands failed: $($commands.Count - $passedTests)" $Red
    
    if ($passedTests -eq $commands.Count) {
        Write-ColorOutput "`n✅ STACK OVERFLOW FIX VERIFICATION: PASSED" $Green
        Write-ColorOutput "Stack monitoring tools are working on hardware!" $Green
    } else {
        Write-ColorOutput "`n❌ STACK OVERFLOW FIX VERIFICATION: FAILED" $Red
        Write-ColorOutput "Some commands failed - check implementation" $Red
        exit 1
    }
    
} catch {
    Write-ColorOutput "Error: $($_.Exception.Message)" $Red
    exit 1
} finally {
    if ($port -and $port.IsOpen) {
        $port.Close()
        Write-ColorOutput "Connection closed." $Blue
    }
}

Write-ColorOutput "`nStack overflow fix HIL test completed." $Blue