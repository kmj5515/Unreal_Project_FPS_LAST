. (Join-Path $PSScriptRoot 'Common.ps1')
$config = Get-ServerConfig
Assert-ServerConfig -Config $config
if (-not (Test-Path -LiteralPath $config.ExecutablePath -PathType Leaf)) { throw "Server executable not found: $($config.ExecutablePath)" }
$existing = Get-TrackedServerProcess
if ($existing) { Write-Host "Server is already running. PID=$($existing.Id)"; exit 0 }
$maintenance = Get-MaintenanceFlagPath
if (Test-Path -LiteralPath $maintenance) { Remove-Item -LiteralPath $maintenance -Force }
$logPath = Join-Path (Get-LogDirectory) "LastFPSServer-$(Get-Date -Format 'yyyyMMdd-HHmmss').log"
$arguments = @([string]$config.LobbyMap, '-server', "-port=$([int]$config.GamePort)", '-log', "-abslog=$logPath") + @($config.AdditionalArguments)
$process = Start-Process -FilePath $config.ExecutablePath -ArgumentList $arguments -WorkingDirectory (Split-Path -Parent $config.ExecutablePath) -WindowStyle Hidden -PassThru
$process.Id | Set-Content -LiteralPath (Get-ServerPidPath) -Encoding Ascii
Start-Sleep -Seconds 3
if ($process.HasExited)
{
    Remove-Item -LiteralPath (Get-ServerPidPath) -Force -ErrorAction SilentlyContinue
    throw "Server exited immediately after startup. Check the log: $logPath"
}
Write-Host "LastFPS server started. PID=$($process.Id), UDP=$([int]$config.GamePort)"
Write-Host "Log: $logPath"
