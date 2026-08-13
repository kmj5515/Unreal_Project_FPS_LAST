# 서버 PC에 복사할 패키지

이 폴더 전체만 서버 컴퓨터에 복사한다. `Controller-PC-Package`는 서버 컴퓨터에 필요하지 않다.

1. `Server.config.psd1`의 `ExecutablePath`를 서버 빌드의 실제 경로로 변경한다.
2. `02-Install-Firewall-As-Admin.bat`을 실행한다.
3. `01-Install-Startup-Task-As-Admin.bat`을 실행한다.
4. `03-Start-Server.bat`으로 먼저 수동 실행을 확인한다.

`Logs`와 `Runtime` 폴더는 실행 시 자동 생성된다.
