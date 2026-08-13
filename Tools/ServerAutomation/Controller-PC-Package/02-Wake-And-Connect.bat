@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Wake-And-Connect.ps1"
if errorlevel 1 pause
