# [Guide] 에디터 유틸리티 위젯(UMG) 제작 및 연동

C++ 백엔드 구현이 완료되었으므로, 이제 에디터에서 사용할 UI를 제작하고 연결해야 합니다. 제가 `.uasset` 파일을 직접 생성할 수는 없지만, 아래 가이드를 따라 위젯을 구성하시면 바로 작동합니다.

## 1. 위젯 생성 및 기본 설정
1.  **에셋 생성**: `Content/Editor/` 폴더에서 우클릭 -> **Editor Utilities** -> **Editor Utility Widget**을 선택하고 이름을 `WBP_LevelSelectionEditor`로 지정합니다.
2.  **부모 클래스 변경** (선택 사항): 위젯 블루프린트 설정에서 부모 클래스를 `EditorUtilityWidget`으로 유지합니다.

## 2. UI 레이아웃 구성
- **[Canvas Panel]**
    - **[Vertical Box]** (전체 레이아웃)
        - **[Text]**: "레벨 선택 도구" (제목)
        - **[Button]**: "새로고침" (맵 목록 다시 읽기)
        - **[Scroll Box]**: 이름 `MapScrollBox` (맵 항목들이 나열될 곳)

## 3. 블루프린트 로직 구현

### A. 초기화 및 목록 생성 (Refresh logic)
위젯의 `Construct` 이벤트나 새로고침 버튼 클릭 시 다음 로직을 실행합니다:
1.  `MapScrollBox`의 모든 자식을 제거합니다 (`Clear Children`).
2.  C++ 함수 `GetAllMapAssets`를 호출합니다.
3.  반환된 `FLastFPSMapAssetInfo` 배열을 루프(`ForEach`) 돌립니다.
4.  각 항목마다 개별 맵 열(Row) 위젯을 생성하여 `MapScrollBox`에 추가합니다.

### B. 맵 열 위젯 (WBP_LevelRow) 구성
각 맵을 표시할 작은 위젯을 하나 더 만듭니다.
- **구성 요소**:
    - **CheckBox**: 즐겨찾기용 (하트/별 모양 이미지 권장)
    - **TextBlock**: 맵 이름 (`MapName`)
    - **Button**: "이동" 버튼
- **로직**:
    - **즐겨찾기 체크**: `OnCheckStateChanged` 시 현재 모든 즐겨찾기 목록을 가져와 해당 이름을 추가/삭제 후 C++ `SaveFavorites` 호출.
    - **이동 버튼**: 클릭 시 `ExecuteConsoleCommand` 노드를 사용하여 `open [PackagePath]` 명령 실행.

## 4. C++ 연동 팁
- `ULastFPSEditorLevelHelper`는 블루프린트 함수 라이브러리이므로, 블루프린트 어디서든 함수 이름(`GetAllMapAssets`, `SaveFavorites`, `LoadFavorites`)을 검색하면 바로 나타납니다.

---

### UI 제작 시 참고할 로직 구조 (의사 코드)
```python
# 새로고침 버튼 클릭 시
def OnRefreshClicked():
    MapScrollBox.ClearChildren()
    MapInfos = LastFPSEditorLevelHelper.GetAllMapAssets()
    
    for Info in MapInfos:
        NewRow = CreateWidget(WBP_LevelRow)
        NewRow.SetData(Info) # 이름, 경로, 즐겨찾기 여부 전달
        MapScrollBox.AddChild(NewRow)

# 이동 버튼 클릭 시
def OnOpenLevelClicked(PackagePath):
    # 에디터에서 레벨을 여는 콘솔 명령어
    ExecuteConsoleCommand("open " + PackagePath)
```

위 가이드를 따라 위젯을 제작해 보시겠어요? 제작 중 특정 노드 연결이나 로직 처리가 막히는 부분이 있다면 말씀해 주세요!
