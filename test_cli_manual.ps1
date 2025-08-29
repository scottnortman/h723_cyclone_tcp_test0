# Manual CLI Test Script
# Simple test to verify CLI buffer fix is working

param(
    [string]$ComPort = "COM3",
    [int]$BaudRate = 115200
)

Write-Host "Manual CLI Test - Testing CLI Buffer Fix" -ForegroundColor Cyan
Write-Host "=======================================" -ForegroundColor Cyan
Write-Host "Port: $ComPort, Baud: $BaudRate"
Write-Host ""

try {
    # Create and configure serial port
    $port = New-Object System.IO.Ports.SerialPort
    $port.PortName = $ComPort
    $port.BaudRate = $BaudRate
    $port.DataBits = 8
    $port.Parity = [System.IO.Ports.Parity]::None
    $port.StopBits = [System.IO.Ports.StopBits]::One
    $port.ReadTimeout = 3000
    $port.WriteTimeout = 3000
    
    Write-Host "Opening serial port..." -ForegroundColor Yellow
    $port.Open()
    
    if ($port.IsOpen) {
        Write-Host "Serial port opened successfully!" -ForegroundColor Green
        
        # Clear any existing data
        Start-Sleep -Milliseconds 500
        if ($port.BytesToRead -gt 0) {
            $port.ReadExisting() | Out-Null
        }
        
        # Send carriage return to establish CLI connection
        Write-Host "Establishing CLI connection..." -ForegroundColor Yellow
        $port.Write([byte]0x0D)
        Start-Sleep -Milliseconds 200
        
        # Check for prompt
        if ($port.BytesToRead -gt 0) {
            $response = $port.ReadExisting()
            Write-Host "Initial response: '$response'" -ForegroundColor Cyan
        }
        
        # Test CLI buffer command
        Write-Host "Testing CLI buffer command..." -ForegroundColor Yellow
        $port.WriteLine("uavcan-test-buffer")
        Start-Sleep -Milliseconds 1000
        
        if ($port.BytesToRead -gt 0) {
            $response = $port.ReadExisting()
            Write-Host "Buffer test response length: $($response.Length) characters" -ForegroundColor Green
            Write-Host "Response:" -ForegroundColor White
            Write-Host $response
            
            # Check for end marker
            if ($response -match "END_MARKER: Buffer test completed successfully") {
                Write-Host "SUCCESS: End marker found - buffer fix is working!" -ForegroundColor Green
            } else {
                Write-Host "WARNING: End marker not found - possible truncation" -ForegroundColor Yellow
            }
        } else {
            Write-Host "No response received" -ForegroundColor Red
        }
        
        # Test another command
        Write-Host "`nTesting status command..." -ForegroundColor Yellow
        $port.WriteLine("uavcan-status")
        Start-Sleep -Milliseconds 500
        
        if ($port.BytesToRead -gt 0) {
            $response = $port.ReadExisting()
            Write-Host "Status response length: $($response.Length) characters" -ForegroundColor Green
            Write-Host "Response:" -ForegroundColor White
            Write-Host $response
        }
        
    } else {
        Write-Host "Failed to open serial port" -ForegroundColor Red
    }
}
catch {
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
}
finally {
    if ($port -and $port.IsOpen) {
        $port.Close()
        $port.Dispose()
    }
}

Write-Host "`nTest completed." -ForegroundColor Cyan