# 아웃게임 UI 시스템

> 한 줄 요약: **PlayerController는 "UI 리모컨", Subsystem은 "셋톱박스", ScreenRegistry는 "채널 목록".**
> 화면 추가는 코드가 아니라 데이터(레지스트리 한 줄)로 한다. 마지막 갱신: 2026-06-06

---

## 1. 큰 그림 (리모컨 비유)

```
🎮 게임코드 / NPC / 버튼            ← "손"  (리모컨 버튼을 누름)
        │  PC->OpenScreen(UI.Screen.Inventory)
        ▼
📱 ALastFPSPlayerController          ← "리모컨"  (UI 진입점, 얇음)
        │  Subsystem에 그대로 위임
        ▼
📦 ULastFPSUIManagerSubsystem       ← "셋톱박스"  (실제 push/pop 로직, 딱 하나)
        │  "이 태그 = 무슨 위젯?" 조회
        ▼
📋 ULastFPSScreenRegistry (DataAsset) ← "채널 목록"  (태그 → 위젯/레이어)
        │  실제 위젯을 로드해 레이어에 올림
        ▼
🧱 UPrimaryGameLayout (4 레이어)     ← "화면"  (Game / Menu / GameMenu / Modal)
```

| 층 | 클래스 / 파일 | 역할 |
|---|---|---|
| 리모컨 | `Game/LastFPSPlayerController` | UI 진입점. `OpenScreen` / `CloseScreen` 만. |
| 셋톱박스 | `UI/LastFPSUIManagerSubsystem` | 태그→조회→레이어 push/pop. 중복오픈 방지. **로직 단일 소스** |
| 채널목록 | `UI/LastFPSScreenRegistry` + `LastFPSScreenTypes` | 태그 → `{위젯, 레이어, 이름, 아이콘}` |
| 설정 | `UI/LastFPSUISettings` | 어떤 Registry를 쓸지 (Project Settings) |
| 화면 베이스 | `UI/LastFPSContentScreenWidget` | 풀스크린 콘텐츠 공용(타이틀/닫기) |
| 레이아웃 | `UI/LastFPSPrimaryGameLayout` + `LastFPSUITags` | 4계층 레이어 스택 |

---

## 2. 화면 띄우는 법 (단 하나의 길)

```cpp
// 어디서든 — PC가 리모컨
PC->OpenScreen(UI.Screen.Shop);    // 열기
PC->CloseScreen(UI.Screen.Shop);   // 닫기
```
- BP에서도 동일: PlayerController 노드 → `Open Screen` (Screen Tag 핀).
- 화면별 전용 메서드(`TryPushXxx`)는 **존재하지 않는다.** 전부 태그 하나로.

### 진입/ESC 화면은 **GameMode**가 지정 (→ PC는 1개로 공유)
"어떤 화면을 띄울지"는 맵마다 다른 규칙이라 **GameMode가 태그 2개를 소유**한다.
PC는 BeginPlay에 GameMode에서 읽어와(`CacheUIConfigFromGameMode`) 연다. 덕분에 PlayerController는 맵마다 다를 필요 없이 **1개로 공유**된다.

| GameMode 필드 | 동작 |
|---|---|
| `InitialScreenTag` | 맵 진입 시 자동으로 연다 (PrimaryGameLayout 준비될 때까지 재시도). 비우면 안 엶. |
| `EscMenuScreenTag` | ESC로 연다(허브 메뉴 등). 비우면 ESC 무시. 닫기는 CommonUI Back. |

> 네트워크 주의: PC가 `GetAuthGameMode`로 읽으므로 standalone/호스트에선 OK,
> 순수 클라이언트는 못 읽음. 완전 대응은 GameState 복제로 추후 (TODO).

---

## 3. 흐름

### 맵 이동 (예: MainMenu → Hub)
이전 맵의 월드(PC·위젯)가 통째로 파괴되므로 **수동 Close 불필요**. 새 맵의 GameMode가 지정한 `InitialScreenTag`를 PC가 읽어서 연다.

