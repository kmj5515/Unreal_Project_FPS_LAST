# 아웃게임 리팩터 & UI 시스템 로드맵

> `Outgame_Dev.md`의 보조 트랙 — **"정리하면서 개발"**.
> 기반 클래스를 먼저 정리한 뒤, 그 위에 태그 기반 UI 화면 라우팅 시스템을 올린다.
> 기준 브랜치: `kmj-dev-roll` · 작성: 2026-06-06
> **UI 시스템 사용/구조 → [`UI_System.md`](UI_System.md) · 에디터 세팅 체크리스트 → [`UI_System_Editor_Setup.md`](UI_System_Editor_Setup.md)**

---

## 진단 — 핵심 프레임워크 현황

| 클래스 | 상태 | 정리 포인트 |
|---|---|---|
| `LastFPSGameModeBase` | 단일 베이스 + 맵별 BP 파생 추정 | `CharacterPawnClasses` 3중 중복 / 맵별 GM 역할 불명확 |
| `LastFPSGameInstance` | Travel/Save 양호 ✅ | 맵 경로 `/Test/` 하드코딩 |
| `LastFPSPlayerController` | **비대** — 책임 4개 혼재 | UI라우팅 / 캐릭터선택 / NPC상호작용 / 모달헬퍼 |
| `LastFPSPlayerState` | 전투통계·GAS = **인게임 팀 영역** | 아웃게임 수정 금지 (경계선: `SelectedCharacterIndex`) |

### 확인된 중복 / 부채
- **`SelectedCharacterIndex` 이중 보유**: `PlayerController`(Replicated)와 `PlayerState`(Replicated) 양쪽 → 단일 소스 정리 필요.
- **캐릭터 목록 3중 중복**: `CharacterPawnClasses`(GM) / `SelectableCharacterClasses`(PC) / `CharacterNames`(위젯) → DataAsset 통합 (`Outgame_Dev.md:189`).
- **핵심 통찰**: PC의 UI 라우팅 이관 = "PC 정리" + "UI 시스템 구축"이 동일 작업. 합쳐서 진행.

---

## Phase 0 — 기반 클래스 정리 (먼저)

> 새 시스템 올리기 전, 저위험 정리부터. 각 항목은 독립 커밋 단위.

### 0-1. PlayerController 정리 ✅ 완료·빌드검증 (2026-06-06)
- [x] `RetryPush` 타이머 3종 공용 헬퍼로 통합 → 이후 UI 흡수로 자연 소멸
- [x] 책임 구획 정리(섹션 배너): UI진입점 / 캐릭터선택 / 상호작용 / 모달·공지 / HUD
- [x] **PC를 "얇은 리모컨"으로 전환** — 화면별 push 메서드(`TryPushMainMenu/CharSelect/OpenHubMenu`) + 위젯 하드참조 제거, `OpenScreen/CloseScreen`(Subsystem 위임) + `InitialScreenTag`/`EscMenuScreenTag`만 남김

> **스파게티 방지 완료 기준:**
> - [x] 화면 띄우는 길 = `OpenScreen` 하나 (HUD/모달은 의도적 예외, `UI_System.md §7`)
> - [x] PC는 위젯 클래스를 모름 (태그만)
> - [x] **고아 BP/죽은 경로 제거** — 죽은 C++ 경로 제거 + 고아 `BP_PC_Hub` `git rm` 완료 (2026-06-06). (Scoreboard 에셋은 안 씀 — 신경 안 씀)

### 0-2. GameMode / PlayerController 현황 (2026-06-06 조사 완료)

> 방법: `.umap`/`.uasset` 바이너리 참조 추적. World Settings GameMode override 및 GM의
> `PlayerControllerClass`는 에디터에서 최종 확인 권장이나, 아래는 직렬화 참조상 일관된 결과.

**실제 연결표 (맵 → GameMode → PlayerController):**

| 맵 | GameMode | 실제 PlayerController | 진입 플래그 |
|---|---|---|---|
| MainMenuMap | `BP_MainMenu_GameMode` (C++ 직속) | `BP_MainMenu_LastFPS_PlayerController` | `bPushMainMenuOnBeginPlay` |
| CharacterSelectMap | `BP_CharacterSelect_GameMode` (C++ 직속) | `BP_CharacterSelect_PlayerController` | `bPushCharacterSelectOnBeginPlay` |
| HubMap | `BP_Hub_GameMode` → `BP_GameModeBase` | **`BP_LastFPS_PlayerController`** (플래그 없음) | 없음 |
| NewMap | `BP_GameModeBase_Child` → `BP_GameModeBase` | `BP_LastFPS_PlayerController` | 없음 |
| TestMap / LastMatchMap1 | override 없음 → 프로젝트 기본 | 미지정 | - |

