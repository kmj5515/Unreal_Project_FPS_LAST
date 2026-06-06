# 아웃게임 개발 체크리스트

> 마지막 업데이트: 2026-06-07
> 기준 브랜치: `kmj-dev-roll`
> 전체 완성도: **약 30%** (Phase 1 기반 + UI 라우팅 시스템 + PC 통합 완료)

> **설계 방침** — The First Descendant 참고 PvE 루터슈터지만 **"매치" 개념 없음**: 매치 결과/스코어보드, 로비 출격(StartMatch) **미사용**

> **UI 시스템 문서** — 구조/사용법 → [`UI_System.md`](UI_System.md)

> **최근 변경 (06-07) — UI 라우팅 시스템 + PC 통합** (커밋 `6a68d30`)
> - **태그 기반 화면 라우팅 신설** — `ScreenRegistry`(DataAsset) + `UIManagerSubsystem.OpenScreen(Tag)`. 화면 추가 = "위젯 + 레지스트리 행 + 태그"(코드 0줄)
> - **PlayerController 1개로 통합** — 화면별 push 메서드 전멸, 진입/ESC 화면을 GameMode가 결정. 맵별 PC 3종 → `BP_LastFPS_PlayerController` 1개
> - **허브 메뉴 = 온디맨드** (ESC 토글), **ESC 닫기**는 `ActivatableWidget` 포커스+`NativeOnKeyDown`로 처리
> - **상호작용 키 F → G** (캐릭터 궁극기 `IA_Ultimate`와 충돌 회피)
> - **MainMenu Start → 캐릭터 선택** (임시 Hub 직행 스킵 제거)
> - 정리: 고아 `BP_PC_Hub` 삭제, `WBP_Lobby`→`WBP_Hub` 리네임, RetryTimer 중복 통합
>
> **이전 변경 (06-05)** — 공통 버튼 CommonUI 전환(`ULastFPSButtonBase`), 로비 출격버튼 제거 / **(06-04)** NPC(`BP_NPC_Quartermaster`)·마커 에디터 완료

---

## 범례

| 기호 | 의미 |
|---|---|
| ✅ | 완료 |
| 🔨 | 진행중 (일부 구현) |
| ⬜ | 미시작 |
| ⚠️ | 구현 있으나 구조 문제 |

---

## Phase 1 — 기반 구조

### 1-1. UI 프레임워크

- [x] **GameUIPolicy + Layer Stack**
  - `LastFPSUIPolicy`, `LastFPSPrimaryGameLayout`, `LastFPSUIManagerSubsystem`
  - 4계층 (Game / GameMenu / Menu / Modal) C++ 자동 생성

- [x] **CommonActivatableWidget 베이스** — `LastFPSActivatableWidget`
  - `NativeOnHandleBackAction` + **ESC 닫기 폴백**(활성화 시 포커스 확보 + `NativeOnKeyDown`)

- [x] **태그 기반 화면 라우팅 시스템** ✅ (06-07) → 상세 `UI_System.md`
  - `ScreenRegistry`(DataAsset) + `ScreenTypes` + `UISettings`(DeveloperSettings)
  - `UIManagerSubsystem.OpenScreen / CloseScreen / IsScreenOpen`
  - `ContentScreenWidget` 풀스크린 베이스, `UI.Screen.*` 태그
  - PlayerController는 `OpenScreen(Tag)` 위임만 (얇은 리모컨)

- [x] **공통 버튼** (`ULastFPSButtonBase : UCommonButtonBase`) — WBP 5종 전환 완료

- [x] **공통 팝업 베이스 (Confirm / Notice)** — `LastFPSModalDialogBase` → Confirm/Notice

- [x] **화면 전환 / 로딩 화면** — `LastFPSGameInstance` Travel System + `LoadingScreenWidget`(이벤트 드리븐)

### 1-2. 진입 플로우

- [x] **메인메뉴** (`LastFPSMainMenuWidget`)
  - Start → **캐릭터 선택** (`RequestTravelToCharacterSelect`)
  - Quit → Confirm 팝업 → 종료
  - [ ] Settings 버튼 → `UI.Screen.Settings` 연결 (화면 제작 후)

