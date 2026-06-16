# 모듈 UI — 에디터(WBP) 작업 가이드

> 작성: 2026-06-16 · 관련 C++: `LastFPSModuleScreenWidget` / `ModuleEntryWidget` / `ModuleSlotWidget`
> 백엔드/시스템 문서 → [`Module_System.md`](Module_System.md)

C++로 위젯 3종은 끝났고, 여기 적힌 대로 **WBP 3개 + 레지스트리 행 + 진입 버튼**만 만들면 인게임에서 작동한다.

---

## 0. 선행 — 빌드 먼저

새 C++ 위젯 클래스라 **빌드 전에는 부모 클래스 목록에 안 뜬다.** 먼저 빌드(VS 또는 에디터 Compile) → 그다음 WBP 생성.

## 핵심 규칙 — 바인딩 이름
위젯 트리에서 **위젯 이름을 아래 표의 이름과 정확히 똑같이**(대소문자 구분) 지어야 C++가 자동으로 잡는다. 다 `BindWidgetOptional`이라 일부 빠져도 컴파일은 되지만, 해당 칸은 안 보인다.

---

## 1. WBP_ModuleEntry — 보유 모듈 목록 한 줄

- **부모 클래스:** `LastFPSModuleEntryWidget`
- 한 줄짜리 가로 레이아웃. 좌측 아이콘 + 이름/스탯 + 우측 캐파/장착 버튼.

```
Root (Border 또는 SizeBox, 높이 60~80 권장)
└─ HorizontalBox
   ├─ Overlay  (아이콘 묶음, 고정폭 ~64)
   │  ├─ [Image]  Img_RarityBorder     ← 희귀도 색 배경/테두리
   │  └─ [Image]  Image_Icon           ← 모듈 아이콘
   ├─ VerticalBox  (Fill)
   │  ├─ [TextBlock]  TB_ModuleName     ← 모듈 이름
   │  └─ [TextBlock]  TB_Stats          ← 스탯 요약(멀티라인, 예 "공격력 +25")
   ├─ [TextBlock]  TB_Capacity          ← 캐파 코스트 (숫자)
   ├─ [TextBlock]  TB_Count             ← 보유 수량 "x3" (1개면 자동 숨김)
   └─ [공통버튼]   Button_Equip         ← "장착"
```

| 바인딩 이름 | 위젯 타입 | 용도 |
|---|---|---|
| `Image_Icon` | Image | 모듈 아이콘 (없으면 자동 숨김) |
| `Img_RarityBorder` | Image | 희귀도 색 (C++가 ColorAndOpacity 지정) |
| `TB_ModuleName` | TextBlock | 모듈 이름 |
| `TB_Stats` | TextBlock | 스탯 요약 (Auto-wrap/멀티라인 켜기) |
| `TB_Capacity` | TextBlock | 캐파 코스트 |
| `TB_Count` | TextBlock | 보유 수량 (1개면 숨김) |
| `Button_Equip` | **공통 버튼**(부모 `ULastFPSButtonBase`) | 장착. 캐파/슬롯 없으면 C++가 자동 비활성 |

> **공통 버튼**: 기존 공통 버튼 WBP(부모가 `ULastFPSButtonBase`인 것)를 배치하고 이름을 `Button_Equip`로 변경. 버튼 안 텍스트는 "장착"으로.

---

## 2. WBP_ModuleSlot — 장착 슬롯 1칸

- **부모 클래스:** `LastFPSModuleSlotWidget`
- 정사각형 칸. 비었으면 `Img_Empty`만, 장착되면 아이콘+이름. 클릭하면 해제.

```
Root (SizeBox, 정사각 96~128 권장)
└─ 공통버튼  Button_Slot              ← 칸 전체를 덮는 버튼(클릭=해제)
   └─ Overlay
      ├─ [Image]  Img_Empty           ← 빈 슬롯 표시(점선 등). 장착 시 자동 숨김
      ├─ [Image]  Img_RarityBorder    ← 희귀도 색. 빈 슬롯이면 숨김
      ├─ [Image]  Image_Icon          ← 장착 모듈 아이콘
      └─ VerticalBox (하단 정렬)
         ├─ [TextBlock]  TB_ModuleName ← 이름(짧게)
         └─ [TextBlock]  TB_Stats      ← 스탯 요약(선택, 작게)
```

| 바인딩 이름 | 위젯 타입 | 용도 |
|---|---|---|
| `Button_Slot` | **공통 버튼**(`ULastFPSButtonBase`) | 클릭 시 해제. 빈 슬롯이면 자동 비활성 |
| `Img_Empty` | Image | 빈 슬롯 플레이스홀더 |
| `Img_RarityBorder` | Image | 희귀도 색 |
| `Image_Icon` | Image | 장착 모듈 아이콘 |
| `TB_ModuleName` | TextBlock | 모듈 이름 |
| `TB_Stats` | TextBlock | 스탯 요약(선택) |

> 버튼이 칸 전체를 덮고 그 위에 Overlay로 아이콘을 얹는 구조가 클릭 영역 잡기 쉽다.

---

## 3. WBP_Module — 모듈 화면 본체

- **부모 클래스:** `LastFPSModuleScreenWidget` (← `ContentScreenWidget` 상속이라 타이틀/닫기 공통)
- 좌측 = 보유 모듈 목록(스크롤), 우측 = 장착 슬롯 + 캐파 + 스탯 미리보기.

