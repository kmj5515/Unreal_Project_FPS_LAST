. (Join-Path $PSScriptRoot 'Common.ps1')
$config = Get-ServerConfig

# 게임 포트와 Beacon 포트를 모두 열어야 한다.
# Beacon 포트가 막히면 서버는 정상 기동하지만 방 목록이 계속 비어 있다.
$ports = @(
    @{ Name = 'Game';   Port = [int]$config.GamePort },
    @{ Name = 'Beacon'; Port = [int](Get-ConfigValue -Config $config -Key 'BeaconPort' -Default 15000) }
)

foreach ($entry in $ports)
{
    $port = $entry.Port
    if ($port -le 0 -or $port -gt 65535)
    {
        throw "Invalid $($entry.Name) port in Server.config.psd1: $port"
    }

    $ruleName = "LastFPS Server UDP $port"
    if (Get-NetFirewallRule -DisplayName $ruleName -ErrorAction SilentlyContinue)
    {
        Write-Host "Firewall rule already exists: $ruleName ($($entry.Name))"
        continue
    }

    New-NetFirewallRule -DisplayName $ruleName -Direction Inbound -Action Allow -Protocol UDP -LocalPort $port -Profile Any | Out-Null
    Write-Host "Firewall inbound rule added: UDP $port ($($entry.Name))"
}
