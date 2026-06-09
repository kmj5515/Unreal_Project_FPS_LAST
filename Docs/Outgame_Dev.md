# 아웃게임 개발 체크리스트

> 마지막 업데이트: 2026-06-09
> 기준 브랜치: `kmj-dev-roll`
> 전체 완성도: **약 40%** (Phase 1 기반 + UI 라우팅 + PC 통합 + NPC 대화 + 퀘스트 목록 + 설정 기능 C++ 완료)

> **설계 방침** — The First Descendant 참고 PvE 루터슈터지만 **"매치" 개념 없음**: 매치 결과/스코어보드, 로비 출격(StartMatch) **미사용**

> **UI 시스템 문서** — 구조/사용법 → [`UI_System.md`](UI_System.md)

> **최근 변경 (06-09) — 설정 기능 C++ 완료**: `ULastFPSGameUserSettings`(UGameUserSettings 상속, `MasterVolume`/`MusicVolume`/`SFXVolume`/`MouseSensitivity` Config 저장) + `ULastFPSSettingsWidget`(ULastFPSContentScreenWidget 상속) 신설. 그래픽 품질 4단계 버튼 + 볼륨 3 슬라이더 + 감도 슬라이더 + Apply/Revert. 오디오·감도 실제 적용은 `OnAudioSettingsApplied`/`OnSensitivityApplied` BlueprintImplementableEvent로 분리. `DefaultEngine.ini GameUserSettingsClassName` 등록 완료. **에디터 작업**: `WBP_Settings` 부모 클래스를 `LastFPSSettingsWidget`으로 변경 + 위젯 바인딩 추가.
>
> **이전 변경 (06-08) — 퀘스트 데이터 + 목록 UI (C++)**: `FLastFPSQuestData`(DataTable 행) + `ULastFPSQuestScreenWidget`(임무 화면, `QuestTable` 전수 나열) + `ULastFPSQuestEntryWidget`(행) 신설. 임무 화면=퀘스트 로그로 기존 `UI.Screen.Mission` 태그 재사용(C++ 라우팅/태그 추가 0). 에디터 자산(`DT_QuestData`/`WBP_QuestEntry`/`WBP_Missions`/레지스트리 행)만 만들면 허브 "임무" 버튼이 작동.
>
> **이전 변경 (06-07) — 캐릭터 정의 DataAsset**: `ULastFPSCharacterDefinition`(UPrimaryDataAsset) 신설, 캐릭터 선택창의 이름/역할 위젯 직박 배열을 DataAsset 참조(`CharacterDefinitions[]`)로 교체. PawnClass 필드는 정의에 포함하되 스폰 경로엔 아직 미연결(인게임 팀 폰 준비 후).
>
> **이전 변경 (06-07) — 빌드 복구**: 삭제된 `ALastFPSHUD`를 참조하던 잔재 코드(`ALastFPSHero::StartScoreboard`/`StopScoreboard`) 제거 → 에디터 컴파일/실행 정상화. 스코어보드는 설계상 제외(매치 개념 부재)이며 호출부도 이미 주석 처리 상태였음.
>
> **이전 변경 (06-07) — NPC 대화 UI 완료** (인게임 작동 확인)
> - 단방향 대화 시스템 — `FLastFPSDialogueData`(DataTable 행) + `ULastFPSDialogueWidget`(Modal) + `NPC.DialogueRow` + `PC.ShowDialogue`
> - NPC `OnInteract` 폴백 3단: **화면(`ScreenToOpen`) → 대화행(`DialogueRow`) → 공지**
> - 에디터: `WBP_Dialogue` + `DT_DialogueData` + PC `DialogueWidgetClass` 지정 + 테스트 NPC(`BP_NPC_A`/`BP_NPC_B`) 배치, G키 대화 작동 확인
>
> **이전 변경 (06-07) — UI 라우팅 시스템 + PC 통합** (커밋 `6a68d30`)
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
- [x] **CommonActivatableWidget 베이스** — `LastFPSActivatableWidget`
- [x] **태그 기반 화면 라우팅 시스템** ✅ (06-07) → 상세 `UI_System.md`
- [x] **공통 버튼** (`ULastFPSButtonBase : UCommonButtonBase`) — WBP 5종 전환 완료
- [x] **공통 팝업 베이스 (Confirm / Notice)** — `LastFPSModalDialogBase` → Confirm/Notice
- [x] **화면 전환 / 로딩 화면** — `LastFPSGameInstance` Travel System + `LoadingScreenWidget`(이벤트 드리븐)

### 1-2. 진입 플로우