**계층 (비일관):**
```
C++ ALastFPSGameModeBase                 C++ ALastFPSPlayerController
 ├ BP_GameModeBase (PC=BP_LastFPS_PC)     ├ BP_LastFPS_PlayerController (플래그 0)
 │   ├ BP_GameModeBase_Child  (NewMap)     │    └ BP_PC_Hub  ⚠️고아 (bPushHubOnBeginPlay)
 │   └ BP_Hub_GameMode        (HubMap)     ├ BP_MainMenu_LastFPS_PlayerController
 ├ BP_MainMenu_GameMode                    └ BP_CharacterSelect_PlayerController
 └ BP_CharacterSelect_GameMode
```

**🔴 발견된 문제:**
1. **`BP_PC_Hub`는 고아** — `bPushHubOnBeginPlay`/`HubWidgetClass`를 가졌으나 **어떤 GameMode도 PC로 지정하지 않음**. HubMap은 `BP_Hub_GameMode`가 부모(`BP_GameModeBase`)로부터 `BP_LastFPS_PlayerController`(빈 PC)를 상속받아 사용. → **앞선 ESC 토글/CoreRedirect 작업이 실제 Hub에 적용되지 않는 PC에 들어가 있음.**
2. **문서 오류** — `Outgame_Dev.md:16`은 `BP_CharacterSelect_PlayerController`/`BP_MainMenu_LastFPS_PlayerController`를 "삭제됨(`1c522b5`)"이라 기록했으나, **존재하며 실사용 중**.
3. **현재 Hub엔 자동 UI push가 없음** — 빈 PC라 진입 시 로비/HUD 모두 안 뜸 (WASD는 자연히 정상).

**정리 액션 (깊이 결정 후):**
- [x] 고아 `BP_PC_Hub` 제거 (`git rm`). → HubMap은 `BP_LastFPS_PlayerController` 사용 유지.
- [ ] 허브 ESC 메뉴: Hub가 쓰는 PC(`BP_LastFPS_PlayerController` 또는 신규 허브 PC)에 `EscMenuScreenTag = UI.Screen.HubMenu` 지정 (에디터)
- [x] `Outgame_Dev.md:16` 잘못된 "삭제됨" 기록 정정
- [ ] `CharacterPawnClasses` → `CharacterDefinition` DataAsset 통합 (인게임 팀 완료 후, `Outgame_Dev.md:189`)
- [ ] `DebugFlow` 공용 헬퍼 유지

### 0-3. GameInstance 정리
- [ ] 맵 경로 `/Test/` 제거·상수화 (`Outgame_Dev.md:204`) — 릴리즈 전, 지금은 정리만
- [ ] Travel API 표면 점검 (`RequestTravelTo*` 4종 OK)

### 0-4. PlayerState (경계 — 신중)
- [ ] 전투통계·GAS는 **인게임 팀 영역, 수정 금지** 명시
- [ ] `SelectedCharacterIndex` 단일 소스화: PS를 SoT로, PC 중복 제거 검토 (인게임 팀과 협의)

---

## 채택 구조 — B (경량 통합, FD/Lyra 정석 축소판)

> 결정 2026-06-06. 루터슈터는 "통합 게임플레이 + 얹은 메타 UI". 맵별 클래스 제거, UI는 데이터화.

```
[프런트엔드 — UI 전용]              [게임플레이 — 인-월드]
 FrontendGameMode                   GameplayGameMode (인게임 팀과 공유)
  └ MainMenu (폰X, 진입화면=MainMenu) └ Hub (+ 전투) (폰O)
        \________________  ________________/
                         \/
        ALastFPSPlayerController  (1개 공유, 슬림화)
"진입 시 어떤 화면"은 GameMode가 OpenScreen(Tag)로 결정 → PC의 bPush* 플래그·맵별 PC 전멸
```
- 풀 Experience/GameFeature(A안)는 **인게임 팀 영역이라 보류**. UI 데이터화는 우리 ScreenRegistry로 달성.
- 게임플레이 GameMode/PC 통합은 인게임 팀 소유권 확인 후 진행 (열린 질문).

## Phase A — UI 화면 라우팅 시스템 (코어) ✅ C++ 완료·빌드검증 (2026-06-06)

> 콘텐츠 추가 = "위젯 BP 1개 + 레지스트리 행 1개 + 태그 1개". PC 코드 불변.

