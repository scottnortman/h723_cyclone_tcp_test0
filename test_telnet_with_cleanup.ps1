#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Telnet Test with Proper Session Cleanup
.DESCRIPTION
    Test telnet interface with proper session management and cleanup
#>

Write-Host "=== Telnet Test with Proper Session Cleanup ===" -ForegroundColor Blue

# Step 1: Kill any existing telnet processes
Write-Host "`n--- Step 1: Session Cleanup ---" -ForegroundColor Yellow
$telnetProcesses = Get-Process -Name "telnet" -ErrorAction SilentlyContinue
if ($telnetProcesses) {
    Write-Host "Killing $($telnetProcesses.Count) existing telnet process(es)..." -ForegroundColor Yellow
    $telnetProcesses | Stop-Process -Force
    Write-Host "✅ Telnet processes terminated" -ForegroundColor Green
} else {
    Write-Host "✅ No existing telnet processes found" -ForegroundColor Green
}

# Step 2: Wait for firmware to release connection
Write-Host "`n--- Step 2: Connection Release Wait ---" -ForegroundColor Yellow
Write-Host "Waiting 3 seconds for firmware to release connection..." -ForegroundColor Gray
Start-Sleep -Seconds 3
Write-Host "✅ Wait completed" -ForegroundColor Green

# Step 3: Test telnet connection with proper handshake
Write-Host "`n--- Step 3: Telnet Connection Test ---" -ForegroundColor Yellow

try {
    # Create TCP client with proper timeouts
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.ReceiveTimeout = 10000
    $tcpClient.SendTimeout = 5000
    
    Write-Host "Connecting to 192.168.0.20:23..." -ForegroundColor Gray
    $tcpClient.Connect("192.168.0.20", 23)
    $stream = $tcpClient.GetStream()
    
    Write-Host "✅ TCP connection established" -ForegroundColor Green
    
    # Step 4: Wait for system stabilization
    Write-Host "`n--- Step 4: System Stabilization ---" -ForegroundColor Yellow
    Write-Host "Waiting 2 seconds for system to stabilize..." -ForegroundColor Gray
    Start-Sleep -Seconds 2
    
    # Step 5: Flush any initial data
    Write-Host "`n--- Step 5: Buffer Flush ---" -ForegroundColor Yellow
    $buffer = New-Object byte[] 2048
    $flushedBytes = 0
    
    while ($stream.DataAvailable) {
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        $flushedBytes += $bytesRead
        Start-Sleep -Milliseconds 100
    }
    
    if ($flushedBytes -gt 0) {
        Write-Host "Flushed $flushedBytes bytes of initial data" -ForegroundColor Gray
    } else {
        Write-Host "No initial data to flush" -ForegroundColor Gray
    }
    
    # Step 6: Handshake with carriage return
    Write-Host "`n--- Step 6: CLI Handshake ---" -ForegroundColor Yellow
    
    $maxRetries = 3
    $handshakeSuccess = $false
    
    for ($retry = 1; $retry -le $maxRetries; $retry++) {
        Write-Host "Handshake attempt $retry/$maxRetries..." -ForegroundColor Gray
        
        # Send carriage return
        $crBytes = [System.Text.Encoding]::ASCII.GetBytes("`r")
        $stream.Write($crBytes, 0, $crBytes.Length)
        $stream.Flush()
        
        # Wait for response
        Start-Sleep -Milliseconds 500
        
        # Check for prompt
        if ($stream.DataAvailable) {
            $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
            $response = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
            Write-Host "Handshake response ($bytesRead bytes): '$response'" -ForegroundColor Gray
            
            if ($response -match '>') {
                Write-Host "✅ CLI handshake successful!" -ForegroundColor Green
                $handshakeSuccess = $true
                break
            }
        }
        
        if ($retry -lt $maxRetries) {
            Write-Host "No prompt received, retrying..." -ForegroundColor Yellow
            Start-Sleep -Milliseconds 500
        }
    }
    
    if (-not $handshakeSuccess) {
        Write-Host "❌ CLI handshake failed after $maxRetries attempts" -ForegroundColor Red
        throw "Handshake failed"
    }
    
    # Step 7: Test commands
    Write-Host "`n--- Step 7: Command Testing ---" -ForegroundColor Yellow
    
    $commands = @("help", "heap-info", "stack-info")
    $successfulCommands = 0
    
    foreach ($cmd in $commands) {
        Write-Host "`nTesting command: '$cmd'" -ForegroundColor Gray
        
        # Send command with carriage return
        $cmdBytes = [System.Text.Encoding]::ASCII.GetBytes("$cmd`r")
        $stream.Write($cmdBytes, 0, $cmdBytes.Length)
        $stream.Flush()
        
        # Wait for response
        Start-Sleep -Seconds 3
        
        # Read response
        $response = ""
        $totalBytes = 0
        $attempts = 0
        
        while ($attempts -lt 20) {
            if ($stream.DataAvailable) {
                $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
                $newData = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
                $response += $newData
                $totalBytes += $bytesRead
            }
            Start-Sleep -Milliseconds 100
            $attempts++
        }
        
        Write-Host "Response length: $($response.Length) characters ($totalBytes bytes)" -ForegroundColor Cyan
        
        if ($response.Length -gt 10) {
            Write-Host "✅ Command '$cmd' successful" -ForegroundColor Green
            $successfulCommands++
            
            # Show first 200 characters of response
            $preview = if ($response.Length -gt 200) { $response.Substring(0, 200) + "..." } else { $response }
            Write-Host "Response preview:" -ForegroundColor Cyan
            Write-Host $preview -ForegroundColor Gray
        } else {
            Write-Host "❌ Command '$cmd' failed or no response" -ForegroundColor Red
        }
    }
    
    # Step 8: Results
    Write-Host "`n--- Step 8: Test Results ---" -ForegroundColor Yellow
    Write-Host "Successful commands: $successfulCommands/$($commands.Count)" -ForegroundColor Cyan
    
    if ($successfulCommands -eq $commands.Count) {
        Write-Host "🎉 ALL TELNET TESTS PASSED!" -ForegroundColor Green
    } elseif ($successfulCommands -gt 0) {
        Write-Host "⚠️  PARTIAL SUCCESS - Some commands working" -ForegroundColor Yellow
    } else {
        Write-Host "❌ ALL TELNET TESTS FAILED" -ForegroundColor Red
    }
    
} catch {
    Write-Host "❌ Telnet test failed: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    # Step 9: Proper cleanup
    Write-Host "`n--- Step 9: Connection Cleanup ---" -ForegroundColor Yellow
    
    if ($stream) {
        $stream.Close()
        $stream.Dispose()
        Write-Host "✅ Stream closed and disposed" -ForegroundColor Green
    }
    
    if ($tcpClient) {
        $tcpClient.Close()
        $tcpClient.Dispose()
        Write-Host "✅ TCP client closed and disposed" -ForegroundColor Green
    }
    
    # Wait for firmware to release connection
    Write-Host "Waiting 1 second for firmware to release connection..." -ForegroundColor Gray
    Start-Sleep -Seconds 1
    
    Write-Host "✅ Cleanup completed" -ForegroundColor Green
}

Write-Host "`n=== Telnet Test with Proper Session Cleanup Complete ===" -ForegroundColor Blue