#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Test Built-in vs Custom Commands via Telnet
.DESCRIPTION
    Test if only built-in FreeRTOS commands work via telnet
#>

Write-Host "=== Test Built-in vs Custom Commands via Telnet ===" -ForegroundColor Blue

try {
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.ReceiveTimeout = 15000
    $tcpClient.SendTimeout = 10000
    
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    Write-Host "✅ Telnet connected" -ForegroundColor Green
    
    Start-Sleep -Seconds 2
    
    # Handshake
    $buffer = New-Object byte[] 4096
    $crBytes = [System.Text.Encoding]::ASCII.GetBytes("`r")
    $stream.Write($crBytes, 0, $crBytes.Length)
    $stream.Flush()
    Start-Sleep -Milliseconds 500
    
    # Test different types of commands
    $commands = @(
        @{ Name = "help"; Type = "Built-in FreeRTOS" },
        @{ Name = "task-stats"; Type = "Built-in FreeRTOS" },
        @{ Name = "echo-parameters test"; Type = "Built-in FreeRTOS" },
        @{ Name = "heap-info"; Type = "Custom Stack Monitor" },
        @{ Name = "stack-info"; Type = "Custom Stack Monitor" },
        @{ Name = "uavcan-status"; Type = "Custom UAVCAN" }
    )
    
    foreach ($cmd in $commands) {
        Write-Host "`nTesting '$($cmd.Name)' ($($cmd.Type))..." -ForegroundColor Yellow
        
        # Send command
        $cmdBytes = [System.Text.Encoding]::ASCII.GetBytes("$($cmd.Name)`r")
        $stream.Write($cmdBytes, 0, $cmdBytes.Length)
        $stream.Flush()
        
        # Wait for response
        Start-Sleep -Seconds 3
        
        $response = ""
        $attempts = 0
        while ($attempts -lt 15) {
            if ($stream.DataAvailable) {
                $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
                $newData = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
                $response += $newData
            }
            Start-Sleep -Milliseconds 200
            $attempts++
        }
        
        if ($response.Length -gt 10) {
            Write-Host "  ✅ SUCCESS - $($response.Length) characters" -ForegroundColor Green
        } else {
            Write-Host "  ❌ FAILED - $($response.Length) characters" -ForegroundColor Red
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

Write-Host "`n=== Built-in vs Custom Commands Test Complete ===" -ForegroundColor Blue