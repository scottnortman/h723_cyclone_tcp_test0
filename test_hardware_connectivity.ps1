#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Test Hardware Connectivity
.DESCRIPTION
    Simple test to check if hardware is responding after programming
#>

param(
    [string]$ComPort = "COM3",
    [int]$BaudRate = 115200,
    [int]$TimeoutSeconds = 10
)

Write-Host "=== Hardware Connectivity Test ===" -ForegroundColor Cyan
Write-Host "Testing basic hardware communication" -ForegroundColor White
Write-Host ""

# Load .NET serial port assembly
Add-Type -AssemblyName System.IO.Ports

Write-Host "Configuration:" -ForegroundColor White
Write-Host "  Serial Port: $ComPort" -ForegroundColor Gray
Write-Host "  Baud Rate: $BaudRate" -ForegroundColor Gray
Write-Host "  Timeout: $TimeoutSeconds seconds" -ForegroundColor Gray
Write-Host ""

try {
    Write-Host "Step 1: Opening serial connection..." -ForegroundColor Cyan
    
    $serialPort = New-Object System.IO.Ports.SerialPort
    $serialPort.PortName = $ComPort
    $serialPort.BaudRate = $BaudRate
    $serialPort.Parity = [System.IO.Ports.Parity]::None
    $serialPort.DataBits = 8
    $serialPort.StopBits = [System.IO.Ports.StopBits]::One
    $serialPort.Handshake = [System.IO.Ports.Handshake]::None
    $serialPort.ReadTimeout = $TimeoutSeconds * 1000
    $serialPort.WriteTimeout = $TimeoutSeconds * 1000
    
    $serialPort.Open()
    
    if ($serialPort.IsOpen) {
        Write-Host "  ✅ Serial port opened successfully" -ForegroundColor Green
    } else {
        throw "Failed to open serial port"
    }
    
    Write-Host ""
    Write-Host "Step 2: Testing basic communication..." -ForegroundColor Cyan
    
    # Clear any existing data
    $serialPort.DiscardInBuffer()
    $serialPort.DiscardOutBuffer()
    
    # Wait a moment for system to be ready
    Write-Host "  Waiting 3 seconds for system initialization..." -ForegroundColor Gray
    Start-Sleep -Seconds 3
    
    # Try sending a simple command
    Write-Host "  Sending test command..." -ForegroundColor Gray
    $serialPort.WriteLine("")  # Send empty line to get prompt
    Start-Sleep -Milliseconds 500
    
    $serialPort.WriteLine("help")
    Start-Sleep -Milliseconds 1000
    
    # Try to read response
    $response = ""
    $attempts = 0
    $maxAttempts = 10
    
    while ($attempts -lt $maxAttempts) {
        if ($serialPort.BytesToRead -gt 0) {
            $data = $serialPort.ReadExisting()
            $response += $data
            Write-Host "  Received data: $($data.Length) bytes" -ForegroundColor Gray
        }
        
        Start-Sleep -Milliseconds 500
        $attempts++
        
        # Check if we have some response
        if ($response.Length -gt 10) {
            break
        }
    }
    
    Write-Host ""
    Write-Host "Step 3: Analyzing response..." -ForegroundColor Cyan
    
    if ($response.Length -gt 0) {
        Write-Host "  ✅ Hardware is responding" -ForegroundColor Green
        Write-Host "  Response length: $($response.Length) characters" -ForegroundColor Gray
        
        # Show first part of response
        $preview = if ($response.Length -gt 100) { 
            $response.Substring(0, 100) + "..." 
        } else { 
            $response 
        }
        Write-Host "  Response preview: $preview" -ForegroundColor DarkGray
        
        # Check for CLI prompt or commands
        if ($response -match ">" -or $response -like "*help*" -or $response -like "*command*") {
            Write-Host "  ✅ CLI appears to be working" -ForegroundColor Green
            $cliWorking = $true
        } else {
            Write-Host "  ⚠️  CLI may not be fully initialized" -ForegroundColor Yellow
            $cliWorking = $false
        }
    } else {
        Write-Host "  ❌ No response from hardware" -ForegroundColor Red
        Write-Host "  Check connections and power" -ForegroundColor Yellow
        $cliWorking = $false
    }
    
} catch {
    Write-Host "  ❌ Connection failed: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host ""
    Write-Host "Troubleshooting:" -ForegroundColor Yellow
    Write-Host "  - Check that hardware is connected to $ComPort" -ForegroundColor Yellow
    Write-Host "  - Verify no other applications are using the port" -ForegroundColor Yellow
    Write-Host "  - Ensure hardware is powered and running" -ForegroundColor Yellow
    Write-Host "  - Try different COM port if needed" -ForegroundColor Yellow
    $cliWorking = $false
    
} finally {
    if ($serialPort -and $serialPort.IsOpen) {
        Write-Host ""
        Write-Host "Closing serial connection..." -ForegroundColor Gray
        $serialPort.Close()
    }
    if ($serialPort) {
        $serialPort.Dispose()
    }
}

Write-Host ""
Write-Host "=== Connectivity Test Results ===" -ForegroundColor Cyan

if ($cliWorking) {
    Write-Host "✅ Hardware connectivity: WORKING" -ForegroundColor Green
    Write-Host "✅ CLI system: RESPONDING" -ForegroundColor Green
    Write-Host "✅ Ready for CLI buffer tests" -ForegroundColor Green
    Write-Host ""
    Write-Host "Next step: Run CLI buffer test" -ForegroundColor White
    Write-Host "  .\test_cli_buffer_serial.ps1 -ComPort $ComPort" -ForegroundColor Gray
    $exitCode = 0
} else {
    Write-Host "❌ Hardware connectivity: FAILED" -ForegroundColor Red
    Write-Host "❌ CLI system: NOT RESPONDING" -ForegroundColor Red
    Write-Host "🔧 Troubleshooting required" -ForegroundColor Yellow
    $exitCode = 1
}

Write-Host ""
Write-Host "=== Test Complete ===" -ForegroundColor Cyan

exit $exitCode