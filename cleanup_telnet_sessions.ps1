#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Cleanup Telnet Sessions
.DESCRIPTION
    Kills any existing telnet processes and clears connections to port 23
#>

Write-Host "=== Telnet Session Cleanup ===`n" -ForegroundColor Blue

# Kill any existing telnet processes
Write-Host "Checking for existing telnet processes..." -ForegroundColor Yellow
$telnetProcesses = Get-Process -Name "telnet" -ErrorAction SilentlyContinue

if ($telnetProcesses) {
    Write-Host "Found $($telnetProcesses.Count) telnet process(es). Terminating..." -ForegroundColor Yellow
    $telnetProcesses | Stop-Process -Force
    Write-Host "✅ Telnet processes terminated" -ForegroundColor Green
} else {
    Write-Host "✅ No telnet processes found" -ForegroundColor Green
}

# Check for connections to port 23
Write-Host "`nChecking for connections to port 23..." -ForegroundColor Yellow
$connections = netstat -an | findstr ":23"

if ($connections) {
    Write-Host "Active connections to port 23:" -ForegroundColor Yellow
    Write-Host $connections -ForegroundColor Gray
} else {
    Write-Host "✅ No active connections to port 23" -ForegroundColor Green
}

# Wait for firmware to release connection
Write-Host "`nWaiting 3 seconds for firmware to release connection..." -ForegroundColor Yellow
Start-Sleep -Seconds 3

Write-Host "✅ Telnet session cleanup completed`n" -ForegroundColor Green