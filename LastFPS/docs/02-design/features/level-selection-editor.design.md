# [Design] 에디터 유틸리티 레벨 선택 툴 (JSON 저장)

## 컨텍스트 앵커 (Context Anchor)

| 차원 (Dimension) | 내용 (Content) |
|-----------|---------|
| WHY | 에디터에서 여러 맵을 빠르게 오가며 테스트하기 위해 개발자 전용 편의 도구 제공. |
| WHO | 프로젝트 개발자 및 레벨 디자이너. |
| RISK | 1. JSON 파일 입출력 경로 권한. 2. 에디터 전용 모듈 의존성 관리. |
| SUCCESS | 에디터 유틸리티 위젯에서 맵 목록이 표시되고, 즐겨찾기 등록 시 JSON 파일에 저장/로드됨. |
| SCOPE | Editor Utility Widget, JSON 파싱/저장 로직, 맵 에셋 검색 로직. |

## 1. 아키텍처 설계

### 1.1 JSON 매니저 (C++)
`ULastFPSEditorLevelHelper` 클래스를 생성하여 다음과 같은 정적 함수를 제공합니다.
- `SaveFavorites(TArray<FName> FavoriteMaps)`: JSON 파일로 저장.
- `LoadFavorites()`: JSON 파일에서 읽어오기.
- `GetAllMapAssets()`: AssetRegistry를 통해 프로젝트 내 모든 맵 정보를 가져오기.

### 1.2 UI (Editor Utility Widget)
- `WBP_EditorLevelSelection`: 메인 UI.
- `ScrollBox`를 사용하여 맵 리스트를 표시.
- 각 항목에 '별(즐겨찾기)' 버튼과 '이동' 버튼 배치.

## 2. 상세 코드 구조

### 2.1 Build.cs 설정
```csharp
PublicDependencyModuleNames.AddRange(new string[] { 
    "Core", "CoreUObject", "Engine", "InputCore", "Json", "JsonUtilities", "AssetRegistry" 
});

if (Target.Type == TargetType.Editor)
{
    PublicDependencyModuleNames.AddRange(new string[] { "Blutility", "UMGEditor", "UnrealEd" });
}
```

### 2.2 JSON 데이터 구조
```json
{
  "favorites": [
    "LastLobbyMap",
    "LastMatchMap"
  ]
}
```

## 3. 상세 설계 (옵션 C 기준)

### 3.1 폴더 구조
- `Source/LastFPS/Editor/LevelSelection/LastFPSEditorLevelHelper.h / .cpp`: JSON 입출력 및 맵 검색 로직.
- `Content/Editor/WBP_EditorLevelSelection`: Editor Utility Widget (에셋).

## 4. 구현 단계 (Do Phase Guide)
1. **Module 추가**: `LastFPS.Build.cs`에 필요한 모듈 추가.
2. **C++ Helper 클래스**: `Editor/LevelSelection/` 폴더에 JSON 읽기/쓰기 및 맵 검색 로직 구현.
3. **위젯 생성**: `Editor Utility Widget` 생성 및 C++ 함수 바인딩.
4. **테스트**: 에디터에서 실행하여 즐겨찾기 보존 여부 확인.

## 5. 작업 목록 (Tasks)
- [x] [Plan] 에디터 유틸리티 레벨 선택 툴 (JSON 저장)
- [x] [Design] 에디터 유틸리티 레벨 선택 툴 (JSON 저장)
- [x] [Do] Build.cs 모듈 업데이트
- [x] [Do] Editor 폴더 구조화 및 Helper 클래스 이동
- [ ] [Do] Editor Utility Widget (WBP) 제작
- [ ] [Check] 기능 검증