- [x] **메인메뉴** (`LastFPSMainMenuWidget`)
  - Start → **캐릭터 선택** (`RequestTravelToCharacterSelect`)
  - Quit → Confirm 팝업 → 종료
  - [x] Settings 버튼 → `UI.Screen.Settings` 연결 ✅ (06-07) — 메인메뉴 진입 작동 확인

- [ ] **캐릭터 선택창 고도화** 🔨
  - [x] `LastFPSCharacterSelectWidget` — 3카드 선택, Confirm/Back
  - [x] `LastFPSCharacterCardWidget` — `SetSelected(bool)`
  - [x] `CharacterNames`/`CharacterRoles` 위젯 직박 → **DataAsset 연동** ✅ (06-07) — `ULastFPSCharacterDefinition`(DataAsset) 신설, 위젯 `CharacterDefinitions[]`가 이름/역할 표시. 에디터 연동 완료(`DA_Char_0~2` 생성 + `WBP_CharacterSelect` 배열 지정). PawnClass는 인게임 팀 폰 준비 후

- [x] **허브 메뉴** (`LastFPSLobbyWidget` = `WBP_Hub`) — **Menu 레이어 / 온디맨드**
  - `TB_PlayerName`, Inventory/Missions/Shop/Settings/BackToMain 버튼
  - **ESC로 토글** (GameMode `EscMenuScreenTag = UI.Screen.HubMenu`), ESC/Back/닫기로 닫힘
  - 버튼 → `OpenScreenOrNotice` (등록 화면 열기, 미등록이면 "준비 중" 공지)
  - > 출격 버튼 제거됨 — 미션 진입은 NPC/미션보드 별도 경로

- [x] **설정 화면 라우팅** ✅ → `WBP_Settings`(`ContentScreenWidget` 상속) + `UI.Screen.Settings` 등록, 허브/메인메뉴 진입 작동 확인
  - 허브 Settings 버튼 `OpenScreenOrNotice` / 메인메뉴 `HandleSettingsClicked` → `OpenScreen(Screen_Settings())` (미등록 시 공지 폴백)
  - [x] **설정 기능 구현** ✅ (06-09) — `ULastFPSGameUserSettings`(Config 저장) + `ULastFPSSettingsWidget`(C++ 완료). **에디터**: `WBP_Settings` 부모→`LastFPSSettingsWidget`, 버튼/슬라이더 바인딩 추가. 오디오·감도 적용은 BP `OnAudioSettingsApplied`/`OnSensitivityApplied` 구현 필요.

---

## Phase 2 — 핵심 루프

### 2-1. 캐릭터(계승자) 관리
> 인게임 팀 작업 완료 후 진행
- [x] **캐릭터 DataAsset 신설** ✅ (06-07) — `ULastFPSCharacterDefinition : UPrimaryDataAsset` (`DisplayName`/`Role`/`Icon`/`Description`/`PawnClass`). 현재 캐릭터 선택창 이름·역할 표시에 사용. PawnClass 스폰 연결은 인게임 팀 폰 준비 후
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

- [x] **NPC 대화 UI** ✅ (06-07) — 단방향 페이지 대화, 인게임 작동 확인
  - `FLastFPSDialogueData : FTableRowBase` — `SpeakerName`(비우면 NPC `DisplayName`) + `Lines[]`(페이지). 단방향(분기/선택지 없음)
  - `ULastFPSDialogueWidget : ULastFPSModalDialogBase` — Modal 레이어, `Button_Next` 페이지 진행 → 마지막에 닫기, ESC/Back은 전체 닫기
  - `ALastFPSNPCBase.DialogueRow` + `OnInteract` 3단 폴백(**화면 → 대화행 → 공지**)
  - `PlayerController.ShowDialogue(Speaker, Lines)` + `DialogueWidgetClass`
  - 에디터: `WBP_Dialogue` + `DT_DialogueData` + 테스트 NPC `BP_NPC_A`/`BP_NPC_B`
  - > 추가 대화: `DT_DialogueData`에 행 추가 후 NPC `DialogueRow`에 지정 (코드 0줄)

