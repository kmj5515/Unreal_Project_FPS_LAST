@{
    ExecutablePath = '..\Windows\LastFPS\Binaries\Win64\LastFPS.exe'
    # 서버 PC가 상시 구동할 마스터 로비 맵. Build-MasterLobbyServer.ps1의 -LobbyMap과 같아야 한다.
    LobbyMap = '/Game/Maps/Network/MasterLobbyMap'
    GamePort = 7777
    # 호스트가 방 정보를 보고하는 Beacon 포트. DefaultGame.ini의 BeaconPort와 같아야 하며
    # 방화벽과 공유기 포트포워딩도 이 포트를 열어야 방 목록이 채워진다.
    BeaconPort = 15000
    StartupDelaySeconds = 20
    RestartDelaySeconds = 10
    # -server, -log, -port, -abslog은 Start-Server.ps1이 이미 붙이므로 여기에 중복으로 넣지 않는다.
    AdditionalArguments = @('-unattended', '-NoSound', '-NullRHI')
    # 수동 디버깅으로 콘솔 로그 창을 보고 싶을 때만 $true. 예약 작업(SYSTEM)에서는 창이 보이지 않는다.
    ShowServerWindow = $false

    # --- 빌드 배포 (05-Update-Build.bat) ---
    # 개발 PC의 Publish-ServerBuild.ps1이 올린 위치와 같아야 한다. 'rclone config'로 만든 리모트 이름을 쓴다.
    DriveRemoteRoot = 'gdrive:LastFPS/ServerBuild'
    # 내려받은 빌드를 풀어 넣을 폴더. ExecutablePath가 이 폴더 안을 가리켜야 한다.
    InstallDirectory = '..\Windows'
    # PATH에 rclone.exe가 없을 때만 전체 경로를 지정한다.
    RclonePath = ''
}
