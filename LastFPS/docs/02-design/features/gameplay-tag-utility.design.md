# [Design] 게임플레이 태그 유틸리티 (Gameplay Tag Utility)

## 컨텍스트 앵커 (Context Anchor)

| 차원 (Dimension) | 내용 (Content) |
|-----------|---------|
| WHY | 태그 사용 시 하드코딩 방지, 중앙 집중식 관리, C++/BP 간의 원활한 연동 및 중복 정의 방지. |
| WHO | 시스템 프로그래머, 게임 플레이 개발자, 콘텐츠 디자이너. |
| RISK | 1. 기존 태그 시스템과의 충돌. 2. 대규모 프로젝트 시 성능 오버헤드 (검색 로직). |
| SUCCESS | 중앙화된 싱글톤 또는 헬퍼를 통해 태그를 호출하고, 오타나 중복 정의 없이 안정적으로 작동함. |
| SCOPE | Native Gameplay Tags 정의, Blueprint Function Library (태그 체크/필터링), Utility 폴더 구조화. |

## 1. 개요 (Overview)
이 설계는 `Source/LastFPS/Utility/` 폴더 내에 게임플레이 태그 관리 체계를 구축하는 것을 목표로 합니다. `NativeGameplayTags`를 통해 컴파일 타임 안정성을 확보하고, 블루프린트에서 쉽게 접근할 수 있는 유틸리티 함수를 제공합니다.

## 2. 아키텍처 옵션 (Architecture Options)

### 옵션 A: 정적 변수 방식 (Minimal)
- **설명**: `NativeGameplayTags.h`에 전역 정적 변수로 모든 태그를 정의합니다.
- **장점**: 구현이 가장 빠르고 사용이 직관적입니다.
- **단점**: 프로젝트가 커지면 전역 네임스페이스가 오염될 수 있고, 태그 그룹화가 어렵습니다.

### 옵션 B: 완전 분리 방식 (Clean Architecture)
- **설명**: 각 도메인(캐릭터, 무기, UI)별로 별도의 태그 매니저 클래스를 생성합니다.
- **장점**: 의존성이 최소화되고 도메인별 관리가 명확합니다.
- **단점**: 태그 하나를 찾기 위해 여러 파일을 뒤져야 할 수 있으며, 구조가 과하게 복잡해집니다.

### 옵션 C: 싱글톤 & 라이브러리 방식 (Pragmatic Balance - 추천)
- **설명**: `FLastFPSTags` 싱글톤 클래스에서 모든 태그를 초기화하고, `ULastFPSTagFunctionLibrary`에서 이를 블루프린트에 노출합니다.
- **장점**: 한 곳에서 모든 태그를 관리하면서도 블루프린트 사용성이 극대화됩니다. 중복 정의 에러를 원천 차단합니다.
- **단점**: 싱글톤 클래스 파일이 커질 수 있습니다 (카테고리별 주석으로 해결 가능).

## 3. 상세 설계 (옵션 C 기준)

### 3.1 폴더 구조
- `Source/LastFPS/Utility/LastFPSTags.h / .cpp`: Native Tag 정의 및 싱글톤.
- `Source/LastFPS/Utility/LastFPSTagFunctionLibrary.h / .cpp`: BP용 정적 함수.

### 3.2 주요 클래스 구조
```cpp
// 1. Native Tag 매니저
struct FLastFPSTags
{
public:
    static const FLastFPSTags& Get() { return Tags; }
    static void InitializeNativeTags();

    // 캐릭터 관련
    FGameplayTag Character_State_Dead;
    FGameplayTag Character_State_Crouched;

    // 쿨다운 관련 (기존 에러 발생 지점)
    FGameplayTag Cooldown_Skill1;
    FGameplayTag Cooldown_Skill2;

private:
    static FLastFPSTags Tags;
};

// 2. 블루프린트 함수 라이브러리
UCLASS()
class ULastFPSTagFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="LastFPS|Tags")
    static FGameplayTag GetTag_CooldownSkill1();

    UFUNCTION(BlueprintCallable, Category="LastFPS|Tags")
    static bool ActorHasMatchingGameplayTag(AActor* TargetActor, FGameplayTag TagToCheck);
};
```

## 4. 작업 목록 (Tasks)
- [x] [Plan] 게임플레이 태그 유틸리티 개발
- [x] [Design] 게임플레이 태그 유틸리티 개발
- [ ] [Do] Utility 폴더 생성 및 LastFPSTags 구현
- [ ] [Do] LastFPSTagFunctionLibrary 구현
- [ ] [Do] 기존 클래스(HUDWidget, GE 등)의 태그 참조를 유틸리티로 교체
- [ ] [Check] 컴파일 에러 및 기능 작동 확인
