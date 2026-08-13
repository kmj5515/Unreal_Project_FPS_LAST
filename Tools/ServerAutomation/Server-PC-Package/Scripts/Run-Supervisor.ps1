. (Join-Path $PSScriptRoot 'Common.ps1')
$config = Get-ServerConfig
Start-Sleep -Seconds ([int]$config.StartupDelaySeconds)
while ($true)
{
    if (Test-Path -LiteralPath (Get-MaintenanceFlagPath)) { Start-Sleep -Seconds 5; continue }
    try
    {
        & (Join-Path $PSScriptRoot 'Start-Server.ps1')
        $process = Get-TrackedServerProcess
        if ($process)
        {
            $process.WaitForExit()
            Remove-Item -LiteralPath (Get-ServerPidPath) -Force -ErrorAction SilentlyContinue
        }
    }
    catch
    {
        Add-Content -LiteralPath (Join-Path (Get-LogDirectory) 'Supervisor.log') -Value "[$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')] $($_.Exception.Message)" -Encoding UTF8
    }
    Start-Sleep -Seconds ([int]$config.RestartDelaySeconds)
}
