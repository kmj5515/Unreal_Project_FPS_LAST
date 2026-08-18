@echo off
setlocal
echo ====================================================
echo Building Client Installer (Inno Setup)
echo ====================================================
echo.

rem 버전은 첫 번째 인자로 덮어쓸 수 있게 둔다. (예: MakeClientInstaller.bat 1.2.0)
set VERSION=%~1
if "%VERSION%"=="" set VERSION=1.0.0

rem winget는 관리자 권한이 없으면 사용자 폴더에 설치하므로 세 경로를 모두 확인한다.
set ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe
if not exist "%ISCC%" set ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe
if not exist "%ISCC%" set ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe
if not exist "%ISCC%" (
    echo Inno Setup not found. Install it first:  winget install JRSoftware.InnoSetup
    pause
    exit /b 1
)

rem PackageClient.bat과 동일하게 이 BAT 위치를 기준으로 저장소 루트를 찾는다.
set REPO=%~dp0
if not exist "%REPO%PackagedClient\Windows\LastFPS.exe" set REPO=%~dp0..\
if not exist "%REPO%PackagedClient\Windows\LastFPS.exe" (
    echo PackagedClient not found. Run Tools\PackageClient.bat first.
    pause
    exit /b 1
)

"%ISCC%" /DMyAppVersion=%VERSION% "%REPO%Tools\Installer\LastFPS.iss"

if %errorlevel% neq 0 (
    echo.
    echo INSTALLER BUILD FAILED!
) else (
    echo.
    echo INSTALLER BUILD SUCCESSFUL!
    echo Output directory: %REPO%Installer
)
pause
endlocal
