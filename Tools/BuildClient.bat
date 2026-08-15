@echo off
setlocal
echo ====================================================
echo Building Standalone Client (LastFPS Win64 Development)
echo ====================================================
echo.

rem 엔진 경로는 환경에 따라 다르므로 UE_ROOT 환경 변수로 덮어쓸 수 있게 둔다.
if "%UE_ROOT%"=="" set UE_ROOT=E:\UE_5.7
set ENGINE=%UE_ROOT%\Engine

rem 저장소를 다른 위치로 옮겨도 동작하도록 이 BAT 위치를 기준으로 계산한다.
rem 이 BAT이 저장소 루트에 있든 하위 폴더에 있든 동작하도록 uproject를 찾아 루트를 정한다.
rem 위치를 옮길 때마다 상대 경로를 고쳐야 하면 실행 시점에야 UBT가 "프로젝트를 못 찾음"으로 실패한다.
set REPO=%~dp0
if not exist "%REPO%LastFPS\LastFPS.uproject" set REPO=%~dp0..\
if not exist "%REPO%LastFPS\LastFPS.uproject" (
    echo Project not found. Run this from the repository root or one folder below it.
    pause
    exit /b 1
)
set PROJECT=%REPO%LastFPS\LastFPS.uproject

if not exist "%ENGINE%\Build\BatchFiles\Build.bat" (
    echo Engine not found: %ENGINE%
    echo Set UE_ROOT to your Unreal Engine install path and run again.
    pause
    exit /b 1
)

call "%ENGINE%\Build\BatchFiles\Build.bat" LastFPS Win64 Development -Project="%PROJECT%" -WaitMutex

if %errorlevel% neq 0 (
    echo.
    echo BUILD FAILED!
) else (
    echo.
    echo BUILD SUCCESSFUL!
    echo Client executable is located at:
    echo %REPO%LastFPS\Binaries\Win64\LastFPS.exe
)

pause
endlocal
