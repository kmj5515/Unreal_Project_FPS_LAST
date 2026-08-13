param([switch]$SkipWait)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$config = Import-PowerShellDataFile -LiteralPath (Join-Path $root 'Controller.config.psd1')
if ($config.MacAddress -eq 'AA-BB-CC-DD-EE-FF') { throw "Set 'MacAddress' in Controller.config.psd1 before running this script." }
$normalizedMac = ([string]$config.MacAddress) -replace '[^0-9A-Fa-f]', ''
if ($normalizedMac.Length -ne 12) { throw "Invalid MAC address: $($config.MacAddress)" }
[byte[]]$macBytes = for ($index = 0; $index -lt 12; $index += 2) { [Convert]::ToByte($normalizedMac.Substring($index, 2), 16) }
[byte[]]$packet = (,[byte]0xFF * 6) + ($macBytes * 16)
$udp = [System.Net.Sockets.UdpClient]::new()
try
{
    $udp.EnableBroadcast = $true
    $udp.Connect([string]$config.BroadcastAddress, [int]$config.WakePort)
    1..3 | ForEach-Object { [void]$udp.Send($packet, $packet.Length); Start-Sleep -Milliseconds 250 }
}
finally { $udp.Dispose() }
Write-Host "WOL magic packet sent: $($config.MacAddress)"
if ($SkipWait) { exit 0 }
$deadline = [DateTime]::UtcNow.AddSeconds([int]$config.WaitTimeoutSeconds)
$ping = [System.Net.NetworkInformation.Ping]::new()
try
{
    while ([DateTime]::UtcNow -lt $deadline)
    {
        try
        {
            if ($ping.Send([string]$config.ServerAddress, 1000).Status -eq [System.Net.NetworkInformation.IPStatus]::Success)
            { Write-Host "Server PC is responding: $($config.ServerAddress)"; exit 0 }
        }
        catch { Write-Verbose $_.Exception.Message }
        Start-Sleep -Seconds ([int]$config.PingIntervalSeconds)
    }
}
finally { $ping.Dispose() }
throw "Server PC did not respond before timeout: $($config.ServerAddress)"
