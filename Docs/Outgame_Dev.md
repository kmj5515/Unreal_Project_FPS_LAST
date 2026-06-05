# 아웃게임 개발 체크리스트

> 마지막 업데이트: 2026-06-05  
> 기준 브랜치: `kmj-dev-roll`  
> 전체 완성도: **약 22%** (Phase 1 완료, Phase 3-1 Hub 기반 구조 + NPC 에디터 작업 완료)

> **최근 변경 (06-04)**
> - `BP_NPC_Quartermaster`, `WBP_NPCMarker` 에디터 작업 완료 → Phase 3-1 NPC 상호작용 실제 배치 가능
> - 고아 BP 정리: `BP_CharacterSelect_PlayerController`, `BP_MainMenu_LastFPS_PlayerController` 삭제 (`1c522b5`)
> - `WBP_Scoreboard` / `WBP_ScoreRow` 존재 확인 — C++ 백킹(`ULastFPSScoreboardWidget`) 미구현 BP 전용

---

## 범례

| 기호 | 의미 |
|---|---|
| ✅ | 완료 |
| 🔨 | 진행중 (일부 구현) |
| ⬜ | 미시작 |
| ⚠️ | 구현 있으나 구조 문제 |

---

## 미해결 이슈

> 작업 전 확인 필요

- [ ] **`BP_PC_Hub` WASD 이동 불가** ⚠️
  - 원인: `WBP_Lobby`(LobbyWidget)가 활성화될 때 CommonUI가 입력 모드를 `ECommonInputMode::Menu`로 강제 전환
  - 로그: `LogUIActionRouter: InputMode → ECommonInputMode::Menu`
  - 해결 방향: `ULastFPSLobbyWidget::GetDesiredInputConfig()` 오버라이드 → `ECommonInputMode::Game` 반환
  - > 의도적으로 롤백됨 — 추후 적용 여부 결정 필요

---

## Phase 1 — 기반 구조

### 1-1. UI 프레임워크

- [x] **GameUIPolicy + Layer Stack**
  - `LastFPSUIPolicy`, `LastFPSPrimaryGameLayout`, `LastFPSUIManagerSubsystem`
  - 4계층 (Game / GameMenu / Menu / Modal) C++ 자동 생성
  - WBP `BindWidgetOptional`로 선택적 오버라이드 가능

- [x] **CommonActivatableWidget 베이스**
  - `LastFPSActivatableWidget` — `NativeOnHandleBackAction` 오버라이드
  - ESC / 뒤로가기 자동 처리

- [x] **공통 버튼 (`UCommonButtonBase`) 교체 — C++ 완료** 🔨
  - [x] `ULastFPSButtonBase : UCommonButtonBase` 신설 (`UI/LastFPSButtonBase.h`)
  - [x] 5개 위젯 BindWidget 타입 `UButton` → `ULastFPSButtonBase` 전환
    - `MainMenu`(3), `CharacterSelect`(4), `Lobby`(6), `Confirm`(2), `Notice`(1)
  - [x] 바인딩 방식 `OnClicked.AddDynamic` → `OnClicked().AddUObject` 전환, 빌드 검증 완료
  - [ ] **에디터 작업 남음** ⚠️ — 각 WBP의 `Button` 위젯을 `ULastFPSButtonBase` 기반 위젯으로 교체해야 실제 연결됨
    - `BindWidgetOptional`이라 크래시는 없으나, 교체 전까지 해당 버튼 null → 클릭 무동작
    - 권장: `WBP_LastFPSButton`(ULastFPSButtonBase 상속) 1개 제작 후 5개 WBP에서 버튼 교체

- [x] **공통 팝업 베이스 (Confirm / Notice)**
  - `LastFPSModalDialogBase` → `LastFPSConfirmWidget`, `LastFPSNoticeWidget`
  - `FOnLastFPSConfirmResult` 델리게이트, `NativeOnHandleBackAction` 처리

- [x] **화면 전환 / 로딩 화면**
  - `LastFPSTravelTypes`, `LastFPSGameInstance` Travel System
  - `ExecuteServerTravel` — 맵 URL 검증, 1틱 지연, PIE/Standalone 분기
  - `LastFPSLoadingScreenWidget` — 이벤트 드리븐 업데이트 (Tick 폴링 제거 완료)

### 1-2. 진입 플로우

- [x] **메인메뉴 화면** (`LastFPSMainMenuWidget`)
  - Start 버튼 → ~~캐릭터 선택~~ **Hub 직행** (임시 — 캐릭터 구현 완료 후 원복)
  - Quit 버튼 → Confirm 팝업 → 종료
  - [ ] Settings 버튼 구현 필요 (선언만 있음)