- [ ] **캐릭터 선택창 고도화** 🔨
  - [x] `LastFPSCharacterSelectWidget` — 3카드 선택, Confirm/Back
  - [x] `LastFPSCharacterCardWidget` — `SetSelected(bool)`
  - [ ] `CharacterNames`/`CharacterRoles` 위젯 직박 → DataAsset 연동 (인게임 팀 작업 후)

- [x] **허브 메뉴** (`LastFPSLobbyWidget` = `WBP_Hub`) — **Menu 레이어 / 온디맨드**
  - `TB_PlayerName`, Inventory/Missions/Shop/Settings/BackToMain 버튼
  - **ESC로 토글** (GameMode `EscMenuScreenTag = UI.Screen.HubMenu`), ESC/Back/닫기로 닫힘
  - 버튼 → `OpenScreenOrNotice` (등록 화면 열기, 미등록이면 "준비 중" 공지)
  - > 출격 버튼 제거됨 — 미션 진입은 NPC/미션보드 별도 경로

- [ ] **설정 화면** ⬜ → `WBP_Settings`(`ContentScreenWidget` 상속) + `UI.Screen.Settings` (그래픽/사운드/입력, `GameUserSettings`)

---

## Phase 2 — 핵심 루프

### 2-1. 캐릭터(계승자) 관리
> 인게임 팀 작업 완료 후 진행
- [ ] **캐릭터 DataAsset 신설** ⬜ — `ULastFPSCharacterDefinition : UPrimaryDataAsset` (`DisplayName`/`NPCRole`/`PawnClass`/`Icon`/`Description`)
- [ ] **계승자 관리 화면** ⬜ — 보유 목록 / 스탯 / 스킨 프리뷰
- [ ] **아르케 조율 / 성장 시스템** ⬜

### 2-2. 장비 / 인벤토리
- [ ] **아이템 DataTable** ⬜ — `FLastFPSItemData : FTableRowBase`
- [ ] **인벤토리 UI** ⬜ — 고정 슬롯 그리드 프로토 → 추후 `UTileView` 가상화
- [ ] **모듈 시스템 UI** ⬜

---

## Phase 3 — 서브 시스템

### 3-1. Hub 월드

- [x] **허브 메뉴 UI** → Phase 1-2

- [x] **NPC 상호작용 구조**
  - `ILastFPSInteractable`, `ALastFPSNPCBase`(`USphereComponent` 감지 + `UWidgetComponent` 마커), `ULastFPSNPCMarkerWidget`
  - `PlayerController.TryInteract` (**G키**) → `Execute_Interact`
  - **`ScreenToOpen` 태그** — NPC에 화면 지정하면 G로 그 화면 오픈 (미등록/미지정이면 대화 공지)
  - [x] 에디터: `WBP_NPCMarker` + `BP_NPC_Quartermaster`(`ScreenToOpen=UI.Screen.Shop`)
  - > 추가 NPC: `BP_NPC_Quartermaster` 복제 후 메시/`DisplayName`/`NPCRole`/`ScreenToOpen` 변경

- [ ] **NPC 대화 UI** ⬜ — 대화 텍스트/선택지, NPC별 대화 DataTable

### 3-2. 미션 / 퀘스트
- [ ] **퀘스트 데이터 구조** ⬜ — `FLastFPSQuestData : FTableRowBase`
- [ ] **퀘스트 목록 UI** ⬜ / **HUD 퀘스트 트래커** ⬜

### 3-3. 파티 / 매칭 — ❌ 보류 (매치 개념 부재)
- [~] ~~파티 UI~~ → 존속 여부 재검토 / [x] ~~매칭 UI~~ → **제외**

### 3-4. 제작
- [ ] **제작 시스템 UI** ⬜

---

## Phase 4 — 피니시

