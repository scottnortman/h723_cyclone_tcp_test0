#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Simple Telnet Test - Match Serial Timing
.DESCRIPTION
    Test telnet with exact same timing as working serial interface
#>

Write-Host "=== Simple Telnet Test - Match Serial Timing ===" -ForegroundColor Blue

try {
    # Connect to telnet with same approach as serial
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.ReceiveTimeout = 15000
    $tcpClient.SendTimeout = 10000
    
    Write-Host "Connecting to telnet..." -ForegroundColor Gray
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    Write-Host "✅ Connected" -ForegroundColor Green
    
    # Same timing as serial test
    Start-Sleep -Seconds 1
    
    # Flush any initial data (like serial test)
    $buffer = New-Object byte[] 4096
    while ($stream.DataAvailable) {
        $stream.Read($buffer, 0, $buffer.Length) | Out-Null
    }
    
    # Send carriage return (like serial test)
    $crBytes = [System.Text.Encoding]::ASCII.GetBytes([char]13)
    $stream.Write($crBytes, 0, $crBytes.Length)
    $stream.Flush()
    
    # Same wait time as serial test
    Start-Sleep -Milliseconds 300
    
    # Flush response (like serial test)
    if ($stream.DataAvailable) {
        $stream.Read($buffer, 0, $buffer.Length) | Out-Null
    }
    
    Write-Host "✅ Handshake completed" -ForegroundColor Green
    
    # Test commands with EXACT same timing as serial
    $commands = @("heap-info", "stack-info", "memory-info")
    
    foreach ($cmd in $commands) {
        Write-Host "`nTesting '$cmd'..." -ForegroundColor Yellow
        
        # Send command exactly like serial (WriteLine equivalent)
        $cmdBytes = [System.Text.Encoding]::ASCII.GetBytes("$cmd`r`n")
        $stream.Write($cmdBytes, 0, $cmdBytes.Length)
        $stream.Flush()
        
        # Wait EXACTLY same time as serial test (2000ms)
        Start-Sleep -Milliseconds 2000
        
        # Read response exactly like serial test
        $response = ""
        if ($stream.DataAvailable) {
            $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
            $response = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
        }
        
        Write-Host "Response length: $($response.Length) characters" -ForegroundColor Cyan
        
        if ($response.Length -gt 50) {
            Write-Host "✅ SUCCESS" -ForegroundColor Green
            # Show first few lines
            $lines = ($response -split "`n") | Select-Object -First 5
            foreach ($line in $lines) {
                Write-Host "  $line" -ForegroundColor Gray
            }
        } else {
            Write-Host "❌ FAILED" -ForegroundColor Red
            if ($response.Length -gt 0) {
                Write-Host "Raw: '$response'" -ForegroundColor DarkGray
            }
        }
    }
    
} catch {
    Write-Host "❌ Error: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    if ($stream) {
        $stream.Close()
        $stream.Dispose()
    }
    if ($tcpClient) {
        $tcpClient.Close()
        $tcpClient.Dispose()
    }
}

Write-Host "`n=== Simple Telnet Test Complete ===" -ForegroundColor Blue