- [ ] **캐릭터 선택창 고도화** 🔨
  - [x] `LastFPSCharacterSelectWidget` — 3카드 선택, Confirm/Back 동작
  - [x] `LastFPSCharacterCardWidget` — `SetSelected(bool)` BlueprintNativeEvent
  - [ ] `CharacterNames` / `CharacterRoles` 위젯 직박 → DataAsset 연동으로 교체
  - > 인게임 캐릭터 구현 완료 후 진행

- [x] **로비 화면** (`LastFPSLobbyWidget`) — Game Layer
  - `TB_PlayerName`, `Button_StartMatch`, `Button_Inventory`, `Button_Missions`
  - `Button_Shop`, `Button_Settings`, `Button_BackToMain`
  - 미구현 버튼 클릭 시 "준비 중" 공지 표시
  - `PlayerController.bPushHubOnBeginPlay = true` → HubMap 진입 시 자동 표시
  - **에디터 작업**: `WBP_Lobby` 생성 → `BP_PC_Hub`에 `HubWidgetClass` 할당

- [ ] **설정 화면** ⬜
  - 그래픽 / 사운드 / 입력 3탭
  - `GameUserSettings` 연동

---

## Phase 2 — 핵심 루프

### 2-1. 캐릭터(계승자) 관리

> 인게임 팀 작업 완료 후 진행

- [ ] **캐릭터 DataAsset 신설** ⬜
  - `ULastFPSCharacterDefinition : UPrimaryDataAsset`
  - 포함 필드: `DisplayName`, `NPCRole`, `PawnClass`, `Icon`, `Description`
  - GameMode 단독 소유 → PC / 위젯은 GameMode 경유 조회

- [ ] **계승자 관리 화면** ⬜
  - 보유 캐릭터 목록, 스탯 표시, 스킨 프리뷰

- [ ] **아르케 조율 / 성장 시스템** ⬜

### 2-2. 장비 / 인벤토리

- [ ] **아이템 DataTable 설계** ⬜
  - `FLastFPSItemData : FTableRowBase`

- [ ] **인벤토리 UI** ⬜
  - 프로토타입: 고정 슬롯 그리드로 먼저 구현
  - > 아이템 수 늘면 `UListView` / `UTileView` 가상화로 교체

- [ ] **모듈 시스템 UI** ⬜

---

## Phase 3 — 서브 시스템

### 3-1. Hub 월드

- [x] **로비 UI** → Phase 1-2 완료

- [x] **NPC 상호작용 구조**
  - `ILastFPSInteractable` (`Hub/ILastFPSInteractable.h`) — `Interact()`, `GetInteractionLabel()`
  - `ALastFPSNPCBase` — `USphereComponent` 범위 감지, `UWidgetComponent` 3D 마커
  - `ULastFPSNPCMarkerWidget` — 이름 / 역할 텍스트, `[F] 대화` 힌트 토글
  - `PlayerController` — `SetNearestInteractable` / `ClearNearestInteractable` / `TryInteract` (F키)
  - [x] **에디터 작업 완료**: `WBP_NPCMarker` + `BP_NPC_Quartermaster` 생성 완료
  - > 추가 NPC(역할별) 배치 시 `BP_NPC_Quartermaster` 복제 후 메시/`DisplayName`/`NPCRole` 변경

- [ ] **NPC 대화 UI** ⬜
  - 대화 텍스트 / 선택지
  - NPC별 대화 DataTable

### 3-2. 미션 / 퀘스트

- [ ] **퀘스트 데이터 구조** ⬜ — `FLastFPSQuestData : FTableRowBase`
- [ ] **퀘스트 목록 UI** ⬜ — 진행중 / 완료 탭
- [ ] **HUD 퀘스트 트래커** ⬜ — 현재 목표 1~3개

### 3-3. 파티 / 매칭

- [ ] **파티 UI** ⬜ — 파티원 슬롯 (최대 4인)
- [ ] **매칭 UI** ⬜ — `IOnlineSession` 연동 설계 선행 필요

### 3-4. 제작

- [ ] **제작 시스템 UI** ⬜

---

## Phase 4 — 피니시

- [ ] **매치 결과 화면** 🔨
  - `PlayerState` 통계 이미 수집 중 (킬/뎃/어시스트/힐/딜)
  - `WBP_Scoreboard` / `WBP_ScoreRow` 에디터에 존재하나 C++ 백킹 없음 → `ULastFPSScoreboardWidget` + `ULastFPSScoreRowWidget` 신설 필요
  - PlayerState 배열 바인딩 + `UListView` row 위젯 연동