- [x] ~~**매치 결과 화면 / 스코어보드**~~ → ❌ 설계 제외. `WBP_Scoreboard`/`WBP_ScoreRow` 미사용(안 씀)
- [ ] **상점 (유료 / 무료)** ⬜
- [ ] **시즌 패스** ⬜ / **도전과제 / 업적** ⬜

---

## 기술 부채

### 완료 ✅
- [x] `UCommonButtonBase` 교체 / **RetryPushTimer 중복 통합** / **위젯 하드레퍼런스 제거**(레지스트리 소프트참조로) / **PC 1개 통합**

### 남음
- [ ] **`SelectedCharacterIndex` 단일 소스화** | 난이도: 중 — `PlayerController`·`PlayerState` 양쪽 Replicated 중복. (인게임 팀 협의)
- [ ] **`ULastFPSCharacterDefinition` DataAsset** | 난이도: 중 — `CharacterNames`(위젯)/`SelectableCharacterClasses`(PC)/`CharacterPawnClasses`(GM) 3중 중복 통합. (인게임 팀 작업 후)
  - ↳ 세트로 `PlayerController.SelectableCharacterClasses` 정리 (현재 **허브 폰도 이 목록[인덱스]로 스폰**됨)
- [ ] **밸런스 수치 DataAsset화** | 난이도: 하 — `LastFPSPlayerState.h` constexpr
- [ ] **맵 경로 `/Test/` 제거** | 난이도: 하 — `LastFPSGameInstance` 릴리즈 전

---

## 개발 순서 가이드

```
[지금 당장 — 콘텐츠 양산 (시스템 완성됨, 화면만 찍으면 됨)]
  WBP_Shop  + UI.Screen.Shop  등록  → Quartermaster(G) 검증   (중, 2~3일)
  WBP_Settings + UI.Screen.Settings → 로비/메인메뉴 설정 버튼  (중, 5~7일)

[다음]
  NPC 대화 UI + DataTable
  퀘스트 데이터 구조 + 목록 UI
  인벤토리 UI 프로토타입 (고정 슬롯 그리드)

[인게임 팀 작업 완료 후]
  CharacterDefinition DataAsset → 캐릭터 선택창 연동 → 계승자 관리 화면
  SelectedCharacterIndex 단일화

[마무리]
  모듈 UI / 상점 / 시즌패스 / 도전과제
```

---

## CommonUI 레이어 & 화면 라우팅 참고

| 레이어 | 입력 모드 | 용도 |
|---|---|---|
| `Layer_Game` | Game | 전투 HUD — WASD 통과 |
| `Layer_Menu` | Menu | 메인메뉴/캐릭선택/허브메뉴/콘텐츠 화면 — WASD 차단 |
| `Layer_Modal` | Menu | Confirm / Notice 팝업 |

> 화면을 띄우는 **유일한 경로 = `OpenScreen(태그)`** (PC/Subsystem 위임). "태그→위젯/레이어"는 `DA_ScreenRegistry`(데이터)가 결정.
> 허브 메뉴는 **온디맨드**(ESC) — 평소엔 Game 모드라 이동 가능, 열 때만 Menu 모드.

---

## 구현된 시스템 참고 패턴

| 패턴 | 파일 | 설명 |
|---|---|---|
| **화면 열기/닫기** | `LastFPSPlayerController` `OpenScreen()` | `UIManagerSubsystem`에 위임 → 레지스트리 조회 → 레이어 push |
| Modal 팝업 | `LastFPSPlayerController.cpp` `ShowConfirm()` | `PushWidgetToModalLayer<T>()` |
| 화면 전환(맵) | `LastFPSGameInstance.cpp` `RequestTravelToDestination()` | Travel System |
| 델리게이트 구독/해제 | `LastFPSLoadingScreenWidget.cpp` | `AddUObject` / `Remove(Handle)` |
| GAS 속성 바인딩 | `LastFPSHUDWidget.cpp` | 이벤트 드리븐 |
| NPC 상호작용 | `LastFPSNPCBase.cpp` `HandleBeginOverlap()` | `SetNearestInteractable` → **G키** → `Execute_Interact` |