- [x] `UI/LastFPSScreenTypes.h` — `FLastFPSScreenDef` { `TSoftClassPtr<위젯>`, `LayerTag`, `DisplayName`, `Icon`, `bShowInHubMenu` }
- [x] `UI/LastFPSScreenRegistry.h/.cpp` — `UPrimaryDataAsset`, `TMap<FGameplayTag, FLastFPSScreenDef>` + `FindScreen`
- [x] `UI/LastFPSUISettings.h/.cpp` — `UDeveloperSettings`(Project Settings > Game > LastFPS UI), 레지스트리 `TSoftObjectPtr`
- [x] `LastFPSUIManagerSubsystem` 확장 — `OpenScreen(Tag, PC)` / `CloseScreen(Tag)` / `IsScreenOpen(Tag)` / `Get(WorldContext)`
- [x] `UI/LastFPSContentScreenWidget.h/.cpp` — 풀스크린 베이스(타이틀/닫기), Back→닫기
- [x] 화면 태그 — `UI.Screen.*` 시드 8종 (`DefaultGameplayTags.ini`) + `DeveloperSettings` 모듈 추가
- [x] **(에디터)** `DA_ScreenRegistry` 생성 → Project Settings 할당 + MainMenu/CharacterSelect/HubMenu 등록
- [ ] **(에디터)** 콘텐츠 화면 WBP(Inventory/Mission/Shop/Settings)는 `ULastFPSContentScreenWidget` 상속으로 제작 (Phase C)

## Phase B — 진입 배선 (둘 다: NPC + ESC 탭) ✅ 완료·PIE 검증 (2026-06-07)

- [x] **PC 1개로 통합** — 진입/ESC 태그를 `GameModeBase`로 이동(`InitialScreenTag`/`EscMenuScreenTag`), PC는 `CacheUIConfigFromGameMode`로 읽어 연다
- [x] ESC 진입점 — `EscMenuScreenTag` 기반 `HandleEscMenu` (열기)
- [x] **ESC 닫기** — `ULastFPSActivatableWidget`이 활성화 시 포커스 확보 + `NativeOnKeyDown`에서 ESC→`DeactivateWidget` (CommonUI Back DataTable 불필요)
- [x] `ALastFPSNPCBase`에 `FGameplayTag ScreenToOpen` — 등록 화면이면 `OpenScreen`, 아니면 대화 공지 폴백
- [x] **상호작용 키 F→G** — F는 캐릭터 Enhanced Input 궁극기(`IA_Ultimate`)와 충돌해서 변경
- [x] `WBP_Lobby`(=`WBP_Hub`) 버튼 — `OpenScreenOrNotice`로 교체
- [x] `LastFPSUITags`에 `Screen_*` 접근자 추가 (매직스트링 방지)
- [x] **(에디터)** GameMode 태그 지정 + 3 GameMode PC클래스 통합 + 잉여 PC BP 삭제 → **PC `BP_LastFPS_PlayerController` 1개만**
- [x] **(에디터)** `MainMenu` Start → 캐릭터 선택 직행(임시 스킵 제거)

> **PIE 검증 완료**: 허브 WASD 이동 / NPC **G** 상호작용 / **ESC** 메뉴 토글 / 메인메뉴 Quit 확인팝업 / PC 1개. (상세 → `UI_System_Editor_Setup.md §8`)

## Phase C — 콘텐츠 화면 (양산)

> 코어·배관·진입 모두 동작. 이제 화면 콘텐츠만 찍어내면 됨 (콘텐츠 = WBP + 레지스트리 행).

- [ ] **Shop** — `WBP_Shop`(`ContentScreenWidget` 상속) + `UI.Screen.Shop` 등록 → Quartermaster NPC(G)로 검증 (이미 `ScreenToOpen=Shop` 지정됨)
- [ ] **Settings** — `WBP_Settings` + `UI.Screen.Settings` 등록 → 로비 메뉴/메인메뉴 버튼에서 오픈
- [ ] 이후 인벤토리 / 미션 / 계승자 관리 순으로 양산 (`Outgame_Dev.md` Phase 2~3)

---

## Outgame_Dev.md 항목 매핑

| 본 로드맵 | Outgame_Dev.md |
|---|---|
| 0-1 RetryTimer 통합 | 기술부채 `:207` |
| 0-2 Character DataAsset | 기술부채 `:189`, `:193` |
| 0-3 맵 경로 정리 | 기술부채 `:204` |
| Phase A 위젯 소프트레퍼런스 | 기술부채 `:201` |
| Phase C Settings | `:83`, `:98` |
| Phase C 인벤/미션/상점 | Phase 2-2, 3-2, 4 |
