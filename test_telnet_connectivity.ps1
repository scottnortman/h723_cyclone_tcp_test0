#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Telnet Connectivity Test
.DESCRIPTION
    Test telnet connectivity and basic command execution
#>

Write-Host "=== Telnet Connectivity Test ===" -ForegroundColor Blue

try {
    # Test basic telnet connectivity
    Write-Host "Testing telnet connection to 192.168.0.20:23..." -ForegroundColor Yellow
    
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.ReceiveTimeout = 5000
    $tcpClient.SendTimeout = 3000
    
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    
    Write-Host "✅ Telnet connection established" -ForegroundColor Green
    
    # Wait for any initial data and clear it
    Start-Sleep -Milliseconds 1000
    $buffer = New-Object byte[] 1024
    while ($stream.DataAvailable) {
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        $initialData = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
        Write-Host "Initial data: $($initialData.Length) bytes" -ForegroundColor Gray
    }
    
    # Send handshake
    Write-Host "Sending handshake..." -ForegroundColor Yellow
    $crBytes = [System.Text.Encoding]::ASCII.GetBytes("`r")
    $stream.Write($crBytes, 0, $crBytes.Length)
    $stream.Flush()
    
    # Wait for prompt
    Start-Sleep -Milliseconds 500
    $promptReceived = $false
    $attempts = 0
    
    while ($attempts -lt 10 -and -not $promptReceived) {
        if ($stream.DataAvailable) {
            $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
            $response = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
            Write-Host "Handshake response: '$response'" -ForegroundColor Gray
            
            if ($response -match ">") {
                $promptReceived = $true
                Write-Host "✅ CLI prompt received" -ForegroundColor Green
            }
        }
        Start-Sleep -Milliseconds 100
        $attempts++
    }
    
    if (-not $promptReceived) {
        Write-Host "⚠️  No prompt received, trying command anyway" -ForegroundColor Yellow
    }
    
    # Test a simple command
    Write-Host "Testing 'help' command..." -ForegroundColor Yellow
    $cmdBytes = [System.Text.Encoding]::ASCII.GetBytes("help`r")
    $stream.Write($cmdBytes, 0, $cmdBytes.Length)
    $stream.Flush()
    
    # Wait for response
    Start-Sleep -Milliseconds 2000
    $response = ""
    $attempts = 0
    
    while ($attempts -lt 10) {
        if ($stream.DataAvailable) {
            $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
            $chunk = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
            $response += $chunk
        }
        Start-Sleep -Milliseconds 200
        $attempts++
    }
    
    Write-Host "Help command response length: $($response.Length) characters" -ForegroundColor Cyan
    if ($response.Length -gt 50) {
        Write-Host "✅ Telnet command execution working" -ForegroundColor Green
        Write-Host "Response preview:" -ForegroundColor Gray
        Write-Host ($response.Substring(0, [Math]::Min(200, $response.Length))) -ForegroundColor White
    } else {
        Write-Host "❌ Telnet command execution failed" -ForegroundColor Red
        Write-Host "Full response: '$response'" -ForegroundColor Red
    }
    
} catch {
    Write-Host "❌ Telnet test failed: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    if ($stream) {
        $stream.Close()
        $stream.Dispose()
    }
    if ($tcpClient) {
        $tcpClient.Close()
        $tcpClient.Dispose()
    }
    Start-Sleep -Milliseconds 500
    Write-Host "Telnet connection closed" -ForegroundColor Blue
}

Write-Host "Telnet connectivity test completed" -ForegroundColor Blue