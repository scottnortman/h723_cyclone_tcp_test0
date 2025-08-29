#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Telnet Test with Proper Prompt Confirmation
.DESCRIPTION
    Test telnet interface with mandatory '>' prompt confirmation before each command
#>

Write-Host "=== Telnet Test with Proper Prompt Confirmation ===" -ForegroundColor Blue

function Wait-ForPrompt {
    param(
        [System.Net.Sockets.NetworkStream]$Stream,
        [byte[]]$Buffer,
        [int]$TimeoutSeconds = 10
    )
    
    Write-Host "  Waiting for '>' prompt..." -ForegroundColor Gray
    
    $timeout = $TimeoutSeconds * 1000 / 100  # Convert to 100ms intervals
    $attempts = 0
    
    while ($attempts -lt $timeout) {
        if ($Stream.DataAvailable) {
            $bytesRead = $Stream.Read($Buffer, 0, $Buffer.Length)
            $response = [System.Text.Encoding]::ASCII.GetString($Buffer, 0, $bytesRead)
            
            Write-Host "    Received: '$response'" -ForegroundColor DarkGray
            
            if ($response -match '>') {
                Write-Host "  ✅ Prompt '>' confirmed" -ForegroundColor Green
                return $true
            }
        }
        
        Start-Sleep -Milliseconds 100
        $attempts++
    }
    
    Write-Host "  ❌ Timeout waiting for prompt" -ForegroundColor Red
    return $false
}

function Send-CommandWithPromptConfirmation {
    param(
        [string]$Command,
        [System.Net.Sockets.NetworkStream]$Stream,
        [byte[]]$Buffer
    )
    
    Write-Host "`n--- Testing Command: '$Command' ---" -ForegroundColor Yellow
    
    # Step 1: Wait for prompt before sending command
    if (-not (Wait-ForPrompt -Stream $Stream -Buffer $Buffer)) {
        Write-Host "❌ No prompt received, cannot send command" -ForegroundColor Red
        return $false
    }
    
    # Step 2: Send command
    Write-Host "  Sending command: '$Command'" -ForegroundColor Gray
    $cmdBytes = [System.Text.Encoding]::ASCII.GetBytes("$Command`r")
    $Stream.Write($cmdBytes, 0, $cmdBytes.Length)
    $Stream.Flush()
    
    # Step 3: Wait for response
    Write-Host "  Waiting for command response..." -ForegroundColor Gray
    Start-Sleep -Seconds 2
    
    # Step 4: Read response
    $response = ""
    $attempts = 0
    
    while ($attempts -lt 30) {
        if ($Stream.DataAvailable) {
            $bytesRead = $Stream.Read($Buffer, 0, $Buffer.Length)
            $newData = [System.Text.Encoding]::ASCII.GetString($Buffer, 0, $bytesRead)
            $response += $newData
        }
        
        # Check if response is complete (ends with prompt)
        if ($response -match '>$') {
            Write-Host "  Response complete (ends with '>')" -ForegroundColor Green
            break
        }
        
        Start-Sleep -Milliseconds 200
        $attempts++
    }
    
    Write-Host "  Response length: $($response.Length) characters" -ForegroundColor Cyan
    
    if ($response.Length -gt 10) {
        Write-Host "✅ Command '$Command' SUCCESS" -ForegroundColor Green
        
        # Show response preview (without the final prompt)
        $cleanResponse = $response -replace '>$', ''
        $lines = ($cleanResponse -split "`n") | Where-Object { $_.Trim() -ne "" } | Select-Object -First 5
        Write-Host "  Response preview:" -ForegroundColor Cyan
        foreach ($line in $lines) {
            Write-Host "    $line" -ForegroundColor Gray
        }
        return $true
    } else {
        Write-Host "❌ Command '$Command' FAILED - No meaningful response" -ForegroundColor Red
        if ($response.Length -gt 0) {
            Write-Host "  Raw response: '$response'" -ForegroundColor DarkGray
        }
        return $false
    }
}

try {
    # Step 1: Connect to telnet
    Write-Host "`n--- Step 1: Telnet Connection ---" -ForegroundColor Yellow
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.ReceiveTimeout = 20000
    $tcpClient.SendTimeout = 10000
    
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    Write-Host "✅ Telnet connection established" -ForegroundColor Green
    
    # Step 2: System stabilization
    Write-Host "`n--- Step 2: System Stabilization ---" -ForegroundColor Yellow
    Start-Sleep -Seconds 3
    
    # Step 3: Initial handshake
    Write-Host "`n--- Step 3: Initial Handshake ---" -ForegroundColor Yellow
    $buffer = New-Object byte[] 4096
    
    # Flush any initial data
    while ($stream.DataAvailable) {
        $stream.Read($buffer, 0, $buffer.Length) | Out-Null
    }
    
    # Send multiple carriage returns until we get a prompt
    Write-Host "Sending carriage returns until '>' prompt received..." -ForegroundColor Gray
    $crBytes = [System.Text.Encoding]::ASCII.GetBytes("`r")
    $maxAttempts = 5
    $promptReceived = $false
    
    for ($attempt = 1; $attempt -le $maxAttempts; $attempt++) {
        Write-Host "  Attempt $attempt`: Sending CR (0x0D)..." -ForegroundColor Gray
        $stream.Write($crBytes, 0, $crBytes.Length)
        $stream.Flush()
        
        # Wait for prompt with shorter timeout per attempt
        if (Wait-ForPrompt -Stream $stream -Buffer $buffer -TimeoutSeconds 3) {
            $promptReceived = $true
            Write-Host "  ✅ Prompt received after $attempt attempt(s)" -ForegroundColor Green
            break
        }
        
        Write-Host "  No prompt yet, trying again..." -ForegroundColor Yellow
        Start-Sleep -Milliseconds 500
    }
    
    if (-not $promptReceived) {
        throw "Initial handshake failed - no prompt received after $maxAttempts attempts"
    }
    
    Write-Host "✅ Initial handshake successful" -ForegroundColor Green
    
    # Step 4: Test commands with proper prompt confirmation
    Write-Host "`n--- Step 4: Command Testing with Prompt Confirmation ---" -ForegroundColor Yellow
    
    $commands = @("help", "heap-info", "stack-info", "memory-info", "uavcan-status")
    $successCount = 0
    
    foreach ($cmd in $commands) {
        $success = Send-CommandWithPromptConfirmation -Command $cmd -Stream $stream -Buffer $buffer
        if ($success) {
            $successCount++
        }
        
        # Brief pause between commands
        Start-Sleep -Milliseconds 500
    }
    
    # Step 5: Results
    Write-Host "`n--- Step 5: Final Results ---" -ForegroundColor Yellow
    Write-Host "Successful commands: $successCount/$($commands.Count)" -ForegroundColor Cyan
    
    if ($successCount -eq $commands.Count) {
        Write-Host "🎉 ALL TELNET COMMANDS WORKING WITH PROMPT CONFIRMATION!" -ForegroundColor Green
    } elseif ($successCount -gt 0) {
        Write-Host "⚠️  PARTIAL SUCCESS - $successCount commands working" -ForegroundColor Yellow
    } else {
        Write-Host "❌ NO COMMANDS WORKING" -ForegroundColor Red
    }
    
} catch {
    Write-Host "❌ Error: $($_.Exception.Message)" -ForegroundColor Red
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
}

Write-Host "`n=== Telnet Test with Prompt Confirmation Complete ===" -ForegroundColor Blue