- [ ] **상점 (유료 / 무료)** ⬜
- [ ] **시즌 패스** ⬜
- [ ] **도전과제 / 업적** ⬜

---

## 기술 부채

### 즉시 처리

- [x] **`UCommonButtonBase` 교체 — C++ 완료** | 남은 작업: 에디터 WBP 버튼 교체
  - `ULastFPSButtonBase` 신설, 5개 위젯 전환 완료 (Phase 1-1 참고)

- [ ] **`ULastFPSCharacterDefinition` DataAsset 신설** | 난이도: 중
  - 통합 대상: `CharacterNames[]` (위젯), `SelectableCharacterClasses[]` (PC), `CharacterPawnClasses[]` (GameMode)
  - 캐릭터 1개 추가 시 현재 3곳 동시 수정 필요

- [ ] **`PlayerController.SelectableCharacterClasses` 제거** | 난이도: 하
  - DataAsset 신설과 세트로 처리

### 나중에 처리

- [ ] **밸런스 수치 DataAsset화** | 난이도: 하
  - `LastFPSPlayerState.h:19` — `UltimateKillsRequired`, `UltimateKillHealAmount` 등 constexpr

- [ ] **위젯 클래스 `TSoftClassPtr` 전환** | 난이도: 중
  - `LastFPSPlayerController.h` — 5종 위젯 클래스 하드 레퍼런스

- [ ] **맵 경로 `/Test/` 제거** | 난이도: 하
  - `LastFPSGameInstance.h:61-67` — 릴리즈 전 정리

- [ ] **RetryPushTimer 중복 제거** | 난이도: 하
  - `LastFPSPlayerController.h` — 타이머 4종(HUD/MainMenu/CharSelect/Hub) 동일 패턴

---

## 개발 순서 가이드

```
[지금 당장]
  UCommonButtonBase 교체 (하, 2~3일)  ← 모든 위젯 입력 라우팅의 선결 과제
  NPC 대화 UI + DataTable (중, 3~5일)  ← NPC 에디터 작업 완료로 즉시 착수 가능
  설정 화면 (중, 5~7일)

[다음 주]
  퀘스트 데이터 구조 + 목록 UI (중, 3~5일)
  매치 결과 화면 — Scoreboard C++ 신설 (중, 2~3일, 통계 데이터 준비됨)

[인게임 팀 작업 완료 후]
  CharacterDefinition DataAsset 신설
  캐릭터 선택창 DataAsset 연동
  계승자 관리 화면

[마무리]
  인벤토리 UI / 모듈 UI
  파티 / 매칭 UI (IOnlineSession 설계 선행)
  매치 결과 화면 (통계 데이터 이미 준비됨)
  상점 / 시즌패스 / 도전과제
```

---

## CommonUI 레이어 & 입력 모드 참고

| 레이어 | 입력 모드 기본값 | 용도 |
|---|---|---|
| `Layer_Game` | `ECommonInputMode::Game` | 전투 HUD, 허브 오버레이 — WASD 통과 |
| `Layer_Menu` | `ECommonInputMode::Menu` | 메인메뉴, 전체화면 팝업 — WASD 차단 |
| `Layer_Modal` | `ECommonInputMode::Menu` | Confirm / Notice 팝업 |

> 위젯이 직접 `GetDesiredInputConfig()`를 오버라이드하면 레이어와 관계없이 모드를 강제함.  
> 허브처럼 "돌아다니면서 보는 UI"는 반드시 `Game` 모드로 설정해야 이동 가능.

---

## 구현된 시스템 참고 패턴

| 패턴 | 파일 | 설명 |
|---|---|---|
| Modal 팝업 표시 | `LastFPSPlayerController.cpp` `ShowConfirm()` | `PushWidgetToModalLayer<T>()` |
| 화면 전환 | `LastFPSGameInstance.cpp` `RequestTravelToDestination()` | Travel System 진입점 |
| 델리게이트 구독 / 해제 | `LastFPSLoadingScreenWidget.cpp` | `AddUObject` / `Remove(Handle)` |
| GAS 속성 바인딩 | `LastFPSHUDWidget.cpp` | `AttributeChangeDelegate` 이벤트 드리븐 |
| 서버 RPC + OnRep | `LastFPSPlayerController.h` | 캐릭터 선택 인덱스 동기화 |
| Layer Stack Push | `LastFPSPlayerController.cpp` `TryPushMainMenuToUILayout()` | `PushWidgetToLayerStack<T>()` |
| NPC 상호작용 | `LastFPSNPCBase.cpp` `HandleBeginOverlap()` | `SetNearestInteractable` → F키 → `Execute_Interact` |
