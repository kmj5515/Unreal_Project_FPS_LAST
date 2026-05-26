# Lyra CommonUI 통합 (LastFPS)

## 적용된 내용

- **플러그인**: `CommonUI`, `CommonGame`, `CommonUser`, `AsyncMixin`, `CommonLoadingScreen`, `ModularGameplayActors`
- **C++**: `ULastFPSUIManagerSubsystem`, `ULastFPSUIPolicy`, `ULastFPSPrimaryGameLayout`
- **레이어 태그**: `UI.Layer.Game` / `GameMenu` / `Menu` / `Modal`
- **HUD**: `WBP_HUD` → `UI.Layer.Game`, 스코어보드 → `UI.Layer.GameMenu`
- **PlayerController**: `ACommonPlayerController` 상속 (CommonLocalPlayer 이벤트)
- **LocalPlayer**: `UCommonLocalPlayer` (`DefaultEngine.ini`)

## 에디터에서 할 일 (필수)

### 1. 프로젝트 리빌드

`LastFPS.uproject` 우클릭 → Generate Visual Studio project files → **Development Editor** 전체 빌드.

### 2. WBP 부모 클래스 변경

| 위젯 | 새 Parent Class |
|------|-----------------|
| `WBP_HUD` | `LastFPSHUDWidget` (이제 `UCommonActivatableWidget` 기반) |
| `WBP_Scoreboard` | `LastFPSScoreboardWidget` |
| 일시정지/메뉴 (신규) | `CommonActivatableWidget` |

에디터에서 Parent Class가 자동 갱신되지 않으면 WBP를 열고 Parent를 수동으로 바꿉니다.

### 3. CommonInput (메뉴 입력 라우팅)

**Project Settings → Common Input**:

- `Input Data`에 CommonUI용 DataTable 지정 (Lyra `B_CommonInputData` 참고해 자체 제작 가능)
- 메뉴 위젯은 `CommonActivatableWidget` + `Input Mapping` 설정

### 4. CommonLoadingScreen (선택)

Lyra `Content/UI/Foundation/LoadingScreen/W_LoadingScreen_Host`를 참고해 호스트 위젯을 만든 뒤:

`Config/DefaultGame.ini`:

```ini
[/Script/CommonLoadingScreen.CommonLoadingScreenSettings]
LoadingScreenWidget=/Game/UI/W_LastFPSLoadingHost.W_LastFPSLoadingHost_C
```

`CommonLoadingScreen`을 쓰면 `LastFPSGameInstance`의 수동 로딩 UI와 겹칠 수 있습니다. 하나만 사용하세요.

### 5. 메뉴 Push 예시 (Blueprint)

`UI.Layer.Menu`에 메뉴를 올릴 때:

- `Push Content to Layer for Player` (Local Player, `UI.Layer.Menu`, MenuWidgetClass)

C++:

```cpp
UCommonUIExtensions::PushContentToLayer_ForPlayer(
    LocalPlayer, LastFPSUITags::Layer_Menu(), MyMenuClass);
```

## 레이어 용도

| 태그 | 용도 |
|------|------|
| `UI.Layer.Game` | 인게임 HUD |
| `UI.Layer.GameMenu` | Tab 스코어보드, 일시정지 |
| `UI.Layer.Menu` | 로비/메인 메뉴 |
| `UI.Layer.Modal` | 확인 창 |

## 커스텀 Primary Layout (선택)

`ULastFPSPrimaryGameLayout`은 C++에서 4개 스택을 자동 생성합니다.  
디자인을 바꾸려면 `WBP_PrimaryGameLayout` (Parent: `LastFPSPrimaryGameLayout`)을 만들고 `LayerStack_Game` 등에 `CommonActivatableWidgetStack`을 배치한 뒤 **Layer Tag**를 위 태그에 맞게 설정하세요.

`ULastFPSUIPolicy`의 `LayoutClass`를 해당 WBP로 바꾸려면 BP `B_LastFPSUIPolicy`를 만들어 `DefaultGame.ini`의 `DefaultUIPolicyClass`를 BP로 지정하면 됩니다.
