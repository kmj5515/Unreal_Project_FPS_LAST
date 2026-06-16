# 아웃게임 개발 체크리스트

> 마지막 업데이트: 2026-06-16
> 기준 브랜치: `kmj-dev-roll`
> 전체 완성도: **약 65%** (제작 보류로 범위 축소 반영) — Phase 1 완료 + 인벤토리/상점 화폐·보유 + 모듈 장착 백엔드
> 　└ 포폴 수직 슬라이스(모듈 UI + 로드아웃 화면 + SaveGame)만 따지면 **약 75%**. 전체 %가 낮아 보이는 건 계승자 관리·아르케 성장 등 인게임 대기/대형 메타가 분모에 남아서임

> **설계 방침** — The First Descendant 참고 PvE 루터슈터지만 **"매치" 개념 없음**: 매치 결과/스코어보드, 로비 출격(StartMatch) **미사용**

> **UI 시스템 문서** — 구조/사용법 → [`UI_System.md`](UI_System.md)
> **모듈 시스템 문서** — 구조/API/세팅 → [`Module_System.md`](Module_System.md)

> **최근 변경 (06-16) — 모듈(장비) 장착 시스템 백엔드**: 인벤토리를 "보관함→빌드"로 바꾸는 루터슈터 정체성 기능. **C++ 백엔드 완료(UI·빌드검증 대기)**. `FLastFPSModuleData`(DT_ModuleData 행: `StatMods[]` + `CapacityCost`, **행 이름 = DT_ItemData 동일 행**으로 보유/아이콘 공유) + `ULastFPSLoadoutSubsystem`(GameInstanceSubsystem: 슬롯별 장착 상태 보관, `TryEquip`/`Unequip`/`CanEquip` 보유·캐파 검증, `ComputeBonus` 스탯 합, `ApplyToAbilitySystem` Infinite GE 적용). 적용 지점: `ALastFPSCharacterBase::InitAbilitySystem`에서 베이스 스탯(StatData) 적용 **직후** 서버 권위로 GE 가산 → MaxHealth/MaxStamina/AttackDamage/Defense/MoveSpeed (MoveSpeed는 기존 델리게이트로 MaxWalkSpeed까지 자동). 슬롯 4·캐파 10(`DefaultGame.ini` 조정). **더미 5종 임포트 완료**(`Excel/DT_ModuleData.json`: 치명타/체력/방어/신속/지구력, 합 캐파 14>10이라 장착 선택 강제). **남음: 장착 UI(보유 목록 + 슬롯 + `ComputeBonus` 스탯 미리보기, `UI.Screen.Module` 라우팅) / C++ 빌드 검증.** 상세 → [`Module_System.md`](Module_System.md)
>
> **이전 변경 (06-13) — 상점 화폐/재고 시스템 (실제 차감+소지)**: `ULastFPSEconomySubsystem`(GameInstanceSubsystem) 신설 — `Credits`(세션 잔액) + `OwnedItems`(DT_ItemData 행이름→수량). 맵 이동(메인/캐릭선택/허브)에도 유지, 앱 재시작 시 `StartingCredits`(기본 10000)/`StartingOwnedItems`로 초기화(SaveGame 영속화는 추후). 구매 = `TryPurchase(GrantItemRowId, Price)` → 잔액 충분하면 차감 + 아이템 지급, 부족하면 거부. **재고 무제한**(살 수 있으면 반복 구매). `FLastFPSShopItemData`에 `GrantItemRowId`(지급할 DT_ItemData 행) 필드 추가. 상점 화면: `TB_Credits` 잔액 표시 + 잔액 변동 시(`OnCreditsChanged`) 각 엔트리 구매버튼 활성/비활성. 엔트리: 영구 "구매됨" 토글 제거 → 가격 상시 표시 + `SetAffordable` + `OnPurchaseSucceeded`(BP 연출용). 인벤토리: 테이블 전수 표시 폐기 → `OwnedItems`만 표시, `OnInventoryChanged` 구독 자동 재구성, 슬롯 수량(`TB_Count`) 표시. **테스트 데이터**: 아이템 5종 + 상점 5종(서로 `GrantItemRowId` 매칭) 임포트 완료. **에디터 완료(06-14)**: `DT_ShopData.GrantItemRowId` 채우기 + `WBP_Shop` `TB_Credits` + `WBP_ItemSlot` `TB_Count` 모두 완료. (데모 시드 `StartingCredits`/`StartingOwnedItems` `DefaultGame.ini` 지정은 선택 사항으로 잔여)
>
> **이전 변경 (06-10) — 캐릭터 선택창 Prev/Next 버튼 제거**: 카드 직접 클릭(`OnCardClicked`)만으로 선택하도록 정리. `Button_Prev`/`Button_Next` 바인딩 + `HandlePrevClicked`/`HandleNextClicked` + `OnSelectionChanged`의 활성/비활성 토글 코드 제거. **에디터 완료(06-14)**: `WBP_CharacterSelect`에서 Prev/Next 버튼 제거 완료.
>
> **이전 변경 (06-10) — 상점 UI 프로토타입 완료**: `FLastFPSShopItemData`(DataTable 행) + `ULastFPSShopEntryWidget`(목록 한 줄, 구매 버튼 클릭 시 "구매됨" 표시) + `ULastFPSShopScreenWidget`(테이블 전수 → 엔트리 목록 구성). `DT_ShopData` + `WBP_ShopEntry` + `WBP_Shop` 에디터 자산 완료. 화폐/재고 시스템은 아직 없어 구매는 표시 전환만 처리하는 프로토 단계.
>
> **이전 변경 (06-09) — 인벤토리 UI 프로토타입 완료**: C++ + 에디터 자산 완료. `FLastFPSItemData` + `ULastFPSItemSlotWidget`(희귀도 색상 C++ 직접 처리, `RarityToColor()`, `Img_Background`/`Img_RarityBorder` 바인딩) + `ULastFPSInventoryWidget`(SlotCount=24 고정 슬롯 그리드). `DT_ItemData` + `WBP_ItemSlot` + `WBP_Inventory` + 레지스트리 `UI.Screen.Inventory` 행 완료 → 허브 "인벤토리" 버튼 작동.
>
> **이전 변경 (06-09) — 캐릭터 선택창 버그 수정 + 카드 데이터 표시**: Prev/Next 버튼 항상 비활성 버그 수정(`TotalCount`를 `SelectableCharacterClasses.Num()`→`CharacterDefinitions.Num()`으로). `SetupCard(BlueprintNativeEvent)` 추가로 카드마다 이름/역할 독립 표시. `TB_CharDesc` 바인딩 추가(선택 캐릭터 Description 표시).
>
> **이전 변경 (06-09) — 설정 기능 C++ 완료**: `ULastFPSGameUserSettings`(UGameUserSettings 상속, `MasterVolume`/`MusicVolume`/`SFXVolume`/`MouseSensitivity` Config 저장) + `ULastFPSSettingsWidget`(ULastFPSContentScreenWidget 상속) 신설. 그래픽 품질 4단계 버튼 + 볼륨 3 슬라이더 + 감도 슬라이더 + Apply/Revert. 오디오·감도 실제 적용은 `OnAudioSettingsApplied`/`OnSensitivityApplied` BlueprintImplementableEvent로 분리. `DefaultEngine.ini GameUserSettingsClassName` 등록 완료. **에디터 완료(06-14)**: `WBP_Settings` 부모 클래스를 `LastFPSSettingsWidget`으로 변경 + 버튼/슬라이더 바인딩 완료. (오디오·감도 실제 적용 BP `OnAudioSettingsApplied`/`OnSensitivityApplied`는 잔여)
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

