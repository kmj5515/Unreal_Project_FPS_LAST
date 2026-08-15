Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# 개발 PC(업로드)와 서버 PC(다운로드)가 공유하는 배포 규약.
# 두 쪽 스크립트가 같은 파일 이름을 쓰도록 이 파일에서만 정의한다.
$script:DeployArchiveFileName = 'LastFPSServer-Windows.zip'
$script:DeployVersionFileName = 'LastFPSServer-Windows.version.txt'

function Get-DeployArchiveFileName { return $script:DeployArchiveFileName }
function Get-DeployVersionFileName { return $script:DeployVersionFileName }

# rclone은 배포 스크립트의 유일한 외부 의존성이므로 실패 시 설치 방법까지 알려준다.
function Resolve-RclonePath
{
    param([string]$ConfiguredPath)

    if (-not [string]::IsNullOrWhiteSpace($ConfiguredPath))
    {
        if (-not (Test-Path -LiteralPath $ConfiguredPath -PathType Leaf))
        {
            throw "rclone.exe not found at the configured path: $ConfiguredPath"
        }
        return (Resolve-Path -LiteralPath $ConfiguredPath).Path
    }

    $command = Get-Command 'rclone.exe' -ErrorAction SilentlyContinue
    if (-not $command)
    {
        throw 'rclone.exe not found. Install it (winget install Rclone.Rclone) or set RclonePath in the configuration file.'
    }
    return $command.Source
}

# 원격 경로는 'remote:folder/file' 형태여야 하므로 구분자를 슬래시로 통일한다.
function Join-RemotePath
{
    param(
        [Parameter(Mandatory = $true)][string]$RemoteRoot,
        [Parameter(Mandatory = $true)][string]$Leaf
    )

    $trimmed = $RemoteRoot.TrimEnd('/', '\')
    if ($trimmed.EndsWith(':')) { return "$trimmed$Leaf" }
    return "$trimmed/$Leaf"
}

# Windows PowerShell 5.1에는 [System.IO.Path]::GetRelativePath가 없어 직접 계산한다.
# 하위 경로가 아니면 $null을 돌려준다.
function Get-PathRelativeToBase
{
    param(
        [Parameter(Mandatory = $true)][string]$BaseDirectory,
        [Parameter(Mandatory = $true)][string]$FullPath
    )

    $base = [System.IO.Path]::GetFullPath($BaseDirectory).TrimEnd('\') + '\'
    $target = [System.IO.Path]::GetFullPath($FullPath)
    if (-not $target.StartsWith($base, [System.StringComparison]::OrdinalIgnoreCase)) { return $null }
    return $target.Substring($base.Length)
}

function Invoke-Rclone
{
    param(
        [Parameter(Mandatory = $true)][string]$RclonePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [string]$FailureMessage = 'rclone command failed.'
    )

    & $RclonePath @Arguments
    if ($LASTEXITCODE -ne 0)
    {
        throw "$FailureMessage ExitCode=$LASTEXITCODE"
    }
}
