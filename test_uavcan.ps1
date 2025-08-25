# PowerShell script to test UAVCAN via telnet
$client = New-Object System.Net.Sockets.TcpClient
try {
    Write-Host "Connecting to STM32H723 at 192.168.0.20:23..."
    $client.Connect("192.168.0.20", 23)
    $stream = $client.GetStream()
    $writer = New-Object System.IO.StreamWriter($stream)
    $reader = New-Object System.IO.StreamReader($stream)
    
    # Wait for connection to establish
    Start-Sleep -Seconds 3
    
    # Read any initial output
    Write-Host "=== Initial Connection Output ==="
    while($stream.DataAvailable) {
        $line = $reader.ReadLine()
        if ($line) { Write-Host $line }
    }
    
    # Send uavcan-status command
    Write-Host "`n=== Sending 'uavcan-status' command ==="
    $writer.WriteLine("uavcan-status")
    $writer.Flush()
    Start-Sleep -Seconds 2
    
    # Read response
    while($stream.DataAvailable) {
        $line = $reader.ReadLine()
        if ($line) { Write-Host $line }
    }
    
    # Send uavcan-test command
    Write-Host "`n=== Sending 'uavcan-test' command ==="
    $writer.WriteLine("uavcan-test")
    $writer.Flush()
    Start-Sleep -Seconds 5
    
    # Read response
    while($stream.DataAvailable) {
        $line = $reader.ReadLine()
        if ($line) { Write-Host $line }
    }
    
    # Send help command to see available commands
    Write-Host "`n=== Sending 'help' command ==="
    $writer.WriteLine("help")
    $writer.Flush()
    Start-Sleep -Seconds 2
    
    # Read response
    while($stream.DataAvailable) {
        $line = $reader.ReadLine()
        if ($line) { Write-Host $line }
    }
    
} catch {
    Write-Host "Error: $($_.Exception.Message)"
} finally {
    if ($client.Connected) {
        $client.Close()
    }
    Write-Host "`nConnection closed."
}