### 같은 맵 안 (예: 허브에서 인벤 ↔ 상점)
레이어 스택. `OpenScreen`으로 쌓고, `CloseScreen` 또는 ESC/Back으로 pop.

### `OpenScreen` 내부 (셋톱박스)
1. 이미 열렸으면 그대로 반환 (중복 방지)
2. Registry 조회 → `FLastFPSScreenDef`
3. 위젯 클래스 로드(`LoadSynchronous`)
4. `LayerTag` 레이어에 push (없으면 `UI.Layer.Menu`)
5. Menu 레이어면 입력모드 자동 Menu 전환(이동 정지), pop되면 복귀

---

## 4. 콘텐츠 추가하는 법 (코드 0줄)

새 화면(예: 상점):
```
1. 태그        UI.Screen.Shop        (DefaultGameplayTags.ini — 시드 8종 이미 있음)
2. WBP_Shop    부모 = LastFPSContentScreenWidget 으로 위젯 제작
3. DA_ScreenRegistry 행 추가:
      UI.Screen.Shop → { WidgetClass=WBP_Shop, LayerTag=UI.Layer.Menu, DisplayName="상점" }
4. 진입점 (택1, 둘 다 데이터로):
      상점 NPC      → BP에서 ScreenToOpen = UI.Screen.Shop (F 누르면 열림)
      허브 메뉴 버튼 → 이미 OpenScreen(UI.Screen.Shop) 배선됨 (등록 전엔 "준비 중" 공지)
```

---

## 5. 에디터 세팅 (최초 1회)

1. **DA_ScreenRegistry** 생성 (우클릭 → Data Asset → `LastFPSScreenRegistry`)
2. **Project Settings → Game → LastFPS UI → Screen Registry** = `DA_ScreenRegistry`
3. 화면들 등록: `UI.Screen.MainMenu`→메인메뉴 WBP, `.CharacterSelect`→캐릭선택 WBP, `.HubMenu`→`WBP_Lobby` …
4. **GameMode**에 태그 지정 (PC 아님):
   - `BP_MainMenu_GameMode`: `InitialScreenTag = UI.Screen.MainMenu`
   - `BP_CharacterSelect_GameMode`: `InitialScreenTag = UI.Screen.CharacterSelect`
   - `BP_Hub_GameMode`: `EscMenuScreenTag = UI.Screen.HubMenu`
5. **PC 1개로 통합**: 위 3개 GameMode의 `PlayerControllerClass`를 전부 `BP_LastFPS_PlayerController` 하나로 →
   `BP_MainMenu_..._PlayerController` / `BP_CharacterSelect_PlayerController` 삭제
6. NPC: `BP_NPC_*`의 `ScreenToOpen`에 열 화면 태그 지정 (예: 상점 NPC = `UI.Screen.Shop`)

---

## 6. 스파게티 방지 규칙 (불변)

1. **화면 띄우는 길은 `OpenScreen` 하나.** 화면별 메서드 금지.
2. **코드는 위젯 클래스를 모른다.** 태그만 안다. "태그→위젯"은 Registry(데이터)만.
3. **띄우는 로직은 Subsystem 한 곳.** PC는 위임만(얇게 유지).

---

## 7. 범위 밖 (이 시스템이 안 다루는 것)

- **HUD** (`ShowHitMarker`, `GetHUDWidget`, `bPushHUDOnBeginPlay`) — 인게임 전투 오버레이로 **인게임 팀 영역**. 항상 떠 있는 Game-레이어 오버레이라 "네비게이션 화면"이 아니므로 라우팅에서 제외. 현재 휴면.
- **모달/공지** (`ShowConfirm`, `ShowNotice`) — Modal 레이어 팝업. 화면 스택과 별개의 짧은 다이얼로그라 PC가 직접 push (의도적 예외).
