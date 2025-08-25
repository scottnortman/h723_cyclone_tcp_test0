@echo off
echo === UAVCAN Full Requirements Test ===
echo Date: %date% %time%
echo Target: STM32H723 at 192.168.0.20:23
echo.

set LOGFILE=UAVCAN_Test_%date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~0,2%%time:~3,2%%time:~6,2%.txt
set LOGFILE=%LOGFILE: =0%

echo Starting test and logging to %LOGFILE%
echo === UAVCAN Full Requirements Test === > %LOGFILE%
echo Date: %date% %time% >> %LOGFILE%
echo Target: STM32H723 at 192.168.0.20:23 >> %LOGFILE%
echo. >> %LOGFILE%

powershell -Command "& {$client = New-Object System.Net.Sockets.TcpClient; try { $client.Connect('192.168.0.20', 23); $stream = $client.GetStream(); $writer = New-Object System.IO.StreamWriter($stream); $reader = New-Object System.IO.StreamReader($stream); Start-Sleep -Seconds 2; $commands = @('help', 'uavcan-status', 'uavcan-simple-verify', 'uavcan-test', 'uavcan-system-test', 'uavcan-verify-requirements'); foreach ($cmd in $commands) { Write-Host \"Testing: $cmd\" -ForegroundColor Yellow; $writer.WriteLine($cmd); $writer.Flush(); Start-Sleep -Seconds 5; while($stream.DataAvailable) { $line = $reader.ReadLine(); if ($line) { Write-Host \"  $line\" -ForegroundColor Green; } } } } catch { Write-Host \"Error: $($_.Exception.Message)\" -ForegroundColor Red } finally { if ($client.Connected) { $client.Close() } } }"

echo.
echo Test completed. Check console output above for results.
pause