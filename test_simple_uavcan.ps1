# Simple PowerShell script to test all UAVCAN requirements via telnet
$IPAddress = "192.168.0.20"
$Port = 23

$client = New-Object System.Net.Sockets.TcpClient

try {
    Write-Host "Connecting to STM32H723 at ${IPAddress}:${Port}..." -ForegroundColor Cyan
    $client.Connect($IPAddress, $Port)
    $stream = $client.GetStream()
    $writer = New-Object System.IO.StreamWriter($stream)
    $reader = New-Object System.IO.StreamReader($stream)
    
    # Wait for connection to establish
    Start-Sleep -Seconds 2
    
    # Clear any initial output
    Write-Host "=== Clearing initial output ===" -ForegroundColor Gray
    while($stream.DataAvailable) {
        $line = $reader.ReadLine()
        if ($line) { Write-Host $line -ForegroundColor Gray }
    }
    
    # Test commands one by one
    $commands = @("help", "uavcan-status", "uavcan-simple-verify", "uavcan-test", "uavcan-system-test", "uavcan-verify-requirements")
    
    foreach ($cmd in $commands) {
        Write-Host "`n=== Testing: $cmd ===" -ForegroundColor Yellow
        
        # Send command
        $writer.WriteLine($cmd)
        $writer.Flush()
        Start-Sleep -Seconds 4
        
        # Read response
        $hasOutput = $false
        while($stream.DataAvailable) {
            $line = $reader.ReadLine()
            if ($line) { 
                Write-Host $line -ForegroundColor Green
                $hasOutput = $true
            }
        }
        
        if (!$hasOutput) {
            Write-Host "No response received" -ForegroundColor Red
        }
    }
    
} catch {
    Write-Host "Connection Error: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    if ($client.Connected) {
        $client.Close()
    }
    Write-Host "`nConnection closed." -ForegroundColor Gray
}