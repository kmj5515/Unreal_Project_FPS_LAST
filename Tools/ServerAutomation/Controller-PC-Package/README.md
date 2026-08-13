# 제어·게임 PC에 둘 패키지

이 폴더는 서버 컴퓨터가 아닌 현재 게임 컴퓨터에 둔다. `Server-PC-Package`는 이 컴퓨터에 필요하지 않다.

1. `Controller.config.psd1`에 서버 PC의 유선 MAC 주소와 고정 내부 IP를 입력한다.
2. `ClientExecutablePath`를 이 컴퓨터의 LastFPS 클라이언트 경로로 변경한다.
3. 서버만 켜려면 `01-Wake-Server.bat`을 실행한다.
4. 서버를 켜고 게임까지 실행하려면 `02-Wake-And-Connect.bat`을 실행한다.

외부 인터넷 사용자는 WOL 스크립트가 필요하지 않으며, 게임 클라이언트가 공인 IP 또는 DDNS 주소로 직접 접속하면 된다.
