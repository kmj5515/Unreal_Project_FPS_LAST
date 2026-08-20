# LastFPS — UE 5.7 PvE 루터슈터

> Unreal Engine 5.7 · C++ · GAS 기반 3인칭 슈팅 게임
> 허브에서 정비 → 퀘스트 수주 → 던전 인카운터·보스 처치 → 결과·전리품까지 한 판이 온전히 돌아간다.

**팀 프로젝트 (프로그래머 2명)** — 전투 · 캐릭터 · 월드 · 개발 툴 담당
담당 범위와 공동 작업 구분은 기술 문서에 정리했습니다.

📄 **기술 문서: [Docs/Tech/LastFPS-기술문서.md](Docs/Tech/LastFPS-기술문서.md)** ← 설계 의사결정과 근거는 여기에 있습니다.

---

## 게임 개요

| 항목 | 내용 |
|---|---|
| 장르 | 3인칭 PvE 루터슈터 |
| 플랫폼 | PC (Windows) |
| 플레이 루프 | 허브 정비 → 퀘스트 수주 → 던전 인카운터·보스 → 결과·드롭 |
| 엔진 | Unreal Engine 5.7 (C++ 주도, Blueprint는 에셋 조립·뷰에 한정) |

---

## 핵심 시스템

### 전투 — GAS

- **단일 데미지 진입점** — 무기·스킬·환경 피해가 모두 `LastFPSDamage::RollAndApplySetByCallerDamage` 한 함수를 지난다
- **쿨다운의 권위가 DataTable에 있다** — `GetCooldownTags`/`ApplyCooldown` 오버라이드로 GE 클래스가 아닌 데이터에서 읽는다
- **투사체 충돌 결과는 상속이 아니라 조합** — `ULastFPSProjectileImpactRule`(단일타격/광역/연쇄) 인스턴스를 에디터에서 조립. 신규 스킬 대부분이 C++ 변경 0줄
- **어트리뷰트** — Health, Stamina, UltimateGauge, AttackDamage, Defense, MoveSpeed + 메타 어트리뷰트 `Damage`
- 무기 오버히트(열 게이지 누적 → 발사 잠금 → 냉각), 히트스캔 판정, ADS

### 콘텐츠 · 성능

- **인카운터를 데이터로 조립** — 방 단위 목표(처치/방어/시간제한)를 Definition 조합으로 구성, 목표 유형 추가는 상속만으로 확장
- **오브젝트 풀링** — 투사체·이펙트·적을 풀에서 재사용. 풀 소진 시 일반 Spawn으로 폴백하고 경고는 버킷당 1회만 남긴다
- **비동기 로딩으로 히치 제거** — 소프트 참조 + AssetBundles, 콘텐츠 미준비 시 스폰을 버리지 않고 미룬 뒤 로드 완료에 맞춰 재개
- **퀘스트·드롭·인벤토리** — 수주부터 보상 지급까지 서브시스템으로 분리

### 데이터 기반 설계

- **3계층 분리** — 에셋 구성(Definition) / 밸런스 수치(DataTable) / 런타임 상태(Subsystem·Component)
- **접착제는 경로 문자열이 아니라 Gameplay Tag** — DataTable 조회가 태그 기반 카탈로그 단일 경로
- **Excel → CSV → DataTable 자동화** — 파이썬 툴 3종 + 전용 에디터 플러그인, 왕복 검증 스크립트로 변환 손실 검출
- 소프트 참조 + AssetBundles 기반 비동기 로딩
- 오브젝트 풀링 (풀 소진 시 일반 Spawn 폴백, 경고는 버킷당 1회만)

### UI · 연출

- **HUD는 View / Presenter 10종** (공동 구현) — 갱신을 데이터 성격에 맞춤(어트리뷰트 델리게이트 / 태그 이벤트 / 연속값만 틱)
- CommonUI 레이어 5종, 입력 config writer를 PlayerController 하나로 고정
- Sequencer 기반 컷신, Niagara VFX

### 개발 도구

- GAS 인스펙터 (Slate 기반, 스냅샷은 UObject 미참조 POD, 셰이핑 빌드에서 미생성)
- 자동화 테스트 10건, 콘솔 명령 7개
- 데디케이티드 서버 빌드·배포 자동화 (`RunUAT BuildCookRun` → rclone → 서버 PC 버전 비교 갱신)

---

## 기술 스택

| 분야 | 사용 기술 |
|---|---|
| 엔진 | Unreal Engine 5.7 |
| 언어 | C++ (코어) + Blueprint (에셋 조립·폴리싱) |
| 어빌리티 | Gameplay Ability System |
| 입력 | Enhanced Input |
| UI | CommonUI · CommonGame · UMG |
| 모듈화 | ModularGameplay · ModularGameplayActors |
| VFX · 연출 | Niagara · LevelSequence · MovieScene |
| 렌더링 | Lumen · Nanite |

---

## 문서

| 문서 | 내용 |
|---|---|
| **[기술 문서](Docs/Tech/LastFPS-기술문서.md)** | 담당 범위 · 시스템별 설계 판단 · 문제 해결 사례 · 회고 (단일 문서) |
| [에디터 세팅 체크리스트](Docs/Setup-Checklist.md) | 블루프린트 바인딩 규칙 (2026-05 기준 아카이브) |
| [포트폴리오 영상 기획안](Docs/PortfolioVideo_12min.md) | 12분 영상 구성 |

---

## 빌드 및 실행

### 요구 사항

- Unreal Engine 5.7
- Visual Studio 2022 (MSVC v143)
- Windows 10/11 64-bit

### 에디터 빌드

1. `LastFPS/LastFPS.uproject` 우클릭 → **Generate Visual Studio project files**
2. `LastFPS.sln` 열기
3. `Development Editor` | `Win64` 로 빌드
4. 에디터에서 `LastFPS.uproject` 오픈

> `LastFPS.Build.cs`를 수정한 뒤에는 **전체 리빌드**가 필요하다. Live Coding만으로는 링크 오류가 남는다.

### 실행

에디터에서 `Play` 또는 패키징된 클라이언트 실행. 시작 맵은 허브다.

### 서버 패키징·배포 (참고)

멀티플레이 서버 빌드·배포 자동화는 구축돼 있으나 **안정화가 끝나지 않아 시연 범위에 넣지 않는다.**
`Tools/ServerAutomation/Server-PC-Package/` 가 원본이며 배포 시 `Deploy_Server/`로 복사된다
(`Deploy_Server/`는 `.gitignore` 제외 대상이므로 스크립트는 `Tools/` 쪽을 볼 것).

상세 내용은 [기술 문서](Docs/Tech/LastFPS-기술문서.md)에 있다.

---

## 참고 자료

- [GAS Documentation (tranek)](https://github.com/tranek/GASDocumentation)
- [UE5 Enhanced Input System](https://docs.unrealengine.com/5.0/en-US/enhanced-input-in-unreal-engine/)
- [UE Network Compendium](https://cedric-neukirchen.net/Downloads/Compendium/UE4_Network_Compendium_by_Cedric_eXi_Neukirchen.pdf)