- [x] **캐릭터 선택창 고도화** ✅ (06-10)
  - [x] `LastFPSCharacterSelectWidget` — 3카드 선택(Prev/Next 제거, 카드 클릭만), Confirm/Back + `TB_CharName`/`TB_CharRole`/`TB_CharDesc` 선택 캐릭터 정보 표시
  - [x] `LastFPSCharacterCardWidget` — `SetSelected(bool)` + **카드 직접 클릭** (`FOnCardClicked` + `NativeOnMouseButtonDown`) + `SetupCard`(카드별 이름/역할 독립 표시). 선택 표시(`SetSelected`)는 WBP에서 구현 필요
  - [x] **DataAsset 연동** ✅ (06-07) — `ULastFPSCharacterDefinition`(DataAsset) 신설. 에디터 연동 완료(`DA_Char_0~2` + `WBP_CharacterSelect` 배열 지정). PawnClass는 인게임 팀 폰 준비 후

- [x] **허브 메뉴** (`LastFPSLobbyWidget` = `WBP_Hub`) — **Menu 레이어 / 온디맨드**
  - `TB_PlayerName`, Inventory/Missions/Shop/Settings/BackToMain 버튼
  - **ESC로 토글** (GameMode `EscMenuScreenTag = UI.Screen.HubMenu`), ESC/Back/닫기로 닫힘
  - 버튼 → `OpenScreenOrNotice` (등록 화면 열기, 미등록이면 "준비 중" 공지)
  - > 출격 버튼 제거됨 — 미션 진입은 NPC/미션보드 별도 경로

