#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Test All Commands with Working Telnet Protocol
.DESCRIPTION
    Test all commands using the proven working telnet protocol
#>

Write-Host "=== Test All Commands with Working Telnet Protocol ===" -ForegroundColor Blue

function Send-TelnetCommand {
    param(
        [string]$Command,
        [System.Net.Sockets.NetworkStream]$Stream,
        [byte[]]$Buffer
    )
    
    Write-Host "`nTesting '$Command'..." -ForegroundColor Yellow
    
    # Send command with carriage return (proven working method)
    $cmdBytes = [System.Text.Encoding]::ASCII.GetBytes("$Command`r")
    $Stream.Write($cmdBytes, 0, $cmdBytes.Length)
    $Stream.Flush()
    
    # Wait for response (same timing as working help command)
    Start-Sleep -Seconds 3
    
    $response = ""
    $attempts = 0
    while ($attempts -lt 20) {
        if ($Stream.DataAvailable) {
            $bytesRead = $Stream.Read($Buffer, 0, $Buffer.Length)
            $newData = [System.Text.Encoding]::ASCII.GetString($Buffer, 0, $bytesRead)
            $response += $newData
        }
        Start-Sleep -Milliseconds 200
        $attempts++
    }
    
    Write-Host "Response length: $($response.Length) characters" -ForegroundColor Cyan
    
    if ($response.Length -gt 10) {
        Write-Host "✅ SUCCESS" -ForegroundColor Green
        
        # Show preview
        $lines = ($response -split "`n") | Where-Object { $_.Trim() -ne "" } | Select-Object -First 3
        foreach ($line in $lines) {
            Write-Host "  $($line.Trim())" -ForegroundColor Gray
        }
        return $true
    } else {
        Write-Host "❌ FAILED" -ForegroundColor Red
        if ($response.Length -gt 0) {
            Write-Host "Raw: '$response'" -ForegroundColor DarkGray
        }
        return $false
    }
}

try {
    # Use the proven working connection method
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.ReceiveTimeout = 15000
    $tcpClient.SendTimeout = 10000
    
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    Write-Host "✅ Connected" -ForegroundColor Green
    
    $buffer = New-Object byte[] 4096
    
    # Use the proven working handshake
    Start-Sleep -Seconds 2
    $crBytes = [System.Text.Encoding]::ASCII.GetBytes("`r")
    $stream.Write($crBytes, 0, $crBytes.Length)
    $stream.Flush()
    
    # Confirm we get the prompt
    Start-Sleep -Seconds 1
    if ($stream.DataAvailable) {
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        $prompt = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
        if ($prompt -match '>') {
            Write-Host "✅ Handshake successful" -ForegroundColor Green
        }
    }
    
    # Test all commands using the working protocol
    $commands = @("help", "heap-info", "stack-info", "memory-info", "uavcan-status", "task-stats")
    $successCount = 0
    
    foreach ($cmd in $commands) {
        $success = Send-TelnetCommand -Command $cmd -Stream $stream -Buffer $buffer
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
        Write-Host "✅ Telnet interface is fully functional" -ForegroundColor Green
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
}

Write-Host "`n=== All Commands Test Complete ===" -ForegroundColor Blue