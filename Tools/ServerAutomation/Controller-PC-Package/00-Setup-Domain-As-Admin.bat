@echo off
:: 관리자 권한 확인
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo =====================================================
    echo [ERROR] Please right-click this file and select "Run as Administrator".
    echo [오류] 이 파일을 우클릭하여 '관리자 권한으로 실행'해 주세요.
    echo =====================================================
    pause
    exit /b 1
)

echo =====================================================
echo Setting up local domain: server.lastfps.com -^> 172.30.1.15
echo =====================================================

:: hosts 파일에 이미 있는지 확인
findstr /C:"server.lastfps.com" "C:\Windows\System32\drivers\etc\hosts" >nul
if %errorlevel% equ 0 (
    echo Domain already exists in hosts file!
    echo 이미 도메인이 등록되어 있습니다.
) else (
    echo. >> "C:\Windows\System32\drivers\etc\hosts"
    echo 172.30.1.15 server.lastfps.com >> "C:\Windows\System32\drivers\etc\hosts"
    echo Successfully added domain to hosts file.
    echo 성공적으로 도메인을 등록했습니다.
)

echo.
pause
