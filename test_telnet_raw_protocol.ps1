#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Raw Telnet Protocol Analysis
.DESCRIPTION
    Capture and analyze the exact telnet protocol exchange
#>

Write-Host "=== Raw Telnet Protocol Analysis ===" -ForegroundColor Blue

try {
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    Write-Host "✅ Connected" -ForegroundColor Green
    
    $buffer = New-Object byte[] 1024
    
    # Step 1: Capture initial exchange
    Write-Host "`n--- Step 1: Initial Exchange ---" -ForegroundColor Yellow
    Start-Sleep -Milliseconds 500
    
    if ($stream.DataAvailable) {
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        Write-Host "Initial data received: $bytesRead bytes" -ForegroundColor Cyan
        for ($i = 0; $i -lt $bytesRead; $i++) {
            Write-Host "  Byte $i`: 0x$($buffer[$i].ToString('X2')) ($([char]$buffer[$i]))" -ForegroundColor Gray
        }
    } else {
        Write-Host "No initial data" -ForegroundColor Gray
    }
    
    # Step 2: Send just CR and see what happens
    Write-Host "`n--- Step 2: Send Carriage Return ---" -ForegroundColor Yellow
    $crByte = [byte]0x0D
    $stream.WriteByte($crByte)
    $stream.Flush()
    Write-Host "Sent: 0x0D (CR)" -ForegroundColor Cyan
    
    Start-Sleep -Milliseconds 1000
    
    if ($stream.DataAvailable) {
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        Write-Host "Response: $bytesRead bytes" -ForegroundColor Cyan
        $responseText = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
        Write-Host "Text: '$responseText'" -ForegroundColor Gray
        for ($i = 0; $i -lt $bytesRead; $i++) {
            Write-Host "  Byte $i`: 0x$($buffer[$i].ToString('X2')) ($([char]$buffer[$i]))" -ForegroundColor Gray
        }
    } else {
        Write-Host "No response to CR" -ForegroundColor Red
    }
    
    # Step 3: Send a simple command character by character
    Write-Host "`n--- Step 3: Send 'help' Character by Character ---" -ForegroundColor Yellow
    $helpCommand = "help"
    
    foreach ($char in $helpCommand.ToCharArray()) {
        $charByte = [byte][char]$char
        $stream.WriteByte($charByte)
        $stream.Flush()
        Write-Host "Sent: 0x$($charByte.ToString('X2')) ($char)" -ForegroundColor Cyan
        Start-Sleep -Milliseconds 100
        
        # Check for any immediate response
        if ($stream.DataAvailable) {
            $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
            Write-Host "  Immediate response: $bytesRead bytes" -ForegroundColor Yellow
        }
    }
    
    # Send CR to execute command
    Write-Host "Sending CR to execute command..." -ForegroundColor Cyan
    $stream.WriteByte([byte]0x0D)
    $stream.Flush()
    
    # Wait for full response
    Start-Sleep -Seconds 2
    
    $fullResponse = ""
    while ($stream.DataAvailable) {
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        $responseChunk = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
        $fullResponse += $responseChunk
        Write-Host "Read chunk: $bytesRead bytes" -ForegroundColor Gray
        Start-Sleep -Milliseconds 100
    }
    
    Write-Host "`nFull response length: $($fullResponse.Length)" -ForegroundColor Cyan
    if ($fullResponse.Length -gt 0) {
        Write-Host "Response text:" -ForegroundColor Green
        Write-Host $fullResponse -ForegroundColor Gray
    }
    
    # Step 4: Try heap-info the same way
    Write-Host "`n--- Step 4: Send 'heap-info' Character by Character ---" -ForegroundColor Yellow
    $heapCommand = "heap-info"
    
    foreach ($char in $heapCommand.ToCharArray()) {
        $charByte = [byte][char]$char
        $stream.WriteByte($charByte)
        $stream.Flush()
        Write-Host "Sent: 0x$($charByte.ToString('X2')) ($char)" -ForegroundColor Cyan
        Start-Sleep -Milliseconds 100
    }
    
    # Send CR to execute
    Write-Host "Sending CR to execute heap-info..." -ForegroundColor Cyan
    $stream.WriteByte([byte]0x0D)
    $stream.Flush()
    
    # Wait for response
    Start-Sleep -Seconds 3
    
    $heapResponse = ""
    while ($stream.DataAvailable) {
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        $responseChunk = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
        $heapResponse += $responseChunk
        Write-Host "Read chunk: $bytesRead bytes" -ForegroundColor Gray
        Start-Sleep -Milliseconds 100
    }
    
    Write-Host "`nHeap-info response length: $($heapResponse.Length)" -ForegroundColor Cyan
    if ($heapResponse.Length -gt 0) {
        Write-Host "Heap-info response:" -ForegroundColor Green
        Write-Host $heapResponse -ForegroundColor Gray
    } else {
        Write-Host "❌ No response to heap-info" -ForegroundColor Red
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

Write-Host "`n=== Raw Protocol Analysis Complete ===" -ForegroundColor Blue