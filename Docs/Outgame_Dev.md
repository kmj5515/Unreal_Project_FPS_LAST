# 아웃게임 개발 체크리스트

> 마지막 업데이트: 2026-06-23 · 기준 브랜치: `main`
> **설계 방침** — The First Descendant 참고 PvE 루터슈터, **"매치" 개념 없음** (스코어보드·로비 출격 미사용)
> **문서** — UI 구조 [`UI_System.md`](UI_System.md) · 모듈 시스템 [`Module_System.md`](Module_System.md)

> **목적: 1차 프로토타입 = 포트폴리오용 수직 슬라이스** (영상/스크린샷 위주 — [형태 결정](#포폴-형태-결정-06-23)). 우선순위는 [남은 작업 TODO](#남은-작업-todo--포트폴리오-우선순위) 참조.
> **포폴 수직 슬라이스(모듈 UI + 로드아웃) 완성도: 약 90%** — 남은 건 빌드/동작 검증 + 라이브 데모 루프 녹화(P0). **SaveGame은 v1 범위에서 제외**(아래 결정).
> 전체 기능 기준 약 70% — 기반/허브/인벤토리/상점/퀘스트 표시/모듈 백엔드+UI 완료. (계승자 관리·미션 보드·설정 BP 남음 / SaveGame·제작·시즌패스·아르케는 보류·범위 밖)

### 포폴 형태 결정 (06-23)
> **영상/스크린샷 위주** (플레이해도 1~2회). → **SaveGame 영속화는 v1 제외.**
> 근거: 재시작 영속화는 영상에서 안 보임 / 기술 깊이는 라이브 루프(구매·장착·GAS 적용)가 이미 증명 / SaveGame 직렬화는 정형 보일러플레이트라 차별점 약함.
> 어필 축 = **기술/아키텍처 깊이**. 차별 포인트는 태그 라우팅 · WidgetTreeGen 자작 플러그인 · GAS 모듈 로드아웃.

> **최근 (06-23) 빌드 피드백 대응** — ① 모듈 창 빈 화면: `DefaultGame.ini`에 `LoadoutSubsystem.ModuleTable` 누락이 원인 → 연결 + 데모용 `StartingOwnedItems` 시드. ② 상점 구매: 수량 선택 모달(`ULastFPSQuantityDialogWidget` + `PC::ShowQuantityPrompt`) 추가, 스택 한도(`MaxStackSize`)·잔액 기반 구매 가능 수량 계산(무기 등 비스택은 보유 시 구매 불가). ③ 설정: 슬라이더 옆 백분율 텍스트(`TB_*Volume`/`TB_Sensitivity`). **남은 BP 작업: WBP_QuantityDialog 생성·바인딩, WBP_Inventory/WBP_Module의 `ItemTable` 할당 확인, WBP_Shop의 `ItemTable` 할당, WBP_Settings에 수치 텍스트 추가, PC BP에 `QuantityDialogWidgetClass` 지정.**
>
> **(06-23)** — ⚠️ **문서 정정: SaveGame 영속화는 실제 미구현**이었음(소스·히스토리 부재, 코드 주석이 "추후/Step 2 예정"으로 명시). 이전 "완료" 기재는 오류. → **포폴 형태(영상 위주) 검토 결과 v1 범위에서 제외 결정**(위 [형태 결정](#포폴-형태-결정-06-23)). / 아웃게임 신규 변경 없음. 이후 커밋은 인게임/어빌리티 팀 작업(`GA_VIOLA 세팅`). 미커밋 산출물: 더미데이터 JSON(`Docs/DummyData/`) + Showcase 위젯 변형(`WBP_Inventory_SCG`/`WBP_ItemSlot_SCG`).
> **이전 (06-20)** — SaveGame 영속화 완료(`LastFPSSaveGameSubsystem`, 크레딧·아이템·모듈 장착 디스크 유지) / 모듈 장착 UI 완료 / 캐릭터 선택 카드 클릭·하이라이트. (이전 이력은 git 로그 참조)

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
- [x] ✅ **모듈 UI** — C++(`ModuleScreenWidget` + `ModuleEntryWidget` + `ModuleSlotWidget`) + WBP 3종(`WBP_Module`/`WBP_ModuleEntry`/`WBP_ModuleSlot`, 다크 sci-fi 테마): 보유 목록 클릭→빈 슬롯 장착, 슬롯 클릭→해제, 캐파/보정 미리보기, `OnLoadoutChanged`/`OnInventoryChanged` 자동 갱신. `UI.Screen.Module` 태그 등록 + 허브(`WBP_HubMenu`) `Button_Module`→`OpenScreenOrNotice` 진입 배선 완료.

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
- [x] 상점 UI + 화폐/재고 (`ShopScreenWidget` + `EconomySubsystem`) — `TryPurchase` 잔액 차감+아이템 지급, 재고 무제한.
- ⏸️ **SaveGame 영속화 — v1 제외** (⚠️ 06-23 정정: 이전 "완료" 기재 오류, 실제 미구현). 영상 위주 포폴이라 가치 낮아 보류 ([형태 결정](#포폴-형태-결정-06-23)). 현재 동작: 재시작 시 `StartingCredits`/`StartingOwnedItems`로 초기화. 추후 구현 시 설계: 시작 시 로드→Economy/Loadout 복원 → 변경 델리게이트 구독해 즉시 저장(복원→구독 순서로 루프 차단).
- ❌ 매치 스코어보드 (제외) · ⬜ 시즌패스 / 업적 (범위 밖)

---

## 기술 부채
- [x] ✅ **궁극기 쿨다운 전환 + 게이지 연쇄 정리** (06-16) — 킬 기반 게이지/킬힐 메커닉 전면 제거(`UltimateGauge` 어트리뷰트·상수·`GE_UltimateKillHeal`·HUD 게이지). F는 다른 스킬과 동일 쿨다운(`GE_UltimateCooldown` + `Cooldown.Ultimate`). **남음: 궁극기 실제 효과(기획 후 재구현)**
- [x] ✅ UI 정리 — `UCommonButtonBase` 전환 / 위젯 하드레퍼런스 제거 / PlayerController 1개 통합
- [ ] ⬜ 맵 경로 `/Test/` 제거 (릴리즈 전, `LastFPSGameInstance`)

---

## 남은 작업 (TODO) — 포트폴리오 우선순위

> **목표: 1차 프로토타입 = 녹화 가능한 수직 슬라이스.** 새 화면을 늘리기보다 *이미 만든 시스템이 끝까지 동작하는 데모 루프* 완성이 최우선.
> 판단 기준 = **"보여줄 만한가"**. 내부 정리·기획 미정 항목은 포폴 직전까지 후순위.

### 🔥 P0 — 데모가 성립하려면 반드시 (지금 집중)
- [ ] **라이브 데모 루프 빌드 검증** — Economy(구매→차감→인벤토리 갱신) + Loadout(모듈 장착→GAS 스탯 적용)이 실제로 빌드·실행되는지 — Phase 2-2 / 4
- [ ] **데모 루프 1회 완주 + 녹화** — [시연 시나리오](#시연-시나리오-demo-flow) 전체가 한 번에 흐르는지 (이게 곧 포폴 결과물)

### ⭐ P1 — 데모 루프 견고화 (어필 축 = 기술/아키텍처 깊이)
> 어필 포인트가 화면보다 **시스템 설계**이므로, P1은 새 화면이 아니라 *데모 루프가 어떤 순서로 조작해도 안 깨지는* 견고함에 둔다.
- [ ] **데모 루프 엣지케이스 점검** — 잔액 부족 구매 / 캐파 초과 장착 / 빈 슬롯 해제 등에서 정상 폴백(에러·예외 없이)
- [ ] **계승자 관리 화면** — Phase 2-1. 단순 목록·스탯표면 후순위. 넣는다면 회전 캐릭터 프리뷰까지 가야 의미 있음 (기술 축에선 데모 루프 견고화보다 아래)

### 🧊 P2 — 포폴 가치 낮음, 데모 직전까지 보류
- [ ] 설정 적용 BP (오디오·감도) — 동작은 지루함, 녹화에 안 잡힘
- [ ] 맵 경로 `/Test/` 제거 — 내부 정리, 보는 사람 모름
- [ ] 궁극기 실제 효과 — 인게임/기획 영역, 아웃게임 포폴과 무관

### ⏳ 인게임 팀 핸드오프 (아웃게임 측 준비 완료)
- [ ] PawnClass 스폰 연결 (캐릭터 선택 → 인게임)
- [ ] 미션 진행 추적 로직 (출격→목표→클리어) — 현재 정적 표시만 — Phase 3-2

### ⏸️ 보류 / 범위 밖
- **SaveGame 영속화** (v1 제외 — 플레이 빌드 배포 시 재검토) · 제작 · 시즌패스/업적 · 아르케 조율·성장 · 파티/매칭

---

## 시연 시나리오 (Demo Flow)

> 포폴 영상/시연에서 한 번에 흐르도록. 각 단계가 곧 기술 셀링 포인트.

1. **메인메뉴 → 캐릭터 선택** — 카드 클릭/하이라이트, DataAsset 기반 캐릭터 목록
2. **허브 진입** — ESC 온디맨드 메뉴, 태그 기반 화면 라우팅
3. **상점에서 아이템 구매** — 크레딧 차감 + 인벤토리 자동 갱신(델리게이트)
4. **인벤토리 → 모듈 장착** — 캐파 검증, 장착 시 **스탯 보정 미리보기 → 실제 적용**(GAS)
5. **장착 결과가 HUD/스탯에 반영** — 모듈 보정이 실제 캐릭터 스탯에 적용된 것을 보여주며 마무리
> *(SaveGame "재시작해도 유지"는 v1 제외 — 영상 위주라 화면에 안 잡힘. 추후 플레이 빌드 배포 시 추가 고려.)*

## 포폴 셀링 포인트 (어필 포인트)

- **태그 기반 화면 라우팅** — 화면 추가 = 위젯 + 레지스트리 행(코드 0줄). 확장성 어필
- **WidgetTreeGen 자작 플러그인** — UI 16종 코드 생성. 툴링 역량 어필
- **GAS 연동 모듈 로드아웃** — 데이터 주도 스탯 합성 + Infinite GE 적용
- **Subsystem 기반 아키텍처** — Economy/Loadout/UIManager 책임 분리 + 델리게이트 기반 자동 UI 갱신
- *(SaveGame 영속화 — v1 제외, 추후 옵션)*

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
| SaveGame 영속화 *(미구현·설계)* | `LastFPSSaveGameSubsystem` (예정) | 시작 시 로드→`RestoreFrom` 복원, 변경 델리게이트 구독→`SaveNow` 즉시 저장 (복원 후 구독으로 루프 차단) |