```
Root (Border/Overlay 전체 배경)
└─ VerticalBox
   ├─ HorizontalBox  (타이틀 바)
   │  ├─ [TextBlock]  TB_Title          ← ContentScreen 공통(화면 제목)
   │  └─ [공통버튼]   Button_Close       ← ContentScreen 공통(닫기/뒤로)
   └─ HorizontalBox  (본문, Fill)
      ├─ VerticalBox  (좌: 보유 목록)
      │  ├─ [TextBlock]  "보유 모듈"      ← 고정 라벨(바인딩 불필요)
      │  ├─ Overlay (Fill)
      │  │  ├─ [ScrollBox]  Box_OwnedModules   ← 엔트리들이 여기 채워짐
      │  │  └─ [TextBlock]  TB_Empty           ← "보유한 모듈이 없습니다" (없을 때만 표시)
      │  └─ ...
      └─ VerticalBox  (우: 장착/미리보기)
         ├─ [TextBlock]  "장착"          ← 고정 라벨
         ├─ [HorizontalBox]  Box_Slots   ← 슬롯들이 여기 채워짐(4칸 가로)
         ├─ HorizontalBox
         │  ├─ [TextBlock]  "캐파"        ← 고정 라벨
         │  └─ [TextBlock]  TB_Capacity   ← "8 / 10"
         └─ VerticalBox
            ├─ [TextBlock]  "스탯 보정"   ← 고정 라벨
            └─ [TextBlock]  TB_StatPreview ← "체력 +50\n공격력 +25" (멀티라인)
```

| 바인딩 이름 | 위젯 타입 | 용도 |
|---|---|---|
| `TB_Title` | TextBlock | (공통) 화면 제목 |
| `Button_Close` | 공통 버튼 | (공통) 닫기/뒤로 |
| `Box_OwnedModules` | **ScrollBox** (또는 VerticalBox) | 보유 모듈 엔트리 컨테이너 |
| `Box_Slots` | **HorizontalBox** | 장착 슬롯 컨테이너 |
| `TB_Capacity` | TextBlock | 사용/최대 캐파 |
| `TB_StatPreview` | TextBlock | 합산 보정(멀티라인) |
| `TB_Empty` | TextBlock | 보유 0일 때 안내 |

### 디테일 패널 설정 (WBP_Module 선택 후)
| 속성 | 값 |
|---|---|
| `Item Table` | `DT_ItemData` |
| `Entry Widget Class` | `WBP_ModuleEntry` |
| `Slot Widget Class` | `WBP_ModuleSlot` |
| `Screen Title` | "모듈" (ContentScreen 공통, 타이틀 표시용) |

> `Box_*`는 `PanelWidget`이면 다 되지만 — 목록은 **ScrollBox**(스크롤), 슬롯은 **HorizontalBox**(가로 4칸) 권장.

---

## 4. 화면 라우팅 등록 — DA_ScreenRegistry

`DA_ScreenRegistry`에 행 1개 추가 (기존 인벤토리/상점 행과 동일 형식):

| 필드 | 값 |
|---|---|
| (Key) 태그 | `UI.Screen.Module` |
| `WidgetClass` | `WBP_Module` |
| `LayerTag` | `UI.Layer.Menu` |
| `DisplayName` | "모듈" |
| `bShowInHubMenu` | 취향 (허브 ESC 메뉴 자동 노출 여부) |

> 태그 `UI.Screen.Module`이 태그 목록에 없으면 GameplayTag로 새로 추가(프로젝트 설정 또는 태그 매니저).

## 5. 진입 버튼
모듈 화면을 열 곳을 하나 연결:
- **허브 메뉴 버튼**: `WBP_Hub`에 "모듈" 버튼 추가 → 클릭 시 `OpenScreenOrNotice(UI.Screen.Module)` (기존 Inventory/Shop 버튼과 동일 패턴)
- 또는 **인벤토리 화면**에서 "모듈 관리" 버튼으로 진입
- 또는 NPC `ScreenToOpen = UI.Screen.Module` (기존 NPC 상호작용 패턴)

---

## 6. 작동 테스트 체크리스트
1. 빌드 → WBP 3종 + 레지스트리 행 + 진입 버튼 완료
2. (모듈 보유 시드) `DefaultGame.ini`의 `StartingOwnedItems` 또는 상점 구매로 모듈 보유
3. 진입 버튼 → 모듈 화면 열림
4. 좌측 보유 목록 클릭 → 슬롯에 장착됨, 캐파/스탯 미리보기 갱신
5. 슬롯 클릭 → 해제됨
6. 캐파 한도 초과 시 목록 버튼 비활성 확인
7. (빌드 들어간 게임에서) 모듈 장착 후 전투 진입 → 스탯 실제 반영 확인 (`showdebug abilitysystem`)

---

## 참고 — 바인딩 이름 전체 요약
```
WBP_ModuleEntry : Image_Icon, Img_RarityBorder, TB_ModuleName, TB_Stats, TB_Capacity, TB_Count, Button_Equip
WBP_ModuleSlot  : Image_Icon, Img_RarityBorder, Img_Empty, TB_ModuleName, TB_Stats, Button_Slot
WBP_Module      : TB_Title, Button_Close, Box_OwnedModules, Box_Slots, TB_Capacity, TB_StatPreview, TB_Empty
                  + 디테일: ItemTable, EntryWidgetClass, SlotWidgetClass, ScreenTitle
```
