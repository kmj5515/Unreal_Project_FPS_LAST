. (Join-Path $PSScriptRoot 'Common.ps1')

# 예약 작업 이름은 Install-StartupTask.ps1 이 등록하는 값과 반드시 같아야 한다.
$supervisorTaskName = 'LastFPS Dedicated Server'

# 슈퍼바이저가 살아 있으면 정지가 실패한다.
# Run-Supervisor 는 "플래그 확인 → 서버 기동" 사이에 틈이 있어, 그 사이에 서버를 죽이면
# PID 파일에 잡히지 않는 새 서버를 띄운다. 플래그를 먼저 세워 재기동 경로를 막고,
# 슈퍼바이저 자체를 끊은 뒤에 서버를 종료한다.
'maintenance' | Set-Content -LiteralPath (Get-MaintenanceFlagPath) -Encoding Ascii

$supervisorTask = Get-ScheduledTask -TaskName $supervisorTaskName -ErrorAction SilentlyContinue
if ($supervisorTask)
{
    # SYSTEM 계정으로 등록된 작업이라 관리자 권한이 없으면 여기서 막힌다.
    # 04-Stop-Server.bat 이 권한을 올려 실행하는 이유다.
    try
    {
        Stop-ScheduledTask -TaskName $supervisorTaskName -ErrorAction Stop
        Write-Host "Supervisor task stopped: $supervisorTaskName"
    }
    catch
    {
        Write-Warning "Failed to stop the supervisor task. Run this script as Administrator. $($_.Exception.Message)"
    }
}

$process = Get-TrackedServerProcess
if (-not $process) { Write-Host 'No tracked LastFPS server process was found.'; exit 0 }
$config = Get-ServerConfig
$expectedName = [System.IO.Path]::GetFileNameWithoutExtension([string]$config.ExecutablePath)
if ($process.ProcessName -ne $expectedName) { throw "PID file points to a different process. Refusing to stop it: PID=$($process.Id)" }
Stop-Process -Id $process.Id
if (-not $process.WaitForExit(15000)) { throw "Server did not stop before timeout. PID=$($process.Id)" }
Remove-Item -LiteralPath (Get-ServerPidPath) -Force -ErrorAction SilentlyContinue
Write-Host "LastFPS server stopped. PID=$($process.Id)"
