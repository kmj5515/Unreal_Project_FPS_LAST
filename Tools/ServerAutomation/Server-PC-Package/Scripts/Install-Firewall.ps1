. (Join-Path $PSScriptRoot 'Common.ps1')
$port = [int](Get-ServerConfig).GamePort
$ruleName = "LastFPS Server UDP $port"
if (Get-NetFirewallRule -DisplayName $ruleName -ErrorAction SilentlyContinue) { Write-Host "Firewall rule already exists: $ruleName"; exit 0 }
New-NetFirewallRule -DisplayName $ruleName -Direction Inbound -Action Allow -Protocol UDP -LocalPort $port -Profile Any | Out-Null
Write-Host "Firewall inbound rule added: UDP $port"
