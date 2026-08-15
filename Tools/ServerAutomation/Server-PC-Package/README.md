# 서버 PC에 복사할 패키지

이 폴더 전체만 서버 컴퓨터에 복사한다. `Controller-PC-Package`는 서버 컴퓨터에 필요하지 않다.

서버 PC는 마스터 로비(`/Game/Maps/Network/MasterLobbyMap`)를 상시 구동한다. 실제 플레이는 로비에서 방을 만든 각 호스트의 리슨 서버에서 진행되며, 이 서버는 방 목록만 유지한다.

1. 자동 생성된 배포 폴더 구조를 유지하면 `ExecutablePath`를 변경하지 않아도 된다.
2. `02-Install-Firewall-As-Admin.bat`을 실행한다.
3. `03-Start-Server.bat`으로 먼저 수동 실행을 확인한다.
4. 정상 확인 후 `01-Install-Startup-Task-As-Admin.bat`으로 부팅 자동 실행을 등록한다.
5. 유지보수나 종료 시 `04-Stop-Server.bat`을 실행한다. 예약 작업이 SYSTEM 계정으로 서버를 띄우므로 이 스크립트는 관리자 권한이 필요하다. 권한이 없으면 자동으로 UAC 승격을 요청하며, 승격을 거부하면 서버가 종료되지 않는다.

`Logs`와 `Runtime` 폴더는 실행 시 자동 생성된다.

## Server.config.psd1

| 키 | 설명 |
|---|---|
| `ExecutablePath` | 이 설정 파일이 있는 폴더 기준 상대 경로 또는 절대 경로 |
| `LobbyMap` | 상시 구동할 로비 맵. 빌드 스크립트의 `-LobbyMap`과 같아야 한다 |
| `GamePort` | UDP 게임 포트. 방화벽 규칙도 이 값을 사용한다 |
| `AdditionalArguments` | 추가 실행 인자. `-server`, `-log`, `-port`, `-abslog`은 스크립트가 이미 붙이므로 넣지 않는다 |
| `ShowServerWindow` | 수동 디버깅으로 콘솔 창을 볼 때만 `$true`. 기본값은 `$false` |
| `DriveRemoteRoot` | 빌드 배포용 rclone 리모트 경로. 개발 PC의 업로드 위치와 같아야 한다 |
| `InstallDirectory` | 내려받은 빌드를 풀어 넣을 폴더. `ExecutablePath`가 이 폴더 안을 가리켜야 한다 |
| `RclonePath` | `rclone.exe`가 PATH에 없을 때만 전체 경로를 지정한다 |

## Google Drive로 빌드 받기

서버 빌드 폴더를 USB나 원격 데스크톱으로 직접 옮기는 대신 `05-Update-Build.bat`으로 Google Drive에서 내려받을 수 있다.

준비 (서버 PC에서 한 번만):

1. `winget install Rclone.Rclone`으로 rclone을 설치한다.
2. `rclone config`로 Google Drive 리모트를 만든다. 이름은 `Server.config.psd1`의 `DriveRemoteRoot` 앞부분과 같아야 한다.
   - 서버 PC에 브라우저를 쓸 수 없으면 개발 PC에서 인증한 뒤 `%APPDATA%\rclone\rclone.conf`를 서버 PC의 같은 경로로 복사한다.
   - 예약 작업(SYSTEM)에서 실행할 때는 SYSTEM 계정의 프로필에도 같은 파일이 있어야 한다.
3. `05-Update-Build.bat`을 실행한다.

동작 순서는 버전 확인 → 최신이면 종료 → 아카이브 다운로드 → 서버 정지 → 폴더 교체 → 실행 중이었다면 재시작이다. 교체 전에 새 폴더의 실행 파일 존재를 확인하고, 이전 설치는 교체가 끝난 뒤에 삭제하므로 실패해도 기존 서버가 남는다.

- `05-Update-Build.bat -Force`: 버전이 같아도 다시 내려받아 교체한다.
- `05-Update-Build.bat -NoRestart`: 교체 후 서버를 다시 켜지 않는다.

설치된 버전은 `Runtime\installed-build.txt`에 기록된다.

예약 작업은 SYSTEM 계정으로 실행되므로 `ShowServerWindow = $true`로 두어도 사용자 데스크톱에는 창이 보이지 않는다. 창이 없다는 이유만으로 서버가 꺼졌다고 판단하지 말고 `Logs` 폴더와 `Get-NetUDPEndpoint -LocalPort 7777`로 확인한다.
