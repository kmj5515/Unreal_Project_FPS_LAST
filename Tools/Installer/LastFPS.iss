; LastFPS 클라이언트 설치 프로그램 (Inno Setup 6)
; Tools\MakeClientInstaller.bat 이 ISCC로 컴파일한다. 직접 열어 컴파일해도 동일하다.

#define MyAppName "LastFPS"
#define MyAppExeName "LastFPS.exe"
; 버전은 CI/수동 배포에서 덮어쓸 수 있게 외부 정의를 허용한다.
#ifndef MyAppVersion
  #define MyAppVersion "1.0.0"
#endif
; 저장소 위치가 바뀌어도 되게 이 .iss 기준 상대 경로로 패키지를 찾는다.
#define SourceDir "..\..\PackagedClient\Windows"

[Setup]
AppId={{7F3C1E64-9B4A-4D2E-9F51-2A6B8C0D1E77}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
OutputDir=..\..\Installer
OutputBaseFilename=LastFPS_Setup_{#MyAppVersion}
; 클라이언트 바이너리가 64비트 전용이라 32비트 환경에서는 설치를 막는다.
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; pak/ucas는 이미 압축되어 있어 max로 올려도 시간만 늘고 크기는 거의 그대로다.
Compression=lzma2/normal
SolidCompression=yes
WizardStyle=modern
; 관리자 권한은 Program Files 설치와 vc_redist에만 필요하다. 사용자 폴더 설치를 막지 않도록
; 권한 선택을 허용한다. /CURRENTUSER 로 실행하면 UAC 없이 설치된다.
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog commandline
DisableProgramGroupPage=yes
; 출력이 2GB를 넘어 컴파일이 실패하면 아래 두 줄을 켠다. Setup.exe + .bin 으로 쪼개진다.
;DiskSpanning=yes
;DiskSliceSize=max

[Languages]
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
; LastFPS.pdb(678MB)는 디버그 심볼이라 플레이어에게 필요 없다. 크래시 분석용으로는 빌드 머신 사본을 쓴다.
; Saved는 빌드 머신에서 테스트 실행할 때 생긴 로그·크래시 덤프라 배포물에 포함되면 안 된다.
Source: "{#SourceDir}\*"; DestDir: "{app}"; Excludes: "*.pdb,Manifest_DebugFiles_Win64.txt,\LastFPS\Saved,\Engine\Saved"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
; UE 필수 런타임. 이미 깔린 PC에서는 건너뛴다. (VCRedistMissing 참고)
Filename: "{app}\Engine\Extras\Redist\en-us\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing prerequisites..."; Flags: waituntilterminated; Check: VCRedistMissing
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent

[Code]
// 이미 런타임이 깔린 PC에서 vc_redist를 실행하면 얻는 것 없이 관리자 권한만 요구하므로
// 64비트 뷰의 등록 키로 설치 여부를 먼저 확인한다. 32비트 OS는 ArchitecturesAllowed가 이미 걸러낸다.
function VCRedistMissing: Boolean;
begin
  Result := not RegValueExists(HKEY_LOCAL_MACHINE_64,
    'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\X64', 'Installed');
end;

[UninstallDelete]
; 실행 중 생성되는 로그/설정은 설치 목록에 없어 남으므로 제거 시 정리한다.
Type: filesandordirs; Name: "{app}\LastFPS\Saved"
Type: filesandordirs; Name: "{app}\Engine\Saved"
