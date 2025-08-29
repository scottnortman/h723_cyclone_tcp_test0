#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Mimic PuTTY Telnet Protocol
.DESCRIPTION
    Replicate exactly how PuTTY connects and communicates
#>

Write-Host "=== Mimic PuTTY Telnet Protocol ===" -ForegroundColor Blue

try {
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.ReceiveTimeout = 15000
    $tcpClient.SendTimeout = 10000
    
    Write-Host "Connecting like PuTTY..." -ForegroundColor Gray
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    Write-Host "✅ Connected" -ForegroundColor Green
    
    $buffer = New-Object byte[] 4096
    
    # Step 1: Handle any telnet negotiation (like PuTTY does)
    Write-Host "`nStep 1: Handle telnet negotiation..." -ForegroundColor Yellow
    Start-Sleep -Seconds 2
    
    if ($stream.DataAvailable) {
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        $negotiation = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
        Write-Host "Negotiation data: '$negotiation'" -ForegroundColor Cyan
        
        # PuTTY typically responds to telnet options
        # For now, just acknowledge we received something
    }
    
    # Step 2: Send initial data like PuTTY (often sends CR+LF)
    Write-Host "`nStep 2: Send initial handshake like PuTTY..." -ForegroundColor Yellow
    
    # PuTTY often sends CR+LF for initial handshake
    $handshakeBytes = [System.Text.Encoding]::ASCII.GetBytes("`r`n")
    $stream.Write($handshakeBytes, 0, $handshakeBytes.Length)
    $stream.Flush()
    
    # Wait for response
    Start-Sleep -Seconds 2
    
    $response = ""
    if ($stream.DataAvailable) {
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        $response = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
        Write-Host "Handshake response: '$response'" -ForegroundColor Cyan
    }
    
    # Step 3: Try different handshake approaches
    if (-not ($response -match '>')) {
        Write-Host "`nStep 3: Try alternative handshakes..." -ForegroundColor Yellow
        
        $handshakes = @("`r", "`n", "`r`n", "")
        
        foreach ($hs in $handshakes) {
            Write-Host "Trying handshake: '$($hs.Replace("`r", "\\r").Replace("`n", "\\n"))'" -ForegroundColor Gray
            
            if ($hs -ne "") {
                $hsBytes = [System.Text.Encoding]::ASCII.GetBytes($hs)
                $stream.Write($hsBytes, 0, $hsBytes.Length)
                $stream.Flush()
            }
            
            Start-Sleep -Seconds 1
            
            if ($stream.DataAvailable) {
                $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
                $hsResponse = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
                Write-Host "  Response: '$hsResponse'" -ForegroundColor Cyan
                
                if ($hsResponse -match '>') {
                    Write-Host "  ✅ Got prompt!" -ForegroundColor Green
                    $response = $hsResponse
                    break
                }
            }
        }
    }
    
    # Step 4: If we have a prompt, try a command
    if ($response -match '>') {
        Write-Host "`nStep 4: Testing command..." -ForegroundColor Yellow
        
        # Send help command like PuTTY would
        $cmdBytes = [System.Text.Encoding]::ASCII.GetBytes("help`r")
        $stream.Write($cmdBytes, 0, $cmdBytes.Length)
        $stream.Flush()
        
        # Wait for response like PuTTY
        Start-Sleep -Seconds 3
        
        $cmdResponse = ""
        $attempts = 0
        while ($attempts -lt 20) {
            if ($stream.DataAvailable) {
                $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
                $newData = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
                $cmdResponse += $newData
            }
            Start-Sleep -Milliseconds 200
            $attempts++
        }
        
        Write-Host "Command response length: $($cmdResponse.Length)" -ForegroundColor Cyan
        
        if ($cmdResponse.Length -gt 100) {
            Write-Host "✅ Command worked! PuTTY protocol successful" -ForegroundColor Green
            
            # Show first few lines
            $lines = ($cmdResponse -split "`n") | Select-Object -First 5
            foreach ($line in $lines) {
                Write-Host "  $($line.Trim())" -ForegroundColor Gray
            }
        } else {
            Write-Host "❌ Command failed" -ForegroundColor Red
            Write-Host "Raw response: '$cmdResponse'" -ForegroundColor DarkGray
        }
    } else {
        Write-Host "❌ No prompt received - cannot test commands" -ForegroundColor Red
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

Write-Host "`n=== PuTTY Protocol Test Complete ===" -ForegroundColor Blue