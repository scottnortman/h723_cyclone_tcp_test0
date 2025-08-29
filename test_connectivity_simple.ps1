# Simple Hardware Connectivity Test
param(
    [string]$ComPort = "COM3",
    [int]$BaudRate = 115200
)

Write-Host "=== Hardware Connectivity Test ===" -ForegroundColor Cyan
Write-Host "Testing basic hardware communication" -ForegroundColor White
Write-Host ""

# Load .NET serial port assembly
Add-Type -AssemblyName System.IO.Ports

Write-Host "Configuration:" -ForegroundColor White
Write-Host "  Serial Port: $ComPort" -ForegroundColor Gray
Write-Host "  Baud Rate: $BaudRate" -ForegroundColor Gray
Write-Host ""

try {
    Write-Host "Opening serial connection..." -ForegroundColor Cyan
    
    $serialPort = New-Object System.IO.Ports.SerialPort
    $serialPort.PortName = $ComPort
    $serialPort.BaudRate = $BaudRate
    $serialPort.Parity = [System.IO.Ports.Parity]::None
    $serialPort.DataBits = 8
    $serialPort.StopBits = [System.IO.Ports.StopBits]::One
    $serialPort.ReadTimeout = 5000
    $serialPort.WriteTimeout = 5000
    
    $serialPort.Open()
    
    if ($serialPort.IsOpen) {
        Write-Host "Serial port opened successfully" -ForegroundColor Green
        
        # Wait for system boot
        Write-Host "Waiting for system initialization..." -ForegroundColor Gray
        Start-Sleep -Seconds 3
        
        # Clear buffers
        $serialPort.DiscardInBuffer()
        $serialPort.DiscardOutBuffer()
        
        # Send test command
        Write-Host "Sending test command..." -ForegroundColor Gray
        $serialPort.WriteLine("help")
        Start-Sleep -Seconds 2
        
        # Read response
        $response = ""
        if ($serialPort.BytesToRead -gt 0) {
            $response = $serialPort.ReadExisting()
        }
        
        if ($response.Length -gt 0) {
            Write-Host "Hardware is responding!" -ForegroundColor Green
            Write-Host "Response length: $($response.Length) characters" -ForegroundColor Gray
            
            # Show preview
            $preview = if ($response.Length -gt 100) { $response.Substring(0, 100) + "..." } else { $response }
            Write-Host "Preview: $preview" -ForegroundColor DarkGray
            
            $success = $true
        } else {
            Write-Host "No response from hardware" -ForegroundColor Red
            $success = $false
        }
        
    } else {
        Write-Host "Failed to open serial port" -ForegroundColor Red
        $success = $false
    }
    
} catch {
    Write-Host "Connection error: $($_.Exception.Message)" -ForegroundColor Red
    $success = $false
    
} finally {
    if ($serialPort -and $serialPort.IsOpen) {
        $serialPort.Close()
    }
    if ($serialPort) {
        $serialPort.Dispose()
    }
}

Write-Host ""
if ($success) {
    Write-Host "Connectivity Test: SUCCESS" -ForegroundColor Green
    Write-Host "Ready for CLI buffer tests" -ForegroundColor Green
} else {
    Write-Host "Connectivity Test: FAILED" -ForegroundColor Red
    Write-Host "Check hardware connection and power" -ForegroundColor Yellow
}

Write-Host "Test Complete" -ForegroundColor Cyan