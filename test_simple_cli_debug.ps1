#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Simple CLI Debug Test
.DESCRIPTION
    Tests basic CLI functionality to debug command issues
#>

Write-Host "=== Simple CLI Debug Test ===" -ForegroundColor Blue

try {
    Write-Host "Connecting to STM32H723 at 192.168.0.20:23..." -ForegroundColor Blue
    
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    
    # Wait and flush buffer
    Start-Sleep -Milliseconds 1000
    if ($tcpClient.Available -gt 0) {
        $buffer = New-Object byte[] $tcpClient.Available
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        $staleData = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
        Write-Host "Flushed stale data: $($staleData.Length) bytes" -ForegroundColor Gray
    }
    
    # Send handshake
    $stream.Write([byte]0x0D, 0, 1)
    Start-Sleep -Milliseconds 500
    
    # Check for prompt
    if ($tcpClient.Available -gt 0) {
        $buffer = New-Object byte[] 1024
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        $response = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
        Write-Host "Handshake response: '$response'" -ForegroundColor Yellow
    }
    
    # Test help command
    Write-Host "`nTesting 'help' command..." -ForegroundColor Blue
    $cmdBytes = [System.Text.Encoding]::ASCII.GetBytes("help`r`n")
    $stream.Write($cmdBytes, 0, $cmdBytes.Length)
    
    Start-Sleep -Milliseconds 2000
    
    if ($tcpClient.Available -gt 0) {
        $buffer = New-Object byte[] 4096
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        $response = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
        Write-Host "Help response ($($response.Length) chars):" -ForegroundColor Green
        Write-Host $response -ForegroundColor Gray
    } else {
        Write-Host "No response to help command" -ForegroundColor Red
    }
    
    # Test a specific UAVCAN command with longer timeout
    Write-Host "`nTesting 'uavcan-status' command with extended timeout..." -ForegroundColor Blue
    $cmdBytes = [System.Text.Encoding]::ASCII.GetBytes("uavcan-status`r`n")
    $stream.Write($cmdBytes, 0, $cmdBytes.Length)
    
    # Wait longer for response
    $timeout = [System.DateTime]::Now.AddSeconds(10)
    $response = ""
    
    while ([System.DateTime]::Now -lt $timeout) {
        if ($tcpClient.Available -gt 0) {
            $buffer = New-Object byte[] 4096
            $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
            $newData = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
            $response += $newData
            Write-Host "Received data chunk: '$newData'" -ForegroundColor Cyan
        }
        Start-Sleep -Milliseconds 100
    }
    
    if ($response.Length -gt 0) {
        Write-Host "UAVCAN status response:" -ForegroundColor Green
        Write-Host $response -ForegroundColor Gray
    } else {
        Write-Host "No response to uavcan-status command" -ForegroundColor Red
    }
    
} catch {
    Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    if ($tcpClient) {
        $tcpClient.Close()
        Write-Host "Connection closed" -ForegroundColor Blue
    }
}

Write-Host "Simple CLI debug test completed" -ForegroundColor Blue