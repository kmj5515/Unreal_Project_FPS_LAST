@echo off
setlocal
echo ====================================================
echo Packaging Standalone Client (Win64 Development)
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
set OUTDIR=%REPO%PackagedClient

if not exist "%ENGINE%\Build\BatchFiles\RunUAT.bat" (
    echo Engine not found: %ENGINE%
    echo Set UE_ROOT to your Unreal Engine install path and run again.
    pause
    exit /b 1
)

rem 클라이언트는 마스터 로비 접속 후 각 방으로 이동하므로 로비 맵과 플레이 맵이 모두 필요하다.
rem 대상 맵 목록은 DefaultGame.ini의 MapsToCook이 단일 기준이다. 여기에서 -map으로 다시 지정하지 않는다.
rem -WaitForUATMutex: UAT는 전역 뮤텍스로 한 번에 하나만 실행된다. 이 옵션이 없으면
rem 서버 패키징과 동시에 실행했을 때 즉시 "conflicting instance" 오류로 죽는다.
call "%ENGINE%\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%PROJECT%" -noP4 -platform=Win64 -clientconfig=Development -target=LastFPS -cook -build -stage -pak -iostore -prereqs -archive -archivedirectory="%OUTDIR%" -utf8output -WaitForUATMutex

if %errorlevel% neq 0 (
    echo.
    echo PACKAGING FAILED!
) else (
    echo.
    echo PACKAGING SUCCESSFUL!
    echo Output directory: %OUTDIR%
)
pause
endlocal
