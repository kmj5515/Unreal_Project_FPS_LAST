@echo off
rem 서버와 슈퍼바이저는 SYSTEM 계정의 예약 작업으로 기동된다.
rem 권한을 올리지 않으면 Stop-Process 와 Stop-ScheduledTask 가 접근 거부로 실패해
rem 서버가 그대로 살아남는다. 이미 관리자면 그대로 실행하고, 아니면 승격 후 실행한다.
net session >nul 2>&1
if %errorlevel%==0 goto run

powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "Start-Process powershell.exe -Verb RunAs -ArgumentList '-NoProfile -ExecutionPolicy Bypass -NoExit -File ""%~dp0Scripts\Stop-Server.ps1""'"
exit /b 0

:run
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Stop-Server.ps1"
if errorlevel 1 pause
