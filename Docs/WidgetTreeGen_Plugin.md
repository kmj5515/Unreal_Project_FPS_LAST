# WidgetTreeGen — JSON → UMG 위젯 블루프린트 생성 플러그인

> 작성: 2026-06-17 · 엔진: UE 5.7 · 위치: `LastFPS/Plugins/WidgetTreeGen/`
> 전체 레퍼런스(스키마 표/필드/타입) → 플러그인 폴더의 [`README.md`](../LastFPS/Plugins/WidgetTreeGen/README.md)

JSON으로 기술한 위젯 계층을 받아 **UMG 위젯 블루프린트(.uasset)** 를 자동 생성·컴파일·저장하는
**에디터 전용** 플러그인. 모듈 타입이 `Editor`라 런타임/패키징 빌드에는 절대 안 들어간다.

---

## 0. 선행 — 빌드 먼저

새 C++ 모듈이라 **빌드 전에는 메뉴/노드가 안 뜬다.** 프로젝트 파일 재생성 → 에디터 타깃 빌드.

```bat
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" ^
    -projectfiles -project="%CD%\LastFPS\LastFPS.uproject" -game -rocket -progress

"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" ^
    LastFPSEditor Win64 Development -project="%CD%\LastFPS\LastFPS.uproject" -waitmutex
```

`LastFPS.uproject`에 플러그인은 이미 `Enabled`로 등록해 둠.

---

## 1. 동작 흐름

1. `parentClass` 해석 (`UUserWidget` 또는 서브클래스, 풀패스 `LoadObject`).
2. `UWidgetBlueprintFactory` + `IAssetTools::CreateAsset` 로 WBP 생성.
3. 팩토리가 만든 기본 루트는 버리고 JSON 계층을 재귀로 다시 빌드.
   각 노드는 `name`으로 이름 지정 + `bIsVariable = true` → **변수로 노출**(디자이너/그래프 접근 가능).
4. **부모 패널 종류에 맞춰** 슬롯 세팅:
   - `CanvasPanel` 자식 → `position`/`size` 절대 좌표 (`UCanvasPanelSlot`).
   - `HorizontalBox`/`VerticalBox`/`Overlay` 자식 → `padding`/`halign`/`valign`/`fill`.
5. `MarkBlueprintAsStructurallyModified` → `CompileBlueprint` → 저장.

이름은 sanitize + 트리 내 유니크 보장. 타입은 매핑 TMap → 풀패스 `LoadObject` 폴백 (`ANY_PACKAGE` 미사용).

---

## 2. 에디터에서 실행하는 법 (3가지)

- **메뉴 A:** 상단 메뉴바 **LastFPS** → *Widget Tree Gen* 섹션 → **위젯 트리 생성기**
  → 디테일 패널에서 `Parent Class` / `Json Text`(또는 `Read From File`) / `Save Path` / `Asset Name`
  채우고 **Generate** 버튼(`CallInEditor`) 클릭.
- **메뉴 B:** **LastFPS** → *Widget Tree Gen* → **JSON 파일에서 생성...** → 파일 선택 시 즉시 생성.

> 메뉴 연동 방식: EditorUtility 플러그인의 "LastFPS" 서브메뉴는 **구식 `FNewMenuDelegate`(FMenuBuilder)** 로 채워져서
> UToolMenus `ExtendMenu`로는 항목이 병합되지 않는다(엔진이 신식 `FNewToolMenuDelegate` 서브메뉴만 DB 병합함).
> 그래서 `FEditorUtilityModule`에 확장 delegate(`OnExtendLastFPSMenu`)를 열고, WidgetTreeGen이 StartupModule에서 거기 바인딩해
> 항목을 추가한다. 의존 방향: WidgetTreeGen(PostEngineInit) → EditorUtility(PreDefault) — 나중 로드가 먼저 로드에 의존(정상).
- **에디터 유틸리티 위젯(Blutility):** 버튼 `OnClicked`에서 `BlueprintCallable` 노드 호출
  — *Generate Widget Blueprint From JSON Text* / *...From JSON File*.
  반환 `FWidgetTreeGenResult`(`bSuccess` / `AssetPath` / `ErrorMessage`).

샘플 JSON: `LastFPS/Plugins/WidgetTreeGen/Resources/sample_widget.json`
(샘플 실행 시 `/Game/UI/Generated/WBP_Generated` 생성 — Canvas 위 Button+Label, 하단 VerticalBox 메뉴).

---

## 3. 확장 포인트

- **새 위젯 타입 짧은 이름:** `WidgetTreeGenerator.cpp`의 `GetWidgetTypeAliasMap()`에 한 줄 추가.
  (커스텀 BP 위젯은 그냥 `type`에 풀패스 `"/Game/UI/WBP_Foo.WBP_Foo_C"` 넣으면 됨.)
- **새 슬롯 속성:** 같은 파일 `ApplySlot()` 의 패널 타입별 분기에 추가.
- **새 노드 필드:** `ParseNode()` + `FWidgetGenNode` 구조체.
