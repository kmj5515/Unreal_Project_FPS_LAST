# 아웃게임 개발 체크리스트

> 마지막 업데이트: 2026-06-16 · 기준 브랜치: `kmj-dev-roll`
> **설계 방침** — The First Descendant 참고 PvE 루터슈터, **"매치" 개념 없음** (스코어보드·로비 출격 미사용)
> **문서** — UI 구조 [`UI_System.md`](UI_System.md) · 모듈 시스템 [`Module_System.md`](Module_System.md)

> **전체 완성도: 약 65%** — 기반/허브/인벤토리/상점/퀘스트 표시/모듈 백엔드 완료.
> 남은 인스코프: 모듈 UI · SaveGame 영속화 · 계승자 관리 화면 · 미션 보드. (제작·시즌패스·아르케 성장은 보류/범위 밖)
> 포폴 수직 슬라이스(모듈 UI + 로드아웃 + SaveGame) 기준으로는 **약 75%**.

> **최근 (06-16)** — 모듈 장착 백엔드 완료 / 궁극기 킬 기반 폐기 → 쿨다운 전환. (이전 이력은 git 로그 참조)

---

## 범례
✅ 완료 · 🔨 일부 구현 · ⬜ 미시작 · ⏸️ 보류 · ❌ 제외

---

## Phase 1 — 기반 구조 ✅

### 1-1. UI 프레임워크
- [x] GameUIPolicy + Layer Stack / CommonActivatableWidget 베이스 (`LastFPSActivatableWidget`)
- [x] **태그 기반 화면 라우팅** — `ScreenRegistry`(DataAsset) + `UIManagerSubsystem.OpenScreen(Tag)`. 화면 추가 = 위젯+레지스트리 행 (코드 0줄) → `UI_System.md`
- [x] 공통 버튼 (`ULastFPSButtonBase`) / 공통 팝업 (`LastFPSModalDialogBase` → Confirm·Notice)
- [x] 화면 전환·로딩 화면 (`LastFPSGameInstance` Travel + `LoadingScreenWidget`)

### 1-2. 진입 플로우
- [x] 메인메뉴 (`LastFPSMainMenuWidget`) — Start→캐릭터 선택 / Settings / Quit
- [x] 캐릭터 선택창 (`LastFPSCharacterSelectWidget` + `CharacterCardWidget`) — 카드 클릭 선택, `CharacterDefinition` DataAsset 연동(`DA_Char_0~2`). *PawnClass 스폰은 인게임 대기*
- [x] 허브 메뉴 (`WBP_Hub`) — ESC 토글, Inventory/Missions/Shop/Settings 버튼 → `OpenScreenOrNotice`
- [x] 설정 (`ULastFPSGameUserSettings` + `WBP_Settings`) — 그래픽/볼륨/감도. **남음: 오디오·감도 실제 적용 BP(`OnAudioSettingsApplied`/`OnSensitivityApplied`)**

---

## Phase 2 — 핵심 루프

### 2-1. 계승자 관리 *(인게임 팀 대기)*
- [x] 캐릭터 DataAsset (`ULastFPSCharacterDefinition` — DisplayName/Role/Icon/Description/PawnClass/StatData/AbilitySet)
- [ ] 계승자 관리 화면 ⬜ — 보유 목록 / 스탯 / 스킨 프리뷰
- [ ] 아르케 조율·성장 ⏸️ (보류)

### 2-2. 장비 / 인벤토리
- [x] 아이템 DataTable (`FLastFPSItemData` — 타입/희귀도/스택)
- [x] 인벤토리 UI (`ItemSlotWidget` + `InventoryWidget`, 24슬롯) — `EconomySubsystem.OwnedItems` 연동, `OnInventoryChanged` 자동 갱신
- [x] **모듈 백엔드** (`FLastFPSModuleData` + `LoadoutSubsystem`) — 장착/캐파 검증/스탯 합/Infinite GE 적용 → `Module_System.md`. **남음: C++ 빌드 검증**
- [ ] 🔨 **모듈 UI** ← *다음 작업* — 보유 목록 + 슬롯 + 스탯 미리보기(`ComputeBonus`), `TryEquip`/`Unequip`, `UI.Screen.Module` 라우팅

---

## Phase 3 — 서브 시스템

### 3-1. Hub
- [x] NPC 상호작용 (`ILastFPSInteractable` + `NPCBase`, **G키**, `ScreenToOpen` → `DialogueRow` → 공지 폴백) — `BP_NPC_Quartermaster`
- [x] NPC 대화 (`FLastFPSDialogueData` + `DialogueWidget`, 단방향 페이지)

