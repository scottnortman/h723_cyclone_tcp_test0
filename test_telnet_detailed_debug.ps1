#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Detailed Telnet Debug - Mimic PuTTY behavior
.DESCRIPTION
    Detailed debugging to replicate PuTTY's successful connection
#>

Write-Host "=== Detailed Telnet Debug - Mimic PuTTY ===" -ForegroundColor Blue

function Test-TelnetCommand {
    param(
        [string]$Command,
        [System.Net.Sockets.NetworkStream]$Stream,
        [byte[]]$Buffer
    )
    
    Write-Host "`n--- Testing Command: '$Command' ---" -ForegroundColor Yellow
    
    # Send command with carriage return (like PuTTY)
    $cmdBytes = [System.Text.Encoding]::ASCII.GetBytes("$Command`r")
    $Stream.Write($cmdBytes, 0, $cmdBytes.Length)
    $Stream.Flush()
    
    Write-Host "Command sent: '$Command'" -ForegroundColor Gray
    
    # Wait for response with multiple attempts
    $response = ""
    $totalBytes = 0
    
    for ($attempt = 1; $attempt -le 30; $attempt++) {
        Start-Sleep -Milliseconds 200
        
        if ($Stream.DataAvailable) {
            $bytesRead = $Stream.Read($Buffer, 0, $Buffer.Length)
            $newData = [System.Text.Encoding]::ASCII.GetString($Buffer, 0, $bytesRead)
            $response += $newData
            $totalBytes += $bytesRead
            Write-Host "  Attempt $attempt`: Read $bytesRead bytes" -ForegroundColor Gray
        }
        
        # Check if we have a complete response (ends with prompt)
        if ($response -match '>$') {
            Write-Host "  Complete response detected (ends with '>')" -ForegroundColor Green
            break
        }
    }
    
    Write-Host "Total response: $($response.Length) characters ($totalBytes bytes)" -ForegroundColor Cyan
    
    if ($response.Length -gt 10) {
        Write-Host "✅ Command '$Command' SUCCESS" -ForegroundColor Green
        
        # Show response preview
        $lines = $response -split "`n"
        $previewLines = $lines | Select-Object -First 10
        Write-Host "Response preview (first 10 lines):" -ForegroundColor Cyan
        foreach ($line in $previewLines) {
            Write-Host "  $line" -ForegroundColor Gray
        }
        if ($lines.Count -gt 10) {
            Write-Host "  ... ($($lines.Count - 10) more lines)" -ForegroundColor Gray
        }
        return $true
    } else {
        Write-Host "❌ Command '$Command' FAILED - No meaningful response" -ForegroundColor Red
        if ($response.Length -gt 0) {
            Write-Host "Raw response: '$response'" -ForegroundColor DarkGray
        }
        return $false
    }
}

try {
    # Step 1: Create connection exactly like PuTTY
    Write-Host "`n--- Step 1: TCP Connection ---" -ForegroundColor Yellow
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.ReceiveTimeout = 15000  # Longer timeout
    $tcpClient.SendTimeout = 10000
    
    Write-Host "Connecting to 192.168.0.20:23..." -ForegroundColor Gray
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    Write-Host "✅ TCP connection established" -ForegroundColor Green
    
    # Step 2: Wait for system to be ready (like PuTTY does)
    Write-Host "`n--- Step 2: System Stabilization ---" -ForegroundColor Yellow
    Write-Host "Waiting 3 seconds for system to be ready..." -ForegroundColor Gray
    Start-Sleep -Seconds 3
    
    # Step 3: Check for any initial banner
    Write-Host "`n--- Step 3: Initial Banner Check ---" -ForegroundColor Yellow
    $buffer = New-Object byte[] 4096
    $initialData = ""
    
    if ($stream.DataAvailable) {
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        $initialData = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
        Write-Host "Initial banner ($bytesRead bytes):" -ForegroundColor Cyan
        Write-Host $initialData -ForegroundColor Gray
    } else {
        Write-Host "No initial banner" -ForegroundColor Gray
    }
    
    # Step 4: Send initial carriage return (PuTTY handshake)
    Write-Host "`n--- Step 4: Initial Handshake ---" -ForegroundColor Yellow
    Write-Host "Sending initial carriage return..." -ForegroundColor Gray
    
    $crBytes = [System.Text.Encoding]::ASCII.GetBytes("`r")
    $stream.Write($crBytes, 0, $crBytes.Length)
    $stream.Flush()
    
    # Wait for prompt response
    Start-Sleep -Milliseconds 1000
    
    $promptResponse = ""
    if ($stream.DataAvailable) {
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        $promptResponse = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
        Write-Host "Prompt response: '$promptResponse'" -ForegroundColor Cyan
    }
    
    if ($promptResponse -match '>') {
        Write-Host "✅ Initial handshake successful - CLI ready" -ForegroundColor Green
    } else {
        Write-Host "⚠️  No prompt received, but continuing..." -ForegroundColor Yellow
    }
    
    # Step 5: Test commands one by one
    Write-Host "`n--- Step 5: Command Testing ---" -ForegroundColor Yellow
    
    $commands = @("help", "heap-info", "stack-info", "memory-info", "uavcan-status")
    $successCount = 0
    
    foreach ($cmd in $commands) {
        $success = Test-TelnetCommand -Command $cmd -Stream $stream -Buffer $buffer
        if ($success) {
            $successCount++
        }
        
        # Small delay between commands
        Start-Sleep -Milliseconds 500
    }
    
    # Step 6: Results
    Write-Host "`n--- Step 6: Final Results ---" -ForegroundColor Yellow
    Write-Host "Successful commands: $successCount/$($commands.Count)" -ForegroundColor Cyan
    
    if ($successCount -eq $commands.Count) {
        Write-Host "🎉 ALL TELNET COMMANDS WORKING!" -ForegroundColor Green
    } elseif ($successCount -gt 0) {
        Write-Host "⚠️  PARTIAL SUCCESS - $successCount commands working" -ForegroundColor Yellow
    } else {
        Write-Host "❌ NO COMMANDS WORKING" -ForegroundColor Red
    }
    
} catch {
    Write-Host "❌ Error: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "Stack trace:" -ForegroundColor Red
    Write-Host $_.Exception.StackTrace -ForegroundColor DarkRed
} finally {
    # Cleanup
    Write-Host "`n--- Cleanup ---" -ForegroundColor Yellow
    if ($stream) {
        $stream.Close()
        $stream.Dispose()
        Write-Host "✅ Stream closed" -ForegroundColor Green
    }
    if ($tcpClient) {
        $tcpClient.Close()
        $tcpClient.Dispose()
        Write-Host "✅ TCP client closed" -ForegroundColor Green
    }
    Start-Sleep -Seconds 1
}

Write-Host "`n=== Detailed Telnet Debug Complete ===" -ForegroundColor Blue