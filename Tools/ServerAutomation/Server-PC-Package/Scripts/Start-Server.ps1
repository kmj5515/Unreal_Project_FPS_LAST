. (Join-Path $PSScriptRoot 'Common.ps1')
$config = Get-ServerConfig
Assert-ServerConfig -Config $config
$executablePath = Resolve-ServerExecutablePath -ConfiguredPath $config.ExecutablePath
if (-not (Test-Path -LiteralPath $executablePath -PathType Leaf)) { throw "Server executable not found: $executablePath" }
$existing = Get-TrackedServerProcess
if ($existing) { Write-Host "Server is already running. PID=$($existing.Id)"; exit 0 }
$maintenance = Get-MaintenanceFlagPath
if (Test-Path -LiteralPath $maintenance) { Remove-Item -LiteralPath $maintenance -Force }
$logPath = Join-Path (Get-LogDirectory) "LastFPSServer-$(Get-Date -Format 'yyyyMMdd-HHmmss').log"
$mapUrl = if (([string]$config.LobbyMap).Contains('?')) { [string]$config.LobbyMap } else { "$($config.LobbyMap)?listen" }
$arguments = @($mapUrl, '-server', "-port=$([int]$config.GamePort)", '-log', "-abslog=$logPath") + @(Get-ConfigValue -Config $config -Key 'AdditionalArguments' -Default @())
# 상시 구동은 창을 숨기고, 수동 디버깅 때만 설정으로 창을 띄운다.
$windowStyle = if ([bool](Get-ConfigValue -Config $config -Key 'ShowServerWindow' -Default $false)) { 'Normal' } else { 'Hidden' }
$process = Start-Process -FilePath $executablePath -ArgumentList $arguments -WorkingDirectory (Split-Path -Parent $executablePath) -WindowStyle $windowStyle -PassThru
$process.Id | Set-Content -LiteralPath (Get-ServerPidPath) -Encoding Ascii
Start-Sleep -Seconds 3
if ($process.HasExited)
{
    Remove-Item -LiteralPath (Get-ServerPidPath) -Force -ErrorAction SilentlyContinue
    throw "Server exited immediately after startup. Check the log: $logPath"
}
Write-Host "LastFPS server started. PID=$($process.Id), UDP=$([int]$config.GamePort)"
Write-Host "Log: $logPath"