- [x] **설정 화면 라우팅** ✅ → `WBP_Settings`(`ContentScreenWidget` 상속) + `UI.Screen.Settings` 등록, 허브/메인메뉴 진입 작동 확인
  - 허브 Settings 버튼 `OpenScreenOrNotice` / 메인메뉴 `HandleSettingsClicked` → `OpenScreen(Screen_Settings())` (미등록 시 공지 폴백)
  - [x] **설정 기능 구현** ✅ (06-09, 에디터 06-14) — `ULastFPSGameUserSettings`(Config 저장) + `ULastFPSSettingsWidget`(C++ 완료) + `WBP_Settings` 부모→`LastFPSSettingsWidget` + 버튼/슬라이더 바인딩 완료. 오디오·감도 실제 적용 BP `OnAudioSettingsApplied`/`OnSensitivityApplied`만 잔여(잔여 1건).

---

## Phase 2 — 핵심 루프

### 2-1. 캐릭터(계승자) 관리
> 인게임 팀 작업 완료 후 진행
- [x] **캐릭터 DataAsset 신설** ✅ (06-07) — `ULastFPSCharacterDefinition : UPrimaryDataAsset` (`DisplayName`/`Role`/`Icon`/`Description`/`PawnClass`). 현재 캐릭터 선택창 이름·역할 표시에 사용. PawnClass 스폰 연결은 인게임 팀 폰 준비 후
- [ ] **계승자 관리 화면** ⬜ — 보유 목록 / 스탯 / 스킨 프리뷰
- [ ] **아르케 조율 / 성장 시스템** ⬜

### 2-2. 장비 / 인벤토리
- [x] **아이템 DataTable** ✅ (06-09) — `FLastFPSItemData : FTableRowBase` (`ItemName`/`Description`/`Icon`/`ItemType`/`Rarity`/`MaxStackSize`). `ELastFPSItemType`(무기·모듈·소모품·재료) + `ELastFPSItemRarity`(일반·희귀·영웅·전설)
- [x] **인벤토리 UI** ✅ (06-09) — `ULastFPSItemSlotWidget`(희귀도 색상 C++ 처리, `Img_Background`/`Img_RarityBorder`/`Image_Icon`/`TB_ItemName`/`TB_Rarity`) + `ULastFPSInventoryWidget`(SlotCount=24 고정 슬롯 그리드) + 에디터(`DT_ItemData`/`WBP_ItemSlot`/`WBP_Inventory`/레지스트리 행) 완료
  - [x] **보유 연동** ✅ (06-13) — 테이블 전수 표시 폐기 → `EconomySubsystem.OwnedItems`만 표시, `OnInventoryChanged` 구독 자동 재구성, 슬롯 수량(`TB_Count`, 선택 바인딩) 표시. 구매로 보유가 늘면 즉시 반영
- [x] **모듈 시스템 백엔드** ✅ (06-16) — `FLastFPSModuleData`(DT_ModuleData) + `ULastFPSLoadoutSubsystem`(슬롯별 장착 상태/캐파 검증/스탯 합/Infinite GE 적용). 스폰 시 `ALastFPSCharacterBase::InitAbilitySystem`에서 베이스 스탯 직후 서버 권위로 적용. 더미 5종 임포트(`Excel/DT_ModuleData.json`). 상세 → [`Module_System.md`](Module_System.md). **남음: C++ 빌드 검증**
- [ ] **모듈 시스템 UI** 🔨 — 장착 화면(보유 모듈 목록 + 슬롯 + `ComputeBonus` 스탯 미리보기) + `TryEquip`/`Unequip` 연결 + `OnLoadoutChanged` 자동 갱신 + `UI.Screen.Module` 라우팅 등록. **백엔드 완료, UI만 남음**

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
- [x] **HUD 퀘스트 트래커** ✅ (06-10, 에디터 06-14) — C++ + 에디터 자산 완료. `ULastFPSQuestTrackerWidget : UUserWidget` — `QuestTable`(`DT_QuestData`, 퀘스트 화면과 동일 테이블 재사용)에서 `Status == InProgress`인 행만 최대 `MaxTrackedQuests`개 골라 `EntryWidgetClass`(`WBP_QuestEntry` 재사용 가능)로 `Box_TrackerList`에 표시. `ULastFPSHUDWidget`에 `WBP_QuestTracker`(`BindWidgetOptional`) 바인딩 추가. 에디터: `WBP_QuestTracker`(부모 `QuestTrackerWidget`, `QuestTable`/`EntryWidgetClass` 지정) 생성 완료. **에디터 완료(06-14)**: 위젯 계층(`Box_TrackerList`/`TB_Empty`) 구성 + `WBP_HUD`에 `WBP_QuestTracker` 이름으로 배치 완료

### 3-3. 파티 / 매칭 — ❌ 보류 (매치 개념 부재)
- [~] ~~파티 UI~~ → 존속 여부 재검토 / [x] ~~매칭 UI~~ → **제외**

