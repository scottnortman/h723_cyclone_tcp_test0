#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Telnet Test with Proper Prompt Confirmation
.DESCRIPTION
    Wait for '>' prompt before sending each command to handle buffering issues
#>

Write-Host "=== Telnet Test with Proper Prompt Confirmation ===" -ForegroundColor Blue

function Wait-ForPrompt {
    param(
        [System.Net.Sockets.NetworkStream]$Stream,
        [byte[]]$Buffer,
        [int]$TimeoutSeconds = 10
    )
    
    $startTime = Get-Date
    $response = ""
    
    while (((Get-Date) - $startTime).TotalSeconds -lt $TimeoutSeconds) {
        if ($Stream.DataAvailable) {
            $bytesRead = $Stream.Read($Buffer, 0, $Buffer.Length)
            $newData = [System.Text.Encoding]::ASCII.GetString($Buffer, 0, $bytesRead)
            $response += $newData
            
            # Check if we received the prompt
            if ($response -match '>$') {
                Write-Host "  ✅ Prompt received: '$($response.Trim())'" -ForegroundColor Green
                return $true
            }
        }
        Start-Sleep -Milliseconds 100
    }
    
    Write-Host "  ❌ Timeout waiting for prompt. Received: '$response'" -ForegroundColor Red
    return $false
}

function Send-CommandAndWaitForResponse {
    param(
        [string]$Command,
        [System.Net.Sockets.NetworkStream]$Stream,
        [byte[]]$Buffer
    )
    
    Write-Host "`n--- Testing Command: '$Command' ---" -ForegroundColor Yellow
    
    # Step 1: Wait for prompt before sending command
    Write-Host "Step 1: Waiting for prompt..." -ForegroundColor Gray
    if (-not (Wait-ForPrompt -Stream $Stream -Buffer $Buffer)) {
        Write-Host "❌ No prompt received, skipping command" -ForegroundColor Red
        return $false
    }
    
    # Step 2: Send command
    Write-Host "Step 2: Sending command '$Command'..." -ForegroundColor Gray
    $cmdBytes = [System.Text.Encoding]::ASCII.GetBytes("$Command`r")
    $Stream.Write($cmdBytes, 0, $cmdBytes.Length)
    $Stream.Flush()
    
    # Step 3: Wait for response (not just prompt)
    Write-Host "Step 3: Waiting for command response..." -ForegroundColor Gray
    $startTime = Get-Date
    $response = ""
    $gotResponse = $false
    
    while (((Get-Date) - $startTime).TotalSeconds -lt 15) {
        if ($Stream.DataAvailable) {
            $bytesRead = $Stream.Read($Buffer, 0, $Buffer.Length)
            $newData = [System.Text.Encoding]::ASCII.GetString($Buffer, 0, $bytesRead)
            $response += $newData
            
            # Check if we got a complete response (ends with prompt)
            if ($response -match '>$' -and $response.Length -gt 10) {
                $gotResponse = $true
                break
            }
        }
        Start-Sleep -Milliseconds 200
    }
    
    Write-Host "Response length: $($response.Length) characters" -ForegroundColor Cyan
    
    if ($gotResponse -and $response.Length -gt 10) {
        Write-Host "✅ Command '$Command' SUCCESS" -ForegroundColor Green
        
        # Show response preview (without the final prompt)
        $cleanResponse = $response -replace '>$', ''
        $lines = ($cleanResponse -split "`n") | Where-Object { $_.Trim() -ne "" } | Select-Object -First 5
        Write-Host "Response preview:" -ForegroundColor Cyan
        foreach ($line in $lines) {
            Write-Host "  $($line.Trim())" -ForegroundColor Gray
        }
        return $true
    } else {
        Write-Host "❌ Command '$Command' FAILED" -ForegroundColor Red
        if ($response.Length -gt 0) {
            Write-Host "Raw response: '$response'" -ForegroundColor DarkGray
        }
        return $false
    }
}

try {
    # Connect to telnet
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.ReceiveTimeout = 20000
    $tcpClient.SendTimeout = 10000
    
    Write-Host "Connecting to 192.168.0.20:23..." -ForegroundColor Gray
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    Write-Host "✅ TCP connection established" -ForegroundColor Green
    
    # Wait for system to stabilize
    Start-Sleep -Seconds 3
    
    $buffer = New-Object byte[] 4096
    
    # Initial handshake - send CR and wait for first prompt
    Write-Host "`nInitial handshake..." -ForegroundColor Yellow
    $crBytes = [System.Text.Encoding]::ASCII.GetBytes("`r")
    $stream.Write($crBytes, 0, $crBytes.Length)
    $stream.Flush()
    
    if (-not (Wait-ForPrompt -Stream $stream -Buffer $buffer)) {
        throw "Initial handshake failed - no prompt received"
    }
    
    Write-Host "✅ Initial handshake successful" -ForegroundColor Green
    
    # Test commands with proper prompt waiting
    $commands = @("help", "heap-info", "stack-info", "memory-info", "uavcan-status")
    $successCount = 0
    
    foreach ($cmd in $commands) {
        $success = Send-CommandAndWaitForResponse -Command $cmd -Stream $stream -Buffer $buffer
        if ($success) {
            $successCount++
        }
        
        # Small delay between commands
        Start-Sleep -Milliseconds 500
    }
    
    # Results
    Write-Host "`n=== FINAL RESULTS ===" -ForegroundColor Blue
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
} finally {
    if ($stream) {
        $stream.Close()
        $stream.Dispose()
    }
    if ($tcpClient) {
        $tcpClient.Close()
        $tcpClient.Dispose()
    }
    Start-Sleep -Seconds 1
}

Write-Host "`n=== Telnet Test with Prompt Confirmation Complete ===" -ForegroundColor Blue