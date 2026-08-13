Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$config = Import-PowerShellDataFile -LiteralPath (Join-Path $root 'Controller.config.psd1')
if (([string]$config.ClientExecutablePath).Contains('D:\LastFPSClient')) { throw "Set 'ClientExecutablePath' in Controller.config.psd1 before running this script." }
if (-not (Test-Path -LiteralPath $config.ClientExecutablePath -PathType Leaf)) { throw "Client executable not found: $($config.ClientExecutablePath)" }
& (Join-Path $PSScriptRoot 'Wake-Server.ps1')
Start-Sleep -Seconds ([int]$config.ServerStartupGraceSeconds)
$address = "$($config.ServerAddress):$([int]$config.GamePort)"
Start-Process -FilePath $config.ClientExecutablePath -ArgumentList (@($address) + @($config.ClientAdditionalArguments)) -WorkingDirectory (Split-Path -Parent $config.ClientExecutablePath)
Write-Host "LastFPS client started: $address"