### 3-4. 제작 — ⏸️ 보류 (06-16, 포폴 범위 밖)
- [~] **제작 시스템 UI** ⏸️ 보류 — TFD 핵심 메타지만 비중 대비 무거워 수직 슬라이스(모듈/로드아웃/SaveGame) 완성 이후로 미룸

---

## Phase 4 — 피니시

- [x] ~~**매치 결과 화면 / 스코어보드**~~ → ❌ 설계 제외. `WBP_Scoreboard`/`WBP_ScoreRow` 미사용(안 씀). C++ `ALastFPSHUD` 클래스 및 관련 잔재 코드 제거 완료 (06-07)
- [x] **상점 UI** ✅ (06-13) — `FLastFPSShopItemData`(+`GrantItemRowId`) + `ULastFPSShopEntryWidget` + `ULastFPSShopScreenWidget`(`DT_ShopData` 전수 나열) + 에디터(`DT_ShopData`/`WBP_ShopEntry`/`WBP_Shop`) 완료
  - [x] **화폐/재고 시스템** ✅ (06-13) — `ULastFPSEconomySubsystem`(GameInstanceSubsystem): `TryPurchase`로 잔액 차감 + `OwnedItems` 지급, 잔액 부족 시 거부, **재고 무제한**. `TB_Credits` 잔액 표시 + 잔액 변동 시 구매버튼 활성/비활성. **에디터 완료(06-14)**: `DT_ShopData.GrantItemRowId` / `WBP_Shop` `TB_Credits` / `WBP_ItemSlot` `TB_Count` 완료. (데모 시드 `StartingCredits`·`StartingOwnedItems`는 선택 잔여) > SaveGame 영속화(앱 재시작 유지)는 추후
- [ ] **시즌 패스** ⬜ / **도전과제 / 업적** ⬜

---

## 기술 부채

### 완료 ✅
- [x] `UCommonButtonBase` 교체 / **RetryPushTimer 중복 통합** / **위젯 하드레퍼런스 제거**(레지스트리 소프트참조로) / **PC 1개 통합**

### 남음
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
  ✅ 캐릭터 선택창 마무리 — 카드 직접 클릭, Prev/Next 버그 수정, SetupCard, TB_CharDesc
  ✅ 인벤토리 UI — FLastFPSItemData + ULastFPSItemSlotWidget + ULastFPSInventoryWidget + 에디터 자산 완료
  ✅ 상점 UI — FLastFPSShopItemData + ULastFPSShopEntryWidget + ULastFPSShopScreenWidget + 에디터 자산 완료
  ✅ 상점 화폐/재고 시스템 — ULastFPSEconomySubsystem(Credits/OwnedItems), TryPurchase 차감+지급, 인벤토리 보유 연동, 재고 무제한
     (에디터 완료 06-14: DT_ShopData.GrantItemRowId / WBP_Shop TB_Credits / WBP_ItemSlot TB_Count)
     (추후: SaveGame 영속화 — 앱 재시작에도 잔액/보유 유지)
  ✅ HUD 퀘스트 트래커 — ULastFPSQuestTrackerWidget(QuestTable에서 진행중 퀘스트만 필터) + HUDWidget 바인딩 + WBP_QuestTracker/WBP_HUD 에디터 자산 완료(06-14)
  ✅ 모듈 시스템 백엔드 (06-16) — FLastFPSModuleData(DT_ModuleData) + ULastFPSLoadoutSubsystem(장착/캐파검증/스탯합/Infinite GE), 스폰 시 적용, 더미 5종 임포트
     (남음: 장착 UI / C++ 빌드 검증)

[다음]
  모듈 시스템 UI — 보유 모듈 목록 + 슬롯 + ComputeBonus 스탯 미리보기, TryEquip/Unequip 연결, UI.Screen.Module 라우팅 등록

[인게임 팀 작업 완료 후]
  CharacterDefinition DataAsset → PawnClass 스폰 연결 → 계승자 관리 화면 / 아르케 조율·성장 시스템

[마무리]
  상점 화폐/재고 — SaveGame 영속화 (앱 재시작에도 잔액/보유 유지)
  (보류) 제작 시스템 UI — 포폴 범위 밖, 수직 슬라이스 완성 이후
  파티 UI 존속 여부 재검토
  시즌 패스 / 도전과제·업적
  설정 — OnAudioSettingsApplied(사운드 클래스 연결) / OnSensitivityApplied(Input Modifier 연결) BP 구현
  기술 부채 — 밸런스 수치 DataAsset화 / 맵 경로 `/Test/` 제거
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
| 화폐/보유(경제) | `LastFPSEconomySubsystem` `TryPurchase()` | GameInstanceSubsystem(맵 이동 유지) + `OnCreditsChanged`/`OnInventoryChanged` 델리게이트로 상점·인벤토리 UI 갱신 |
