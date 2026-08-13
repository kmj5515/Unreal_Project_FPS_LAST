@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Stop-Server.ps1"
if errorlevel 1 pause
