#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Telnet Negotiation Test
.DESCRIPTION
    Handle telnet protocol negotiation like PuTTY does
#>

Write-Host "=== Telnet Negotiation Test ===" -ForegroundColor Blue

# Telnet protocol constants
$IAC = 255    # Interpret As Command
$WILL = 251   # Will option
$WONT = 252   # Won't option  
$DO = 253     # Do option
$DONT = 254   # Don't option
$ECHO = 1     # Echo option
$SGA = 3      # Suppress Go Ahead

try {
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    Write-Host "✅ Connected" -ForegroundColor Green
    
    $buffer = New-Object byte[] 1024
    
    # Wait for server to send telnet options
    Write-Host "`nWaiting for server telnet negotiation..." -ForegroundColor Yellow
    Start-Sleep -Seconds 1
    
    if ($stream.DataAvailable) {
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        Write-Host "Server sent $bytesRead bytes:" -ForegroundColor Cyan
        
        for ($i = 0; $i -lt $bytesRead; $i++) {
            $byte = $buffer[$i]
            if ($byte -eq $IAC) {
                Write-Host "  IAC (255)" -ForegroundColor Yellow
            } elseif ($byte -eq $WILL) {
                Write-Host "  WILL (251)" -ForegroundColor Yellow
            } elseif ($byte -eq $WONT) {
                Write-Host "  WONT (252)" -ForegroundColor Yellow
            } elseif ($byte -eq $DO) {
                Write-Host "  DO (253)" -ForegroundColor Yellow
            } elseif ($byte -eq $DONT) {
                Write-Host "  DONT (254)" -ForegroundColor Yellow
            } else {
                Write-Host "  $byte (0x$($byte.ToString('X2'))) $([char]$byte)" -ForegroundColor Gray
            }
        }
        
        # Respond to any telnet negotiations
        Write-Host "`nSending telnet responses..." -ForegroundColor Yellow
        
        # Standard responses: DON'T ECHO, DON'T SGA
        $response = @($IAC, $DONT, $ECHO, $IAC, $DONT, $SGA)
        $stream.Write($response, 0, $response.Length)
        $stream.Flush()
        Write-Host "Sent: IAC DONT ECHO, IAC DONT SGA" -ForegroundColor Cyan
        
    } else {
        Write-Host "No telnet negotiation from server" -ForegroundColor Gray
        
        # Send our own negotiation
        Write-Host "Sending client telnet negotiation..." -ForegroundColor Yellow
        $clientNeg = @($IAC, $WONT, $ECHO, $IAC, $WONT, $SGA)
        $stream.Write($clientNeg, 0, $clientNeg.Length)
        $stream.Flush()
        Write-Host "Sent: IAC WONT ECHO, IAC WONT SGA" -ForegroundColor Cyan
    }
    
    # Wait a bit more
    Start-Sleep -Seconds 1
    
    # Now try normal CLI interaction
    Write-Host "`n--- Normal CLI Test After Negotiation ---" -ForegroundColor Yellow
    
    # Send CR
    $stream.WriteByte([byte]0x0D)
    $stream.Flush()
    Write-Host "Sent CR" -ForegroundColor Cyan
    
    Start-Sleep -Milliseconds 500
    
    if ($stream.DataAvailable) {
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        $response = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
        Write-Host "Response to CR: '$response'" -ForegroundColor Green
    } else {
        Write-Host "No response to CR" -ForegroundColor Red
    }
    
    # Try help command
    Write-Host "`nSending 'help' command..." -ForegroundColor Yellow
    $helpBytes = [System.Text.Encoding]::ASCII.GetBytes("help`r")
    $stream.Write($helpBytes, 0, $helpBytes.Length)
    $stream.Flush()
    
    Start-Sleep -Seconds 2
    
    $helpResponse = ""
    while ($stream.DataAvailable) {
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        $chunk = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
        $helpResponse += $chunk
        Start-Sleep -Milliseconds 100
    }
    
    Write-Host "Help response length: $($helpResponse.Length)" -ForegroundColor Cyan
    if ($helpResponse.Length -gt 0) {
        Write-Host "✅ Help command worked!" -ForegroundColor Green
        Write-Host $helpResponse.Substring(0, [Math]::Min(200, $helpResponse.Length)) -ForegroundColor Gray
    } else {
        Write-Host "❌ Help command failed" -ForegroundColor Red
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

Write-Host "`n=== Telnet Negotiation Test Complete ===" -ForegroundColor Blue