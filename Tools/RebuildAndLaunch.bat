@echo off
setlocal

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
set LOGDIR=%REPO%LastFPS\Saved\Logs

rem Saved\Logs는 클린 클론 직후 없을 수 있어 리다이렉션 전에 만들어 둔다.
if not exist "%LOGDIR%" mkdir "%LOGDIR%"

echo BUILDING > "%LOGDIR%\AgentRebuild.status"
call "%ENGINE%\Build\BatchFiles\Build.bat" LastFPSEditor Win64 DebugGame -Project="%PROJECT%" -WaitMutex > "%LOGDIR%\AgentRebuild.log" 2>&1
set RC=%errorlevel%
echo BUILD_EXIT=%RC% > "%LOGDIR%\AgentRebuild.status"
if not %RC%==0 goto fail

echo LAUNCHING >> "%LOGDIR%\AgentRebuild.status"
start "" "%ENGINE%\Binaries\Win64\UnrealEditor-Win64-DebugGame.exe" "%PROJECT%"
exit /b 0

:fail
echo.
echo BUILD FAILED - see %LOGDIR%\AgentRebuild.log
pause
exit /b %RC%
