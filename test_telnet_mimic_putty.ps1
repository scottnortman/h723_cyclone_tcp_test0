#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Mimic PuTTY Telnet Behavior
.DESCRIPTION
    Exactly replicate what PuTTY does when connecting via telnet
#>

Write-Host "=== Mimic PuTTY Telnet Behavior ===" -ForegroundColor Blue

try {
    # Step 1: Connect exactly like PuTTY
    Write-Host "`n--- Step 1: PuTTY-style Connection ---" -ForegroundColor Yellow
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.ReceiveTimeout = 30000  # Long timeout like PuTTY
    $tcpClient.SendTimeout = 30000
    
    Write-Host "Connecting to 192.168.0.20:23..." -ForegroundColor Gray
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    Write-Host "✅ Connected" -ForegroundColor Green
    
    # Step 2: Wait like PuTTY does after connection
    Write-Host "`n--- Step 2: Post-Connection Wait ---" -ForegroundColor Yellow
    Write-Host "Waiting 3 seconds (like PuTTY)..." -ForegroundColor Gray
    Start-Sleep -Seconds 3
    
    $buffer = New-Object byte[] 4096
    
    # Step 3: Check for any automatic banner (PuTTY would show this)
    Write-Host "`n--- Step 3: Check for Banner ---" -ForegroundColor Yellow
    if ($stream.DataAvailable) {
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        $banner = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
        Write-Host "Banner received: '$banner'" -ForegroundColor Cyan
    } else {
        Write-Host "No banner (normal for this firmware)" -ForegroundColor Gray
    }
    
    # Step 4: Send CR exactly like PuTTY (when user presses Enter)
    Write-Host "`n--- Step 4: Send Carriage Return (like pressing Enter in PuTTY) ---" -ForegroundColor Yellow
    Write-Host "Sending CR..." -ForegroundColor Gray
    
    # Send single carriage return (0x0D) like PuTTY
    $stream.WriteByte(0x0D)
    $stream.Flush()
    
    # Step 5: Wait for prompt response (like PuTTY would display)
    Write-Host "`n--- Step 5: Wait for Prompt Response ---" -ForegroundColor Yellow
    $response = ""
    $promptReceived = $false
    
    # Wait up to 10 seconds for response
    for ($i = 1; $i -le 40; $i++) {
        Start-Sleep -Milliseconds 250
        
        if ($stream.DataAvailable) {
            $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
            $newData = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
            $response += $newData
            
            Write-Host "  Attempt $i`: Received $bytesRead bytes: '$newData'" -ForegroundColor Gray
            
            # Check for prompt
            if ($newData -match '>' -or $response -match '>') {
                Write-Host "✅ PROMPT RECEIVED!" -ForegroundColor Green
                $promptReceived = $true
                break
            }
        }
    }
    
    Write-Host "`nTotal response: '$response'" -ForegroundColor Cyan
    Write-Host "Response length: $($response.Length) characters" -ForegroundColor Cyan
    
    if ($promptReceived) {
        Write-Host "🎉 SUCCESS - Telnet CLI is responding like PuTTY!" -ForegroundColor Green
        
        # Step 6: Try a simple command (like you would in PuTTY)
        Write-Host "`n--- Step 6: Test Simple Command ---" -ForegroundColor Yellow
        Write-Host "Sending 'help' command..." -ForegroundColor Gray
        
        $helpBytes = [System.Text.Encoding]::ASCII.GetBytes("help")
        $stream.Write($helpBytes, 0, $helpBytes.Length)
        $stream.WriteByte(0x0D)  # CR like PuTTY
        $stream.Flush()
        
        # Wait for help response
        Start-Sleep -Seconds 3
        $helpResponse = ""
        
        while ($stream.DataAvailable) {
            $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
            $newData = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
            $helpResponse += $newData
        }
        
        Write-Host "Help response length: $($helpResponse.Length) characters" -ForegroundColor Cyan
        if ($helpResponse.Length -gt 100) {
            Write-Host "✅ Help command worked!" -ForegroundColor Green
        } else {
            Write-Host "❌ Help command failed" -ForegroundColor Red
            Write-Host "Response: '$helpResponse'" -ForegroundColor DarkGray
        }
        
    } else {
        Write-Host "❌ FAILED - No prompt received (different from PuTTY behavior)" -ForegroundColor Red
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

Write-Host "`n=== PuTTY Mimic Test Complete ===" -ForegroundColor Blue