@echo off
setlocal
set ENGINE=E:\UE_5.7\Engine
set PROJECT=E:\Unreal_Project_FPS_LAST\LastFPS\LastFPS.uproject
set LOGDIR=E:\Unreal_Project_FPS_LAST\LastFPS\Saved\Logs

echo BUILDING > "%LOGDIR%\AgentRebuild.status"
call "%ENGINE%\Build\BatchFiles\Build.bat" LastFPSEditor Win64 DebugGame -Project="%PROJECT%" -WaitMutex > "%LOGDIR%\AgentRebuild.log" 2>&1
set RC=%errorlevel%
echo BUILD_EXIT=%RC% > "%LOGDIR%\AgentRebuild.status"
if not %RC%==0 goto fail

echo LAUNCHING >> "%LOGDIR%\AgentRebuild.status"
start "" "%ENGINE%\Binaries\Win64\UnrealEditor-Win64-DebugGame.exe" "%PROJECT%"
exit

:fail
echo.
echo BUILD FAILED - see %LOGDIR%\AgentRebuild.log
pause
