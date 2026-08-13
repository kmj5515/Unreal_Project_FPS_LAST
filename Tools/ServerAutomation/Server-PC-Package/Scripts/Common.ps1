Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-PackageRoot { return Split-Path -Parent $PSScriptRoot }

function Get-ServerConfig
{
    $path = Join-Path (Get-PackageRoot) 'Server.config.psd1'
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Configuration file not found: $path" }
    return Import-PowerShellDataFile -LiteralPath $path
}

function Get-RuntimeDirectory
{
    $path = Join-Path (Get-PackageRoot) 'Runtime'
    [System.IO.Directory]::CreateDirectory($path) | Out-Null
    return $path
}

function Get-LogDirectory
{
    $path = Join-Path (Get-PackageRoot) 'Logs'
    [System.IO.Directory]::CreateDirectory($path) | Out-Null
    return $path
}

function Get-ServerPidPath { return Join-Path (Get-RuntimeDirectory) 'LastFPSServer.pid' }
function Get-MaintenanceFlagPath { return Join-Path (Get-RuntimeDirectory) 'maintenance.flag' }

function Assert-ServerConfig
{
    param([Parameter(Mandatory = $true)]$Config)
    if ([string]::IsNullOrWhiteSpace([string]$Config.ExecutablePath) -or
        ([string]$Config.ExecutablePath).Contains('D:\LastFPSServer'))
    {
        throw "Set 'ExecutablePath' in Server.config.psd1 before running this script."
    }
}

function Get-TrackedServerProcess
{
    $pidPath = Get-ServerPidPath
    if (-not (Test-Path -LiteralPath $pidPath -PathType Leaf)) { return $null }
    $storedPid = 0
    if (-not [int]::TryParse((Get-Content -LiteralPath $pidPath -Raw).Trim(), [ref]$storedPid))
    {
        Remove-Item -LiteralPath $pidPath -Force
        return $null
    }
    $process = Get-Process -Id $storedPid -ErrorAction SilentlyContinue
    if (-not $process)
    {
        Remove-Item -LiteralPath $pidPath -Force
        return $null
    }
    return $process
}
