#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Test FreeRTOS Stack Monitoring CLI Commands
    
.DESCRIPTION
    This script tests the newly implemented stack monitoring CLI commands
    to help debug the suspected stack overflow issue.
    
.PARAMETER ComPort
    Serial port to use for communication (default: COM3)
    
.PARAMETER TelnetIP
    IP address for telnet connection (default: 192.168.0.20)
    
.PARAMETER TelnetPort
    Port for telnet connection (default: 23)
    
.PARAMETER UseSerial
    Use serial connection instead of telnet
    
.EXAMPLE
    .\test_stack_monitoring.ps1
    .\test_stack_monitoring.ps1 -UseSerial
    .\test_stack_monitoring.ps1 -ComPort COM4
#>

param(
    [string]$ComPort = "COM3",
    [string]$TelnetIP = "192.168.0.20",
    [int]$TelnetPort = 23,
    [switch]$UseSerial
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

function Test-CLIConnection {
    param($Connection)
    
    Write-ColorOutput "Testing CLI connection..." $Blue
    
    try {
        # Send carriage return to establish connection
        if ($UseSerial) {
            $Connection.Write([byte]0x0D)
        } else {
            $Connection.WriteLine("")
        }
        
        Start-Sleep -Milliseconds 500
        
        # Check for prompt
        $response = ""
        if ($UseSerial) {
            if ($Connection.BytesToRead -gt 0) {
                $response = $Connection.ReadExisting()
            }
        } else {
            if ($Connection.Available -gt 0) {
                $response = $Connection.ReadLine()
            }
        }
        
        if ($response -match '>') {
            Write-ColorOutput "✓ CLI connection established" $Green
            return $true
        } else {
            Write-ColorOutput "✗ No CLI prompt received" $Red
            return $false
        }
    } catch {
        Write-ColorOutput "✗ Connection test failed: $($_.Exception.Message)" $Red
        return $false
    }
}

function Send-CLICommand {
    param($Connection, [string]$Command)
    
    Write-ColorOutput "Sending command: $Command" $Blue
    
    try {
        if ($UseSerial) {
            $Connection.WriteLine($Command)
        } else {
            $Connection.WriteLine($Command)
        }
        
        Start-Sleep -Milliseconds 1000
        
        $response = ""
        if ($UseSerial) {
            if ($Connection.BytesToRead -gt 0) {
                $response = $Connection.ReadExisting()
            }
        } else {
            $timeout = 0
            while ($Connection.Available -eq 0 -and $timeout -lt 30) {
                Start-Sleep -Milliseconds 100
                $timeout++
            }
            
            if ($Connection.Available -gt 0) {
                $buffer = New-Object byte[] 4096
                $bytesRead = $Connection.GetStream().Read($buffer, 0, $buffer.Length)
                $response = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
            }
        }
        
        if ($response.Length -gt 0) {
            Write-ColorOutput "Response received:" $Green
            Write-Host $response
            return $response
        } else {
            Write-ColorOutput "✗ No response received for command: $Command" $Red
            return ""
        }
    } catch {
        Write-ColorOutput "✗ Command failed: $($_.Exception.Message)" $Red
        return ""
    }
}

function Test-StackMonitoringCommands {
    param($Connection)
    
    Write-ColorOutput "`n=== Testing Stack Monitoring Commands ===" $Yellow
    
    $commands = @(
        "help",
        "task-stats", 
        "stack-info",
        "stack-check",
        "heap-info",
        "memory-info",
        "stack-overflow-info"
    )
    
    $results = @{}
    
    foreach ($cmd in $commands) {
        Write-ColorOutput "`nTesting command: $cmd" $Blue
        $response = Send-CLICommand -Connection $Connection -Command $cmd
        
        if ($response.Length -gt 0) {
            $results[$cmd] = "PASS"
            Write-ColorOutput "✓ $cmd - PASSED" $Green
        } else {
            $results[$cmd] = "FAIL"
            Write-ColorOutput "✗ $cmd - FAILED" $Red
        }
    }
    
    return $results
}

function Test-StackWatchCommand {
    param($Connection)
    
    Write-ColorOutput "`n=== Testing Stack Watch Command ===" $Yellow
    
    # First get task list to find a task name
    Write-ColorOutput "Getting task list..." $Blue
    $taskResponse = Send-CLICommand -Connection $Connection -Command "task-stats"
    
    if ($taskResponse -match "(\w+)\s+\w+\s+\d+\s+\d+") {
        $taskName = $matches[1]
        Write-ColorOutput "Found task: $taskName" $Green
        
        # Test stack-watch with specific task
        Write-ColorOutput "Testing stack-watch with task: $taskName" $Blue
        $watchResponse = Send-CLICommand -Connection $Connection -Command "stack-watch $taskName"
        
        if ($watchResponse.Length -gt 0) {
            Write-ColorOutput "✓ stack-watch with task name - PASSED" $Green
            return $true
        } else {
            Write-ColorOutput "✗ stack-watch with task name - FAILED" $Red
            return $false
        }
    } else {
        Write-ColorOutput "Could not extract task name from task-stats output" $Yellow
        return $false
    }
}

function Analyze-StackUsage {
    param($Connection)
    
    Write-ColorOutput "`n=== Analyzing Stack Usage for Overflow Debug ===" $Yellow
    
    # Get comprehensive stack information
    $stackInfo = Send-CLICommand -Connection $Connection -Command "stack-info"
    $stackCheck = Send-CLICommand -Connection $Connection -Command "stack-check"
    $memoryInfo = Send-CLICommand -Connection $Connection -Command "memory-info"
    $overflowInfo = Send-CLICommand -Connection $Connection -Command "stack-overflow-info"
    
    Write-ColorOutput "`n=== STACK OVERFLOW ANALYSIS REPORT ===" $Yellow
    Write-ColorOutput "=====================================" $Yellow
    
    # Check for critical stack usage
    if ($stackCheck -match "CRITICAL") {
        Write-ColorOutput "⚠️  CRITICAL STACK USAGE DETECTED!" $Red
        Write-ColorOutput "Tasks with critical stack usage found." $Red
        Write-ColorOutput "This is likely the cause of the hardware freeze." $Red
    } elseif ($stackCheck -match "WARNING") {
        Write-ColorOutput "⚠️  WARNING: High stack usage detected" $Yellow
        Write-ColorOutput "Some tasks are approaching stack limits." $Yellow
    } else {
        Write-ColorOutput "✓ No critical stack usage detected" $Green
    }
    
    # Check heap usage
    if ($memoryInfo -match "CRITICAL") {
        Write-ColorOutput "⚠️  CRITICAL HEAP USAGE DETECTED!" $Red
        Write-ColorOutput "Heap exhaustion may be contributing to issues." $Red
    } elseif ($memoryInfo -match "WARNING") {
        Write-ColorOutput "⚠️  WARNING: High heap usage detected" $Yellow
    } else {
        Write-ColorOutput "✓ Heap usage appears healthy" $Green
    }
    
    # Check overflow history
    if ($overflowInfo -match "OVERFLOW DETECTED") {
        Write-ColorOutput "🚨 STACK OVERFLOW PREVIOUSLY DETECTED!" $Red
        Write-ColorOutput "System has experienced stack overflows in the past." $Red
    } else {
        Write-ColorOutput "✓ No previous stack overflows detected" $Green
    }
    
    Write-ColorOutput "`nRecommendations:" $Blue
    Write-ColorOutput "1. Review tasks with >75% stack usage" $Blue
    Write-ColorOutput "2. Increase stack size for critical tasks" $Blue
    Write-ColorOutput "3. Monitor heap fragmentation" $Blue
    Write-ColorOutput "4. Enable stack overflow detection hooks" $Blue
}

# Main execution
Write-ColorOutput "FreeRTOS Stack Monitoring Test Script" $Blue
Write-ColorOutput "====================================" $Blue

try {
    if ($UseSerial) {
        Write-ColorOutput "Connecting via Serial ($ComPort)..." $Blue
        $port = New-Object System.IO.Ports.SerialPort $ComPort, 115200, None, 8, One
        $port.ReadTimeout = 3000
        $port.WriteTimeout = 3000
        $port.Open()
        $connection = $port
    } else {
        Write-ColorOutput "Connecting via Telnet ($TelnetIP`:$TelnetPort)..." $Blue
        $tcpClient = New-Object System.Net.Sockets.TcpClient
        $tcpClient.Connect($TelnetIP, $TelnetPort)
        $connection = $tcpClient
    }
    
    if (Test-CLIConnection -Connection $connection) {
        # Test all stack monitoring commands
        $results = Test-StackMonitoringCommands -Connection $connection
        
        # Test stack-watch with specific task
        Test-StackWatchCommand -Connection $connection
        
        # Perform stack overflow analysis
        Analyze-StackUsage -Connection $connection
        
        # Summary
        Write-ColorOutput "`n=== TEST SUMMARY ===" $Yellow
        $passCount = ($results.Values | Where-Object { $_ -eq "PASS" }).Count
        $totalCount = $results.Count
        
        Write-ColorOutput "Commands tested: $totalCount" $Blue
        Write-ColorOutput "Commands passed: $passCount" $Green
        Write-ColorOutput "Commands failed: $($totalCount - $passCount)" $Red
        
        if ($passCount -eq $totalCount) {
            Write-ColorOutput "`n✅ ALL STACK MONITORING TESTS PASSED!" $Green
            Write-ColorOutput "Stack monitoring tools are working correctly." $Green
            Write-ColorOutput "Use the analysis above to debug the stack overflow issue." $Green
        } else {
            Write-ColorOutput "`n❌ SOME TESTS FAILED" $Red
            Write-ColorOutput "Check the output above for details." $Red
        }
    } else {
        Write-ColorOutput "Failed to establish CLI connection" $Red
        exit 1
    }
    
} catch {
    Write-ColorOutput "Error: $($_.Exception.Message)" $Red
    exit 1
} finally {
    if ($connection) {
        if ($UseSerial) {
            $connection.Close()
        } else {
            $connection.Close()
        }
        Write-ColorOutput "Connection closed." $Blue
    }
}

Write-ColorOutput "`nStack monitoring test completed." $Blue