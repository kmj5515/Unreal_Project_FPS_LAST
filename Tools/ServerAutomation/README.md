# LastFPS 서버 자동화 분리 패키지

두 폴더는 서로 독립적입니다. 필요한 폴더 하나만 해당 컴퓨터에 복사하면 됩니다.

## 서버 컴퓨터에 복사

`Server-PC-Package` 폴더 전체를 서버 컴퓨터의 로컬 디스크에 복사합니다.

- 전용 서버 실행 및 감시
- Windows 시작 작업 등록
- UDP 게임 포트 방화벽 규칙 등록
- 서버 수동 시작 및 중지

복사 후 `Server.config.psd1`의 실행 파일과 로비 맵을 서버 컴퓨터 환경에 맞게 수정합니다.

## 현재 게임/제어 컴퓨터에 보관

`Controller-PC-Package` 폴더는 서버 컴퓨터에 넣을 필요가 없습니다.

- Wake-on-LAN으로 서버 컴퓨터 켜기
- 서버 응답 대기
- LastFPS 클라이언트로 직접 접속

`Controller.config.psd1`의 MAC 주소, 서버 주소, 클라이언트 실행 파일 경로를 수정합니다.

## 별도로 준비할 파일

자동화 스크립트와 별개로 서버 컴퓨터에는 Unreal Engine의 전용 서버 패키징 결과가 필요합니다. 해당 결과의 실행 파일 경로를 `Server.config.psd1`에 지정해야 합니다.

서버 컴퓨터가 상시 구동하는 마스터 로비 맵은 `/Game/Maps/Network/MasterLobbyMap`입니다. 이전 이름인 `PartyLobbyMap`은 이 맵으로 이름이 바뀌어 리다이렉터만 남아 있으므로 새로 참조하지 마세요.

패키징 결과는 `Build/Build-MasterLobbyServer.bat`으로 생성합니다. 산출물은 `Deploy_Server`에 떨어지며 `.gitignore` 대상이라 저장소에는 보관하지 않습니다. 이 자동화 폴더만 복사한다고 게임 서버 실행 파일이 만들어지지는 않으므로, 서버 컴퓨터에는 패키징 결과의 `Windows` 폴더 전체를 함께 복사하고 그 실행 파일 경로를 `Server.config.psd1`에 지정해야 합니다.

## Google Drive로 빌드 전달

`Windows` 폴더는 2GB가 넘고 파일 수가 많아 매번 수동 복사하기 어렵습니다. rclone과 Google Drive를 쓰면 아래 두 단계로 끝납니다.

1. 개발 PC: `Build/Publish-ServerBuild.bat` — `Windows` 폴더를 압축해 Drive에 올립니다.
2. 서버 PC: `Server-PC-Package/05-Update-Build.bat` — 새 버전일 때만 내려받아 서버를 정지·교체·재시작합니다.

두 스크립트는 `LastFPSServer-Windows.zip`과 `LastFPSServer-Windows.version.txt`라는 같은 파일 이름 규약을 `Server-PC-Package/Scripts/DeployCommon.ps1` 하나에서 공유합니다. 업로드는 아카이브를 먼저 올리고 버전 파일을 나중에 올려, 전송이 중간에 끊겨도 서버 PC가 미완성 빌드를 최신으로 오인하지 않습니다.

기본 리모트 경로는 양쪽 모두 `gdrive:LastFPS/ServerBuild`이며, 개발 PC는 `Publish-ServerBuild.bat -RemoteRoot ...`로, 서버 PC는 `Server.config.psd1`의 `DriveRemoteRoot`로 바꿉니다. rclone 설치와 인증 방법은 `Server-PC-Package/README.md`에 있습니다.

Google Drive 무료 용량은 15GB이고 압축본은 2GB 안팎이므로, 같은 파일 이름을 덮어쓰는 이 방식은 용량 문제가 없습니다.

## 같은 내부 망일 때 (권장)

서버 PC가 같은 LAN에 있으면 Drive를 거치지 않고 `Build/Publish-ServerBuild-Lan.bat`으로 바로 밀어넣는 편이 빠릅니다. 압축과 왕복 전송이 없고, `/MIR`이 바뀐 파일만 보내므로 두 번째 배포부터는 수 초에 끝납니다.

서버 PC에서 공유 폴더를 한 번 만들어 둡니다. 관리자 PowerShell에서:

```
New-SmbShare -Name LastFPSServer -Path C:\Users\pc\Desktop\Deploy_Server -FullAccess "$env:USERNAME"
```

배포 절차는 세 단계입니다.

1. 서버 PC: `04-Stop-Server.bat` (실행 중인 서버가 실행 파일을 잠그고 있습니다)
2. 개발 PC: `Build/Publish-ServerBuild-Lan.bat`
3. 서버 PC: `03-Start-Server.bat`

`-Destination`의 기본값은 `\\172.30.1.15\LastFPSServer\Windows`입니다. 서버 PC의 내부 망 주소가 바뀌면 인자로 지정하거나 이 기본값을 고쳐야 합니다.

개발 PC에서 공유에 처음 접근할 때는 서버 PC의 로컬 계정으로 인증해야 합니다. 한 번 연결해 두면 재부팅 전까지 유지됩니다.

```
net use \\172.30.1.15\LastFPSServer /user:172.30.1.15\pc *
```

`Access is denied`가 계속 나오면 서버 PC의 `pc` 계정에 암호가 설정돼 있는지 확인합니다. 윈도우는 기본적으로 빈 암호 계정의 네트워크 로그온을 차단합니다.

`/MIR`은 원본에 없는 파일을 대상에서 지웁니다. 대상을 바탕화면이나 프로필 폴더로 잘못 지정하면 그 안의 파일이 모두 삭제되므로, 스크립트는 대상 폴더에 `LastFPS.exe`가 없으면 미러링을 거부합니다. 또한 대상 실행 파일이 잠겨 있으면 서버가 아직 돌고 있다는 뜻이므로 복사를 시작하기 전에 중단합니다.

무엇이 오갈지 먼저 보고 싶으면 `-WhatIf`를 붙여 목록만 출력할 수 있습니다.

첫 배포처럼 대상 폴더가 아직 없을 때는 스크립트가 폴더를 만들며 전체를 복사합니다.
