# [Plan] 에디터 유틸리티 레벨 선택 툴 (JSON 저장)

## 요약 (Executive Summary)
개발 효율성을 높이기 위해 언리얼 에디터 내에서 사용할 수 있는 '레벨 선택 유틸리티 툴'을 개발합니다. 이 툴은 프로젝트의 맵 목록을 보여주고, 즐겨찾기 기능을 제공하며, 해당 데이터를 JSON 파일로 관리합니다.

## 컨텍스트 앵커 (Context Anchor)

| 차원 (Dimension) | 내용 (Content) |
|-----------|---------|
| WHY | 에디터에서 여러 맵을 빠르게 오가며 테스트하기 위해 개발자 전용 편의 도구 제공. |
| WHO | 프로젝트 개발자 및 레벨 디자이너. |
| RISK | 1. JSON 파일 입출력 경로 권한. 2. 에디터 전용 모듈 의존성 관리. |
| SUCCESS | 에디터 유틸리티 위젯에서 맵 목록이 표시되고, 즐겨찾기 등록 시 JSON 파일에 저장/로드됨. |
| SCOPE | Editor Utility Widget, JSON 파싱/저장 로직, 맵 에셋 검색 로직. |

## 1. 요구 사항

### 1.1 기능적 요구 사항
- 에디터 내에서 실행 가능한 Editor Utility Widget 제공.
- 프로젝트 내 모든 `.umap` 에셋 검색 및 리스트업.
- 맵 클릭 시 해당 레벨로 에디터 뷰포트 이동 (Open Level).
- 즐겨찾기 상태를 로컬 JSON 파일에 저장 및 불러오기.

### 1.2 비기능적 요구 사항
- 에디터 환경에서만 작동 (Runtime 제외).
- JSON 파일은 `Saved/Config/EditorFavorites.json` 등에 저장.

## 2. 기술 전략

### 2.1 JSON 처리
- C++ `Json`, `JsonUtilities` 모듈 사용.
- `FFileHelper`를 통해 프로젝트 폴더 내 파일 쓰기/읽기.

### 2.2 맵 에셋 검색
- `AssetRegistry` 모듈을 사용하여 프로젝트 내 모든 레벨 에셋을 동적으로 검색.

## 3. 작업 목록 (Tasks)
- [x] [Plan] 에디터 유틸리티 레벨 선택 툴 (JSON 저장)
- [ ] [Design] 에디터 유틸리티 레벨 선택 툴 (JSON 저장)
- [ ] [Do] Build.cs 모듈 추가 (Json, JsonUtilities, AssetRegistry, Blutility)
- [ ] [Do] JSON 입출력 C++ 헬퍼 클래스 구현
- [ ] [Do] Editor Utility Widget 제작 및 연동
- [ ] [Check] 즐겨찾기 저장 및 맵 이동 확인