### 3-2. 미션 / 퀘스트
- [x] **퀘스트 데이터 구조** ✅ — `FLastFPSQuestData : FTableRowBase` (`Quest/LastFPSQuestData.h`). 필드: `Title`/`Type`(메인·서브)/`Status`(미시작·진행중·완료)/`Summary`/`Description`/`RewardText`/`Icon`. 진행 추적 서브시스템 없음 — 상태는 행에 직접 명시(프로토)
- [x] **퀘스트 목록 UI** 🔨 — C++ 완료, 에디터 자산 대기. `ULastFPSQuestScreenWidget : ULastFPSContentScreenWidget`(`QuestTable` 전수 → `EntryWidgetClass` 생성 → `Box_QuestList`에 채움) + `ULastFPSQuestEntryWidget : UUserWidget`(`SetupQuest`/`OnQuestDisplayed`). **임무 화면 = 퀘스트 로그**로 기존 `UI.Screen.Mission` 태그 재사용(허브 "임무" 버튼이 이미 라우팅). > **설계 결정(06-08)**: 당장은 임무=퀘스트일지 **통합**. TFD처럼 출격용 미션 보드와 일지를 나누고 싶어지면, 위젯은 태그를 모르므로(레지스트리가 결정) → `Screen_Quests()` 태그 + 허브 "일지" 버튼 + 레지스트리 행만 추가하면 분리됨(C++ 위젯 수정 불필요). 에디터: `DT_QuestData` + `WBP_QuestEntry`(부모 `QuestEntryWidget`) + `WBP_Missions`(부모 `QuestScreenWidget`, `QuestTable`/`EntryWidgetClass` 지정) + 레지스트리 `UI.Screen.Mission → WBP_Missions` 행
- [ ] **HUD 퀘스트 트래커** ⬜

### 3-3. 파티 / 매칭 — ❌ 보류 (매치 개념 부재)
- [~] ~~파티 UI~~ → 존속 여부 재검토 / [x] ~~매칭 UI~~ → **제외**

### 3-4. 제작
- [ ] **제작 시스템 UI** ⬜

---

## Phase 4 — 피니시

- [x] ~~**매치 결과 화면 / 스코어보드**~~ → ❌ 설계 제외. `WBP_Scoreboard`/`WBP_ScoreRow` 미사용(안 씀). C++ `ALastFPSHUD` 클래스 및 관련 잔재 코드 제거 완료 (06-07)
- [ ] **상점 (유료 / 무료)** ⬜
- [ ] **시즌 패스** ⬜ / **도전과제 / 업적** ⬜

---

## 기술 부채

### 완료 ✅
- [x] `UCommonButtonBase` 교체 / **RetryPushTimer 중복 통합** / **위젯 하드레퍼런스 제거**(레지스트리 소프트참조로) / **PC 1개 통합**

### 남음
- [ ] **`SelectedCharacterIndex` 단일 소스화** | 난이도: 중 — `PlayerController`·`PlayerState` 양쪽 Replicated 중복. (인게임 팀 협의)
- [~] **`ULastFPSCharacterDefinition` DataAsset로 3중 중복 통합** | 난이도: 중 — DataAsset 신설 + **위젯 직박(`CharacterNames`/`CharacterRoles`) 제거 완료** ✅ (06-07). 남은 중복: `SelectableCharacterClasses`(PC)/`CharacterPawnClasses`(GM) → 폰 스폰이 PawnClass에 의존하므로 **인게임 팀 폰 준비 후** 통합.
  - ↳ 세트로 `PlayerController.SelectableCharacterClasses` 정리 (현재 **허브 폰도 이 목록[인덱스]로 스폰**됨)
- [ ] **밸런스 수치 DataAsset화** | 난이도: 하 — `LastFPSPlayerState.h` constexpr
- [ ] **맵 경로 `/Test/` 제거** | 난이도: 하 — `LastFPSGameInstance` 릴리즈 전

---

## 개발 순서 가이드

```
[완료]
  ✅ WBP_Shop + UI.Screen.Shop 등록 → Quartermaster(G)/허브 Shop 버튼 작동 확인
  ✅ WBP_Settings + UI.Screen.Settings 등록 → 허브/메인메뉴 설정 버튼 라우팅 작동 확인
  ✅ NPC 대화 UI — WBP_Dialogue + DT_DialogueData + 테스트 NPC, G키 대화 작동 확인
  ✅ 퀘스트 목록 UI — DT_QuestData + WBP_QuestEntry + WBP_Missions + 레지스트리 행, 임무 화면 작동 확인
  ✅ 설정 기능 C++ — ULastFPSGameUserSettings + ULastFPSSettingsWidget, WBP_Settings 에디터 완료
     (남음: BP OnAudioSettingsApplied → 사운드 클래스 연결 / OnSensitivityApplied → Input Modifier 연결)

[지금 당장 — Phase 1 마무리]
  캐릭터 선택창 카드 직접 클릭 선택 — LastFPSCharacterCardWidget에 클릭 이벤트 추가

[다음]
  인벤토리 UI 프로토타입 (고정 슬롯 그리드)

[인게임 팀 작업 완료 후]
  CharacterDefinition DataAsset → PawnClass 스폰 연결 → 계승자 관리 화면
  SelectedCharacterIndex 단일화 / SelectableCharacterClasses(PC) DataAsset으로 정리

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
