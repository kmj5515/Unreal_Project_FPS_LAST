. (Join-Path $PSScriptRoot 'Common.ps1')
$taskName = 'LastFPS Dedicated Server'
$supervisorPath = Join-Path $PSScriptRoot 'Run-Supervisor.ps1'
$action = New-ScheduledTaskAction -Execute (Join-Path $PSHOME 'powershell.exe') -Argument "-NoProfile -ExecutionPolicy Bypass -File `"$supervisorPath`"" -WorkingDirectory (Get-PackageRoot)
$trigger = New-ScheduledTaskTrigger -AtStartup
$principal = New-ScheduledTaskPrincipal -UserId 'SYSTEM' -LogonType ServiceAccount -RunLevel Highest
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -RestartCount 3 -RestartInterval (New-TimeSpan -Minutes 1) -ExecutionTimeLimit ([TimeSpan]::Zero)
Register-ScheduledTask -TaskName $taskName -Action $action -Trigger $trigger -Principal $principal -Settings $settings -Description 'Starts LastFPS Server at boot and restarts it after an unexpected exit.' -Force | Out-Null
Write-Host "Scheduled task registered: $taskName"
