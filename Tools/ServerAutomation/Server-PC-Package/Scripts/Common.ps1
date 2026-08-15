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
    if ([string]::IsNullOrWhiteSpace([string]$Config.ExecutablePath))
    {
        throw "Set 'ExecutablePath' in Server.config.psd1 before running this script."
    }
    # LobbyMap이 비어 있으면 맵 없는 URL로 서버가 기동돼 원인 파악이 어려워진다.
    if ([string]::IsNullOrWhiteSpace([string]$Config.LobbyMap))
    {
        throw "Set 'LobbyMap' in Server.config.psd1 before running this script."
    }
    if ([int]$Config.GamePort -le 0 -or [int]$Config.GamePort -gt 65535)
    {
        throw "Set a valid 'GamePort' (1-65535) in Server.config.psd1."
    }
}

# 설정에 없는 키를 StrictMode에서 안전하게 읽는다.
function Get-ConfigValue
{
    param(
        [Parameter(Mandatory = $true)]$Config,
        [Parameter(Mandatory = $true)][string]$Key,
        $Default = $null
    )
    if ($Config.ContainsKey($Key)) { return $Config[$Key] }
    return $Default
}

# 설정 파일의 상대 경로는 모두 패키지 폴더 기준으로 해석한다.
function Resolve-PackageRelativePath
{
    param([Parameter(Mandatory = $true)][string]$ConfiguredPath)
    if ([System.IO.Path]::IsPathRooted($ConfiguredPath)) { return $ConfiguredPath }
    return [System.IO.Path]::GetFullPath((Join-Path (Get-PackageRoot) $ConfiguredPath))
}

function Resolve-ServerExecutablePath
{
    param([Parameter(Mandatory = $true)][string]$ConfiguredPath)
    return Resolve-PackageRelativePath -ConfiguredPath $ConfiguredPath
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
