#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Telnet Test with Long Wait Times
.DESCRIPTION
    Test if telnet commands need longer processing time
#>

Write-Host "=== Telnet Test with Long Wait Times ===" -ForegroundColor Blue

try {
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.ReceiveTimeout = 30000
    $tcpClient.SendTimeout = 15000
    
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    Write-Host "✅ Connected to telnet" -ForegroundColor Green
    
    Start-Sleep -Seconds 2
    
    # Handshake
    $buffer = New-Object byte[] 8192
    $crBytes = [System.Text.Encoding]::ASCII.GetBytes([char]13)
    $stream.Write($crBytes, 0, $crBytes.Length)
    $stream.Flush()
    Start-Sleep -Seconds 1
    
    # Test one command with very long wait
    Write-Host "`nTesting 'heap-info' with 10 second wait..." -ForegroundColor Yellow
    
    $cmdBytes = [System.Text.Encoding]::ASCII.GetBytes("heap-info`r")
    $stream.Write($cmdBytes, 0, $cmdBytes.Length)
    $stream.Flush()
    
    Write-Host "Command sent, waiting 10 seconds..." -ForegroundColor Gray
    
    # Wait much longer and check multiple times
    $response = ""
    for ($i = 1; $i -le 50; $i++) {
        Start-Sleep -Milliseconds 200
        
        if ($stream.DataAvailable) {
            $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
            $newData = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
            $response += $newData
            Write-Host "  Check $i`: Read $bytesRead bytes (total: $($response.Length))" -ForegroundColor Gray
        }
        
        # If we got a substantial response, break
        if ($response.Length -gt 100) {
            Write-Host "  Got substantial response, stopping wait" -ForegroundColor Green
            break
        }
    }
    
    Write-Host "`nFinal response length: $($response.Length) characters" -ForegroundColor Cyan
    
    if ($response.Length -gt 10) {
        Write-Host "✅ SUCCESS - Got response!" -ForegroundColor Green
        Write-Host "Response:" -ForegroundColor Cyan
        Write-Host $response -ForegroundColor Gray
    } else {
        Write-Host "❌ FAILED - No meaningful response" -ForegroundColor Red
        if ($response.Length -gt 0) {
            Write-Host "Raw response: '$response'" -ForegroundColor DarkGray
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

Write-Host "`n=== Long Wait Test Complete ===" -ForegroundColor Blue