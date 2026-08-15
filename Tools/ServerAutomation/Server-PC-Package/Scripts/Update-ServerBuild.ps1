param(
    # 버전이 같아도 강제로 다시 내려받아 교체한다. 설치 폴더가 손상됐을 때 사용한다.
    [switch]$Force,
    # 교체 후 서버를 자동으로 다시 켜지 않는다.
    [switch]$NoRestart
)

. (Join-Path $PSScriptRoot 'Common.ps1')
. (Join-Path $PSScriptRoot 'DeployCommon.ps1')

Add-Type -AssemblyName System.IO.Compression.FileSystem

$config = Get-ServerConfig

$remoteRoot = [string](Get-ConfigValue -Config $config -Key 'DriveRemoteRoot' -Default '')
if ([string]::IsNullOrWhiteSpace($remoteRoot))
{
    throw "Set 'DriveRemoteRoot' in Server.config.psd1 before running this script."
}

$rclone = Resolve-RclonePath -ConfiguredPath ([string](Get-ConfigValue -Config $config -Key 'RclonePath' -Default ''))

$installDirectory = Resolve-PackageRelativePath -ConfiguredPath ([string](Get-ConfigValue -Config $config -Key 'InstallDirectory' -Default '..\Windows'))
$installedVersionPath = Join-Path (Get-RuntimeDirectory) 'installed-build.txt'
$workDirectory = Join-Path (Get-RuntimeDirectory) 'Update'

$remoteVersionPath = Join-RemotePath -RemoteRoot $remoteRoot -Leaf (Get-DeployVersionFileName)
$remoteArchivePath = Join-RemotePath -RemoteRoot $remoteRoot -Leaf (Get-DeployArchiveFileName)

Write-Host "Checking the published build version on $remoteRoot ..."
$remoteVersion = (& $rclone 'cat' $remoteVersionPath)
if ($LASTEXITCODE -ne 0)
{
    throw "Reading the remote version file failed: $remoteVersionPath ExitCode=$LASTEXITCODE"
}
$remoteVersion = ([string]$remoteVersion).Trim()
if ([string]::IsNullOrWhiteSpace($remoteVersion))
{
    throw "The remote version file is empty: $remoteVersionPath"
}

$installedVersion = ''
if (Test-Path -LiteralPath $installedVersionPath -PathType Leaf)
{
    $installedVersion = (Get-Content -LiteralPath $installedVersionPath -Raw).Trim()
}

if (-not $Force -and $installedVersion -eq $remoteVersion)
{
    Write-Host "The server build is already up to date. Version=$installedVersion"
    exit 0
}

if (Test-Path -LiteralPath $workDirectory) { Remove-Item -LiteralPath $workDirectory -Recurse -Force }
[System.IO.Directory]::CreateDirectory($workDirectory) | Out-Null

$archivePath = Join-Path $workDirectory (Get-DeployArchiveFileName)
$extractDirectory = Join-Path $workDirectory 'Windows'

try
{
    Write-Host "Downloading the server build archive ..."
    Invoke-Rclone -RclonePath $rclone -FailureMessage 'Downloading the server build archive failed.' -Arguments @(
        'copyto', $remoteArchivePath, $archivePath, '--progress', '--retries=5'
    )

    Write-Host 'Extracting the server build archive ...'
    [System.IO.Compression.ZipFile]::ExtractToDirectory($archivePath, $extractDirectory)

    # 교체 직전에 검증해야 잘못된 아카이브로 기존 설치를 날리지 않는다.
    $executablePath = Resolve-ServerExecutablePath -ConfiguredPath $config.ExecutablePath
    $relativeExecutablePath = Get-PathRelativeToBase -BaseDirectory $installDirectory -FullPath $executablePath
    if (-not $relativeExecutablePath)
    {
        throw "'ExecutablePath' points outside 'InstallDirectory'. Fix Server.config.psd1: $executablePath"
    }
    if (-not (Test-Path -LiteralPath (Join-Path $extractDirectory $relativeExecutablePath) -PathType Leaf))
    {
        throw "The downloaded archive does not contain the configured server executable: $relativeExecutablePath"
    }

    $wasRunning = $null -ne (Get-TrackedServerProcess)
    if ($wasRunning)
    {
        Write-Host 'Stopping the running server before replacing the build ...'
        & (Join-Path $PSScriptRoot 'Stop-Server.ps1')
    }

    # 이전 설치를 먼저 옆으로 치우고 새 폴더를 넣는다. 덮어쓰기로는 삭제된 파일이 남는다.
    $retiredDirectory = "$installDirectory.old"
    if (Test-Path -LiteralPath $retiredDirectory) { Remove-Item -LiteralPath $retiredDirectory -Recurse -Force }
    if (Test-Path -LiteralPath $installDirectory) { Move-Item -LiteralPath $installDirectory -Destination $retiredDirectory }

    try
    {
        Move-Item -LiteralPath $extractDirectory -Destination $installDirectory
    }
    catch
    {
        # 교체에 실패하면 서버가 통째로 사라지므로 이전 설치를 즉시 되돌린다.
        if (-not (Test-Path -LiteralPath $installDirectory) -and (Test-Path -LiteralPath $retiredDirectory))
        {
            Move-Item -LiteralPath $retiredDirectory -Destination $installDirectory
        }
        throw
    }

    Remove-Item -LiteralPath $retiredDirectory -Recurse -Force -ErrorAction SilentlyContinue
    $remoteVersion | Set-Content -LiteralPath $installedVersionPath -Encoding Ascii -NoNewline
    Write-Host "Server build updated. Version=$remoteVersion"

    if ($wasRunning -and -not $NoRestart)
    {
        & (Join-Path $PSScriptRoot 'Start-Server.ps1')
    }
    elseif ($wasRunning)
    {
        # Stop-Server가 남긴 점검 플래그를 지우지 않으면 감시 스크립트가 서버를 다시 켜지 않는다.
        Write-Host 'The server is left stopped. Run 03-Start-Server.bat to bring it back up.'
    }
}
finally
{
    Remove-Item -LiteralPath $workDirectory -Recurse -Force -ErrorAction SilentlyContinue
}
