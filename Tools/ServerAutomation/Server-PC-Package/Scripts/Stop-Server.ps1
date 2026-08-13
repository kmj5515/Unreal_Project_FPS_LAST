. (Join-Path $PSScriptRoot 'Common.ps1')
'maintenance' | Set-Content -LiteralPath (Get-MaintenanceFlagPath) -Encoding Ascii
$process = Get-TrackedServerProcess
if (-not $process) { Write-Host 'No tracked LastFPS server process was found.'; exit 0 }
$config = Get-ServerConfig
$expectedName = [System.IO.Path]::GetFileNameWithoutExtension([string]$config.ExecutablePath)
if ($process.ProcessName -ne $expectedName) { throw "PID file points to a different process. Refusing to stop it: PID=$($process.Id)" }
Stop-Process -Id $process.Id
if (-not $process.WaitForExit(15000)) { throw "Server did not stop before timeout. PID=$($process.Id)" }
Remove-Item -LiteralPath (Get-ServerPidPath) -Force -ErrorAction SilentlyContinue
Write-Host "LastFPS server stopped. PID=$($process.Id)"
