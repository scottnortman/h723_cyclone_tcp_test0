# PowerShell script to test all UAVCAN requirements via telnet
param(
    [string]$IPAddress = "192.168.0.20",
    [int]$Port = 23,
    [int]$CommandDelay = 3
)

function Send-Command {
    param(
        [System.IO.StreamWriter]$Writer,
        [System.IO.StreamReader]$Reader,
        [System.Net.NetworkInformation.NetworkStream]$Stream,
        [string]$Command,
        [int]$DelaySeconds = 3
    )
    
    Write-Host "`n=== Sending '$Command' command ===" -ForegroundColor Yellow
    $Writer.WriteLine($Command)
    $Writer.Flush()
    Start-Sleep -Seconds $DelaySeconds
    
    # Read response with timeout
    $timeout = 0
    while($timeout -lt 10 -and !$Stream.DataAvailable) {
        Start-Sleep -Milliseconds 100
        $timeout++
    }
    
    $response = @()
    while($Stream.DataAvailable) {
        $line = $Reader.ReadLine()
        if ($line) { 
            Write-Host $line -ForegroundColor Green
            $response += $line
        }
    }
    
    return $response
}

$client = New-Object System.Net.Sockets.TcpClient
try {
    Write-Host "Connecting to STM32H723 at ${IPAddress}:${Port}..." -ForegroundColor Cyan
    $client.Connect($IPAddress, $Port)
    $stream = $client.GetStream()
    $writer = New-Object System.IO.StreamWriter($stream)
    $reader = New-Object System.IO.StreamReader($stream)
    
    # Wait for connection to establish
    Start-Sleep -Seconds 2
    
    # Clear any initial output
    Write-Host "=== Initial Connection Output ===" -ForegroundColor Cyan
    while($stream.DataAvailable) {
        $line = $reader.ReadLine()
        if ($line) { Write-Host $line -ForegroundColor Gray }
    }
    
    # Test all UAVCAN CLI commands
    $commands = @(
        "help",
        "uavcan-status", 
        "uavcan-simple-verify",
        "uavcan-test",
        "uavcan-system-test",
        "uavcan-verify-requirements"
    )
    
    $results = @{}
    
    foreach ($cmd in $commands) {
        try {
            $response = Send-Command -Writer $writer -Reader $reader -Stream $stream -Command $cmd -DelaySeconds $CommandDelay
            $results[$cmd] = $response
            
            # Check for common success/failure indicators
            $success = $false
            $failure = $false
            foreach ($line in $response) {
                if ($line -match "PASS|SUCCESS|passed|completed successfully") {
                    $success = $true
                }
                if ($line -match "FAIL|ERROR|failed|error") {
                    $failure = $true
                }
            }
            
            if ($success -and !$failure) {
                Write-Host "✓ $cmd - SUCCESS" -ForegroundColor Green
            } elseif ($failure) {
                Write-Host "✗ $cmd - FAILED" -ForegroundColor Red
            } else {
                Write-Host "? $cmd - UNKNOWN" -ForegroundColor Yellow
            }
            
        } catch {
            Write-Host "✗ $cmd - EXCEPTION: $($_.Exception.Message)" -ForegroundColor Red
            $results[$cmd] = @("EXCEPTION: $($_.Exception.Message)")
        }
    }
    
    # Summary
    Write-Host "`n=== TEST SUMMARY ===" -ForegroundColor Cyan
    $passed = 0
    $failed = 0
    $unknown = 0
    
    foreach ($cmd in $commands) {
        $response = $results[$cmd]
        $success = $false
        $failure = $false
        
        foreach ($line in $response) {
            if ($line -match "PASS|SUCCESS|passed|completed successfully") {
                $success = $true
            }
            if ($line -match "FAIL|ERROR|failed|error") {
                $failure = $true
            }
        }
        
        if ($success -and !$failure) {
            Write-Host "✓ $cmd" -ForegroundColor Green
            $passed++
        } elseif ($failure) {
            Write-Host "✗ $cmd" -ForegroundColor Red
            $failed++
        } else {
            Write-Host "? $cmd" -ForegroundColor Yellow
            $unknown++
        }
    }
    
    Write-Host "`nResults: $passed passed, $failed failed, $unknown unknown" -ForegroundColor Cyan
    
    if ($failed -eq 0) {
        Write-Host "🎉 ALL TESTS PASSED!" -ForegroundColor Green
    } else {
        Write-Host "⚠️  Some tests failed. Check output above." -ForegroundColor Yellow
    }
    
} catch {
    Write-Host "Connection Error: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "Make sure the STM32H723 is connected and running at $IPAddress" -ForegroundColor Yellow
} finally {
    if ($client.Connected) {
        $client.Close()
    }
    Write-Host "`nConnection closed." -ForegroundColor Gray
}