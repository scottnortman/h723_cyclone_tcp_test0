# Test CLI Help Command
param(
    [string]$ComPort = "COM3",
    [int]$BaudRate = 115200
)

Write-Host "Testing CLI Help Command" -ForegroundColor Cyan
Write-Host "========================" -ForegroundColor Cyan

try {
    $port = New-Object System.IO.Ports.SerialPort
    $port.PortName = $ComPort
    $port.BaudRate = $BaudRate
    $port.DataBits = 8
    $port.Parity = [System.IO.Ports.Parity]::None
    $port.StopBits = [System.IO.Ports.StopBits]::One
    $port.ReadTimeout = 5000
    $port.WriteTimeout = 3000
    
    $port.Open()
    
    if ($port.IsOpen) {
        Write-Host "Connected to $ComPort" -ForegroundColor Green
        
        # Clear buffer
        Start-Sleep -Milliseconds 500
        if ($port.BytesToRead -gt 0) {
            $port.ReadExisting() | Out-Null
        }
        
        # Establish CLI connection
        $port.Write([byte]0x0D)
        Start-Sleep -Milliseconds 300
        
        # Send help command
        Write-Host "Sending 'help' command..." -ForegroundColor Yellow
        $port.WriteLine("help")
        Start-Sleep -Milliseconds 2000
        
        if ($port.BytesToRead -gt 0) {
            $response = $port.ReadExisting()
            Write-Host "Available commands:" -ForegroundColor Green
            Write-Host $response
        } else {
            Write-Host "No response to help command" -ForegroundColor Red
        }
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