### 3-2. 미션 / 퀘스트
- [x] 퀘스트 데이터 (`FLastFPSQuestData`) + 목록 UI (`QuestScreenWidget`/`QuestEntryWidget`, `UI.Screen.Mission`)
- [x] HUD 퀘스트 트래커 (`QuestTrackerWidget` — 진행중 퀘스트만 필터)
- [ ] ⬜ **진행 추적 로직 없음** — 현재 정적 표시(상태를 DT 행에 직접 명시). 출격→목표→클리어는 미션 시스템 필요 *(인게임 핸드오프)*

### 3-3. 파티 / 매칭 — ❌ 제외 (매치 개념 없음)
### 3-4. 제작 — ⏸️ 보류 (포폴 범위 밖, 수직 슬라이스 완성 이후)

---

## Phase 4 — 피니시
- [x] 상점 UI + 화폐/재고 (`ShopScreenWidget` + `EconomySubsystem`) — `TryPurchase` 잔액 차감+아이템 지급, 재고 무제한. **남음: SaveGame 영속화(앱 재시작 유지)**
- ❌ 매치 스코어보드 (제외) · ⬜ 시즌패스 / 업적 (범위 밖)

---

## 기술 부채
- [x] ✅ **궁극기 쿨다운 전환 + 게이지 연쇄 정리** (06-16) — 킬 기반 게이지/킬힐 메커닉 전면 제거(`UltimateGauge` 어트리뷰트·상수·`GE_UltimateKillHeal`·HUD 게이지). F는 다른 스킬과 동일 쿨다운(`GE_UltimateCooldown` + `Cooldown.Ultimate`). **남음: 궁극기 실제 효과(기획 후 재구현)**
- [x] ✅ UI 정리 — `UCommonButtonBase` 전환 / 위젯 하드레퍼런스 제거 / PlayerController 1개 통합
- [ ] ⬜ 맵 경로 `/Test/` 제거 (릴리즈 전, `LastFPSGameInstance`)

---

## 진행 로드맵
- **다음:** 모듈 장착 UI
- **그 다음:** SaveGame 영속화 → 계승자 관리 화면 → 미션 보드/출격(인게임 핸드오프)
- **인게임 팀 대기:** PawnClass 스폰 연결, 미션 진행 로직
- **보류/범위 밖:** 제작 · 시즌패스/업적 · 아르케 성장 · 파티

---

## CommonUI 레이어 & 화면 라우팅 참고

| 레이어 | 입력 모드 | 용도 |
|---|---|---|
| `Layer_Game` | Game | 전투 HUD — WASD 통과 |
| `Layer_Menu` | Menu | 메인메뉴/캐릭선택/허브메뉴/콘텐츠 화면 — WASD 차단 |
| `Layer_Modal` | Menu | Confirm / Notice 팝업 |

> 화면을 띄우는 **유일한 경로 = `OpenScreen(태그)`**. "태그→위젯/레이어"는 `DA_ScreenRegistry`가 결정.
> 허브 메뉴는 **온디맨드**(ESC) — 평소엔 Game 모드, 열 때만 Menu 모드.

---

## 구현된 시스템 참고 패턴

| 패턴 | 파일 | 설명 |
|---|---|---|
| 화면 열기/닫기 | `LastFPSPlayerController::OpenScreen()` | `UIManagerSubsystem` 위임 → 레지스트리 조회 → 레이어 push |
| Modal 팝업 | `LastFPSPlayerController::ShowConfirm()` | `PushWidgetToModalLayer<T>()` |
| 화면 전환(맵) | `LastFPSGameInstance::RequestTravelToDestination()` | Travel System |
| GAS 속성 바인딩 | `LastFPSHUDWidget.cpp` | 이벤트 드리븐 |
| NPC 상호작용 | `LastFPSNPCBase::HandleBeginOverlap()` | `SetNearestInteractable` → G키 → `Execute_Interact` |
| 화폐/보유(경제) | `LastFPSEconomySubsystem::TryPurchase()` | GameInstanceSubsystem(맵 이동 유지) + `OnCreditsChanged`/`OnInventoryChanged` |
| 모듈 로드아웃 | `LastFPSLoadoutSubsystem` | 장착 상태 보관 + `ComputeBonus`(UI) + `ApplyToAbilitySystem`(GE) |
