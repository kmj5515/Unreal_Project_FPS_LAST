# [Plan] 게임플레이 태그 유틸리티 (Gameplay Tag Utility)

## 요약 (Executive Summary)
프로젝트 내에서 게임플레이 태그(Gameplay Tags)를 더 효율적으로 관리하고 접근하기 위한 유틸리티를 개발합니다. 특히 C++와 블루프린트 양측에서 안전하게 태그에 접근할 수 있도록 헬퍼 함수를 제공하고, 앞서 발생했던 '태그 재정의' 에러와 같은 문제를 방지하기 위해 태그 관리 체계를 정립합니다.

## 컨텍스트 앵커 (Context Anchor)

| 차원 (Dimension) | 내용 (Content) |
|-----------|---------|
| WHY | 태그 사용 시 하드코딩 방지, 중앙 집중식 관리, C++/BP 간의 원활한 연동 및 중복 정의 방지. |
| WHO | 시스템 프로그래머, 게임 플레이 개발자, 콘텐츠 디자이너. |
| RISK | 1. 기존 태그 시스템과의 충돌. 2. 대규모 프로젝트 시 성능 오버헤드 (검색 로직). |
| SUCCESS | 중앙화된 싱글톤 또는 헬퍼를 통해 태그를 호출하고, 오타나 중복 정의 없이 안정적으로 작동함. |
| SCOPE | Native Gameplay Tags 정의, Blueprint Function Library (태그 체크/필터링), 에디터용 태그 브라우저 보조 기능. |

## 1. 요구 사항

### 1.1 기능적 요구 사항
- **중앙 집중식 태그 정의**: `NativeGameplayTags`를 사용하여 모든 C++ 태그를 한 곳에서 관리.
- **태그 존재 여부 확인**: 특정 액터나 컴포넌트가 특정 태그를 가지고 있는지 확인하는 편의 함수.
- **태그 기반 액터 검색**: 특정 태그를 가진 모든 액터를 효율적으로 찾는 기능.
- **블루프린트 지원**: C++에서 정의된 태그를 블루프린트에서도 쉽게 사용할 수 있도록 노출.

### 1.2 비기능적 요구 사항
- 성능을 위해 태그 캐싱 또는 `FGameplayTag` 직접 비교 방식 사용.
- 가독성 높은 네이밍 컨벤션 적용.

## 2. 기술 전략

### 2.1 Native Gameplay Tags
`FNativeGameplayTag`를 사용하여 싱글톤 클래스(`ULastFPSTags`)에서 모든 태그를 선언하고 초기화합니다. 이를 통해 컴파일 타임에 태그 유효성을 검사할 수 있습니다.

### 2.2 Blueprint Function Library
액터에서 `IAbilitySystemInterface`를 확인하고 `AbilitySystemComponent`를 통해 태그를 체크하는 공용 정적 함수를 제공합니다.

## 3. 작업 목록 (Tasks)
- [x] [Plan] 게임플레이 태그 유틸리티 개발
- [ ] [Design] 게임플레이 태그 유틸리티 개발
- [ ] [Do] LastFPSTags (Native Tags) 싱글톤 클래스 구현
- [ ] [Do] Blueprint Function Library 유틸리티 함수 구현
- [ ] [Do] 기존 하드코딩된 태그들을 유틸리티로 교체
- [ ] [Check] 태그 동작 및 중복 에러 여부 확인
