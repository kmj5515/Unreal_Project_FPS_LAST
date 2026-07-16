# 목적지 기반 비동기 콘텐츠 로딩 구조 개선안

## 1. 결론

현재 로딩 구조의 핵심 문제는 `LoadSynchronous()` 자체가 아니라, 어떤 시스템이 언제 어떤 콘텐츠를 메모리에 상주시킬지 결정하는 단일 소유자가 없다는 점이다.

현재 프로젝트에는 다음 로딩 방식이 혼재한다.

- `GameInstance` 시작 시 강제 동기 로드
- `GameInstanceSubsystem::Initialize()`에서 동기 로드
- 조회 함수가 처음 호출될 때 지연 동기 로드
- 위젯과 액터가 표시되는 순간 동기 로드
- 레벨 이동과 무관하게 독립적으로 실행되는 에셋 로드

이를 다음 원칙으로 변경한다.

> 게임플레이에 필요한 콘텐츠는 목적지 전환 단계에서 한 번에 비동기 로드하고, 기능 서브시스템과 UI는 이미 로드된 캐시만 조회한다.

모든 참조를 무조건 소프트 참조로 바꾸지는 않는다. 로딩 경계 바깥에서는 소프트 참조를 사용하고, 하나의 콘텐츠 묶음이 선택되어 로드된 뒤에는 강한 참조로 상주시킨다.

## 2. 현재 구조의 문제

| 영역 | 현재 동작 | 문제 |
|---|---|---|
| `GameInstance` 시작 | 로딩 팁 테이블과 이미지를 동기 로드하고 `WaitForStreaming()` 호출 | 게임 시작 단계에서 메인 스레드 정지 |
| 기능 서브시스템 초기화 | 스킬, 무기, 아이템, 퀘스트, 상태 효과 테이블 동기 로드 | 캐릭터 선택 전부터 Hub 콘텐츠 로드 |
| 캐릭터 로스터 | 처음 조회할 때 동기 로드 | 선택 화면 진입 순간 끊김 가능 |
| 캐릭터 정의 | 로스터가 모든 캐릭터 정의를 강하게 참조 | 모든 캐릭터의 Pawn, 메시, 애님, AbilitySet 연쇄 로드 |
| Hub 이동 | 다음 Tick에 바로 `OpenLevel()` 실행 | 선택 캐릭터의 런타임 에셋 준비를 기다리지 않음 |
| 로딩 프로세스 | 레벨, 컨트롤러, Pawn만 추적 | 실제 게임 데이터 로딩이 진행률과 완료 조건에 없음 |
| UI | 화면 클래스와 아이콘을 표시 순간 동기 로드 | 메뉴나 상태 효과가 처음 표시될 때 끊김 |

현재 소스에는 `LoadSynchronous()`와 `WaitForStreaming()` 호출이 총 32곳 존재한다.

### 2.1 캐릭터 로스터의 강한 참조 사슬

`ULastFPSCharacterRoster`는 모든 캐릭터 정의를 `TObjectPtr`로 보관한다.

```cpp
TArray<TObjectPtr<ULastFPSCharacterDefinition>> Characters;
```

각 `ULastFPSCharacterDefinition`은 다시 다음 데이터를 강하게 참조한다.

- `PawnClass`
- `StatData`
- `VisualData`
- `AcceleratorData`
- `AbilitySet`

`VisualData`와 `AbilitySet`은 다시 다음 콘텐츠를 강하게 참조한다.

- Skeletal Mesh
- Anim Instance Class
- Portrait와 Icon
- Gameplay Ability Class
- Gameplay Effect Class

따라서 선택 화면에서 이름과 초상화만 필요해도 로스터를 로드하는 순간 모든 캐릭터의 런타임 콘텐츠가 연쇄 로드될 수 있다.

### 2.2 Hub 로딩 프로세스가 콘텐츠를 기다리지 않음

현재 `ULastFPSLoadingProcessSubsystem`은 다음 준비 상태만 추적한다.

- Level
- Local PlayerController
- Local Player Pawn

선택 캐릭터 데이터, 공용 Data Table, HUD 클래스, NPC 데이터 등은 완료 조건에 포함되지 않는다.

또한 `ULastFPSGameInstance::ExecuteServerTravel()`은 로딩 화면을 시작한 뒤 다음 Tick에 바로 `OpenLevel()` 또는 `ServerTravel()`을 실행한다.

그러나 `ALastFPSGameModeBase::GetDefaultPawnClassForController_Implementation()`은 Pawn 생성 시점에 선택 캐릭터 정의와 `PawnClass`를 즉시 필요로 한다. 따라서 선택 캐릭터는 레벨 이동 전에 준비되어야 한다.

## 3. 목표 로딩 흐름

```text
게임 시작
  → 최소 Bootstrap 콘텐츠만 상주
  → 캐릭터 선택 화면 진입
  → 선택 화면 표시 데이터 비동기 로드
  → 캐릭터 선택
  → 선택 캐릭터 런타임 데이터 선행 비동기 로드
  → 확인 버튼
  → Hub 필수 데이터와 선택 캐릭터 데이터 준비
  → 모든 필수 데이터 Ready
  → OpenLevel(Hub)
  → Level + PlayerController + Pawn 준비
  → 로딩 화면 종료
```

`OpenLevel()`은 반드시 선택 캐릭터의 `PawnClass`를 포함한 필수 콘텐츠가 로드된 이후에 실행한다.

## 4. 로딩 단계 정의

콘텐츠를 다음 단계로 나눈다.

### 4.1 Bootstrap

게임 실행 직후부터 필요한 최소 콘텐츠다.

- Loading Screen Widget Class
- 로딩 화면 기본 배경과 폰트
- UI 루트 레이아웃
- 캐릭터 선택 화면 진입에 필요한 최소 Registry
- 작은 기본 로딩 팁 세트

Bootstrap 콘텐츠는 크기를 작게 유지하고 항상 상주시킨다.

### 4.2 Character Select

캐릭터 선택 화면에서만 필요한 콘텐츠다.

- 캐릭터 이름
- 역할
- 설명
- 선택 카드 초상화
- 선택 화면 Widget Class

전투용 Pawn, 메시, AbilitySet은 포함하지 않는다.

### 4.3 Hub Required

Hub 레벨을 열기 전에 반드시 준비되어야 하는 콘텐츠다.

- 선택된 캐릭터의 Runtime Definition
- 선택된 캐릭터의 `PawnClass`
- 선택된 캐릭터의 VisualData, StatData, AbilitySet
- 공용 스킬과 무기 밸런스 테이블
- 아이템, 모듈, 퀘스트 테이블
- 상태 효과 UI 데이터
- HUD Widget Class와 필수 HUD 리소스
- Hub NPC 기본 데이터

### 4.4 Hub Background

Hub 진입을 막을 필요는 없지만 Hub에서 곧 사용할 가능성이 높은 콘텐츠다.

- 인벤토리 아이콘
- 상점 아이콘
- 퀘스트 아이콘
- 무기 3D 미리보기
- 자주 사용하는 메뉴 Widget Class

Hub Required 완료 이후 로딩 화면이 유지되는 동안 계속 로드하거나, Hub 진입 직후 백그라운드에서 마무리한다.

### 4.5 Dungeon Required

던전 진입 시 목적지와 선택된 난이도에 맞춰 로드한다.

- 던전 레벨 전용 적 정의
- 적 메시와 애니메이션
- 던전 전용 무기와 스킬
- 전용 Niagara, Sound, Material
- 보상 테이블
- 던전 UI

## 5. 단일 콘텐츠 로딩 조정자

새로운 `ULastFPSContentLoadSubsystem`을 로딩의 단일 소유자로 둔다.

이 서브시스템은 다음 책임만 갖는다.

- 목적지에 필요한 에셋 경로 수집
- `FStreamableHandle` 생성과 수명 관리
- 중복 요청 병합
- 로딩 상태 관리
- 진행률 전달
- 로드된 콘텐츠 상주 보장
- 실패 결과 전달
- 목적지 이탈 시 더 이상 필요 없는 Handle 해제

예상 공개 API는 다음과 같다.

```cpp
enum class ELastFPSContentLoadState : uint8
{
    Unloaded,
    Loading,
    Ready,
    Failed
};

void PrepareDestinationAsync(
    ELastFPSTravelDestination Destination,
    FName SelectedCharacterId);

bool IsDestinationReady(ELastFPSTravelDestination Destination) const;
ELastFPSContentLoadState GetLoadState() const;
```

`FStreamableHandle`은 로컬 변수로만 보관하지 않는다. 콘텐츠를 상주시켜야 하는 기간 동안 `ULastFPSContentLoadSubsystem`이 Handle을 소유해야 한다.

## 6. 목적지 Load Set

목적지별 콘텐츠 목록은 코드에 하드코딩하지 않고 Data Asset으로 관리한다.

예상 에셋은 다음과 같다.

```text
DA_LoadSet_Bootstrap
DA_LoadSet_CharacterSelect
DA_LoadSet_Hub
DA_LoadSet_Dungeon_Common
DA_LoadSet_Dungeon_{DungeonId}
```

예상 데이터 구조는 다음과 같다.

```cpp
UCLASS(BlueprintType)
class ULastFPSContentLoadSet : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly)
    TArray<TSoftObjectPtr<UObject>> RequiredAssets;

    UPROPERTY(EditDefaultsOnly)
    TArray<TSoftObjectPtr<UObject>> BackgroundAssets;
};
```

선택 캐릭터처럼 런타임에 결정되는 콘텐츠는 Load Set의 고정 목록과 합쳐서 하나의 비동기 요청으로 처리한다.

초기 구현에서는 `FStreamableManager::RequestAsyncLoad()`와 `FSoftObjectPath` 목록만으로 충분하다. 콘텐츠 규모가 커지고 DLC나 패치 단위 관리가 필요해지면 `UAssetManager`의 Primary Asset Bundle로 확장한다.

## 7. 캐릭터 선택 데이터 분리

현재 로스터의 전체 캐릭터 정의 강한 참조를 선택 화면 전용 행으로 교체한다.

```cpp
USTRUCT(BlueprintType)
struct FLastFPSCharacterRosterEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName CharacterId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText Role;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UTexture2D> Portrait;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<ULastFPSCharacterDefinition> RuntimeDefinition;
};
```

선택 화면에서는 표시 정보와 Portrait만 사용한다.

캐릭터가 선택되면 해당 행의 `RuntimeDefinition` 하나만 비동기 로드한다. 선택된 Runtime Definition 내부의 `PawnClass`, `VisualData`, `StatData`, `AbilitySet`은 강한 참조를 유지해도 된다. 이 데이터는 선택된 캐릭터의 런타임 묶음이며 Hub에 머무는 동안 함께 상주해야 하기 때문이다.

배열 인덱스는 콘텐츠 추가와 정렬 변경에 취약하므로 장기적으로는 `SelectedCharacterIndex`보다 안정적인 `CharacterId` 또는 `FPrimaryAssetId`를 저장하고 복제하는 것이 적합하다.

## 8. Travel을 Prepare와 Commit으로 분리

현재 이동 API를 다음 두 단계로 분리한다.

### 8.1 Prepare Travel

```text
RequestTravelToHub
  → BeginTravelLoading(Hub)
  → Loading.Process.Assets 등록
  → PrepareDestinationAsync(Hub, SelectedCharacterId)
  → 필수 에셋 로드와 서브시스템 캐시 구성
```

### 8.2 Commit Travel

```text
필수 에셋 Ready
  → Loading.Process.Assets 완료
  → CommitTravel
  → OpenLevel 또는 ServerTravel
```

최종 로딩 화면 종료 조건은 다음과 같다.

```text
Assets Ready
+ Level Loaded
+ Local PlayerController Ready
+ Local Player Pawn Ready
```

기존 `ULastFPSLoadingProcessSubsystem`의 외부 프로세스 등록 구조는 재사용한다. `Loading.Process.Assets`를 추가하고 `FStreamableHandle::GetProgress()` 값을 기존 진행률 시스템에 전달한다.

## 9. 기능 서브시스템 변경

기능 서브시스템은 더 이상 에셋 로딩을 소유하지 않는다.

### 현재 방식

```cpp
const UDataTable* ULastFPSEconomySubsystem::GetItemTable() const
{
    return ItemTable.LoadSynchronous();
}
```

### 목표 방식

```cpp
const UDataTable* ULastFPSEconomySubsystem::GetItemTable() const
{
    return LoadedItemTable;
}
```

수명 주기는 다음과 같이 정리한다.

- `Initialize()`: 런타임 상태와 델리게이트만 초기화
- `OnDestinationDataReady()`: 이미 로드된 테이블 확인, 캐시 저장, 인덱스 생성
- 조회 함수: 캐시만 읽음
- `Deinitialize()` 또는 목적지 이탈: 캐시와 델리게이트 정리

조회 함수 내부에서 자동으로 로드하지 않는다. 데이터가 준비되지 않았다면 `nullptr`, `false` 또는 명시적인 실패 결과를 반환하고 반복 경고가 아닌 한 번의 진단 로그를 남긴다.

대상 서브시스템은 다음과 같다.

- `ULastFPSEconomySubsystem`
- `ULastFPSLoadoutSubsystem`
- `ULastFPSQuestSubsystem`
- `ULastFPSSkillDataSubsystem`
- `ULastFPSWeaponDataSubsystem`
- `ULastFPSStatusEffectDataSubsystem`
- `ULastFPSUIManagerSubsystem`

## 10. UI 로딩 정책

Widget은 콘텐츠 로딩을 직접 수행하지 않는다.

### 필수 UI

HUD, 상태 효과 아이콘, 기본 메뉴처럼 자주 사용하는 UI는 Hub Required 또는 Hub Background에서 미리 로드한다.

### 선택 UI

자주 사용하지 않는 화면은 다음 흐름으로 연다.

```text
OpenScreen 요청
  → Widget Class가 이미 로드됨: 즉시 표시
  → 아직 로드되지 않음: 비동기 로드
  → 요청자가 아직 유효하고 화면 요청이 취소되지 않음
  → Widget 생성 및 표시
```

아이콘 위젯과 화면 위젯 내부에서는 `LoadSynchronous()`를 호출하지 않는다.

비동기 아이콘 로드에는 다음 조건이 필요하다.

- 요청 중 위젯 파괴 대응
- 슬롯 재사용 시 이전 요청 결과 무시
- 동일 Texture 요청 공유
- 로드 전 Fallback Icon 표시
- 완료 후 Brush 갱신

## 11. 로딩 화면 Bootstrap 정책

로딩 화면은 다른 콘텐츠를 비동기 로드하기 전에 먼저 표시되어야 하므로 예외적인 Bootstrap 콘텐츠다.

다음 항목만 작게 유지하여 항상 상주시킨다.

- Loading Screen Widget Class
- 기본 배경
- Progress Bar 리소스
- 기본 폰트
- 작은 기본 팁 세트

`ULastFPSGameInstance::Init()`에서 모든 팁 이미지를 동기 로드하거나 `WaitForStreaming()`을 호출하지 않는다.

추가 팁 이미지는 다음 이동이 시작되기 전에 비동기로 준비하거나, 이미 로드된 이미지 중 하나만 표시한다. 로딩 화면 위젯이 생성된 뒤 그 위젯 안에서 다시 `LoadSynchronous()`를 호출하지 않는다.

## 12. 런타임 데이터와 불변 설정 분리

Load Set과 Data Table은 불변 설정만 보관한다.

- 에셋 경로
- 이름과 설명
- 밸런스 값
- UI 표시 옵션
- 콘텐츠 분류

다음 런타임 상태를 공유 Data Asset에 저장하지 않는다.

- 현재 선택 캐릭터
- 로딩 진행률
- 현재 스택
- 현재 퀘스트 진행도
- 장착 상태
- 활성 버프 상태

런타임 상태는 각 기능 서브시스템, PlayerState, AbilitySystemComponent가 소유한다.

## 13. 검증 정책

참조 검증을 위해 런타임 초기화에서 테이블을 동기 로드하지 않는다.

검증은 다음 위치로 이동한다.

- Data Asset의 `IsDataValid()`
- 에디터 전용 검증 명령
- Cook 전 검증
- 비동기 콘텐츠 로드 완료 후 개발 빌드 검증

검증 실패는 다음 정보를 포함한다.

- 문제의 Load Set 또는 Data Table
- 누락된 Row Name이나 Asset Path
- 해당 콘텐츠가 필수인지 선택인지
- 목적지 이름

## 14. 네트워크 고려사항

서버는 선택 캐릭터의 런타임 정의와 `PawnClass`를 Pawn 생성 전에 반드시 로드해야 한다.

멀티플레이에서는 서버만 준비되었다고 이동을 완료하면 클라이언트에서 첫 복제 시점에 에셋 로드 끊김이 발생할 수 있다. 이후 네트워크 확장 단계에서는 다음 흐름이 필요하다.

```text
서버가 Destination Load 요청
  → 각 클라이언트에 선택 콘텐츠 ID 전달
  → 서버와 클라이언트가 비동기 로드
  → 클라이언트 Ready 응답
  → 필수 참여자 Ready 확인
  → ServerTravel
```

초기 Standalone 구현에서는 로컬 `PrepareDestinationAsync()` 완료 후 `OpenLevel()`을 실행하는 것으로 시작할 수 있다.

## 15. 권장 적용 순서

### 1단계: 로딩 조정자와 Travel Gate

- `ULastFPSContentLoadSubsystem` 추가
- 목적지 Load Set 구조 추가
- `RequestTravel`을 Prepare와 Commit으로 분리
- `Loading.Process.Assets` 추가
- 필수 에셋 완료 전 `OpenLevel()` 금지

### 2단계: 캐릭터 로스터 분리

- 선택 화면용 `FLastFPSCharacterRosterEntry` 추가
- 전체 Character Definition 강한 참조 제거
- Portrait와 Runtime Definition을 소프트 참조로 변경
- 선택된 Runtime Definition 하나만 선행 로드
- 가능하면 선택 상태를 Index에서 CharacterId로 변경

### 3단계: 기능 서브시스템 캐시화

- 스킬, 무기, 아이템, 모듈, 퀘스트, 상태 효과 테이블을 Hub Load Set에 포함
- 각 서브시스템의 `LoadSynchronous()` 제거
- 비동기 로드 완료 후 캐시와 인덱스 구성
- 조회 함수에서 캐시만 사용

### 4단계: UI 동기 로드 제거

- HUD 필수 아이콘 사전 로드
- 인벤토리와 상점 아이콘 백그라운드 로드
- `OpenScreen()` 비동기화 또는 Widget Class 사전 로드
- 상태 효과와 스킬 아이콘의 표시 순간 동기 로드 제거

### 5단계: 런타임 검증 이동

- 런타임 초기화의 참조 검증 제거
- `IsDataValid()` 또는 에디터 검증 도구로 이동
- Cook 전에 누락 참조 검사

### 6단계: Dungeon Load Set 확장

- 공용 던전 콘텐츠와 던전별 콘텐츠 분리
- 선택 던전과 난이도를 Load Context에 포함
- 던전 이탈 시 전용 Handle 해제

## 16. 완료 기준

다음 조건을 모두 만족하면 1차 로딩 구조 개선이 완료된 것으로 본다.

- 기능 서브시스템 조회 함수에서 `LoadSynchronous()`를 호출하지 않음
- 일반 UI 위젯에서 `LoadSynchronous()`를 호출하지 않음
- 캐릭터 선택 화면 진입 시 모든 캐릭터의 런타임 콘텐츠를 로드하지 않음
- 선택된 캐릭터의 `PawnClass`가 준비되기 전에 Hub를 열지 않음
- 로딩 화면 진행률에 실제 콘텐츠 비동기 로딩이 포함됨
- Hub 진입 시 필수 테이블과 HUD 콘텐츠가 이미 준비됨
- 목적지 이탈 시 전용 콘텐츠 Handle이 해제됨
- 실패 시 기본 Pawn 생성이나 빈 UI로 조용히 진행하지 않고 이동 취소 또는 복구 경로를 실행함

## 17. 우선 수정 대상 파일

- `LastFPS/Source/LastFPS/Game/LastFPSGameInstance.cpp`
- `LastFPS/Source/LastFPS/Game/Loading/LastFPSLoadingProcessSubsystem.cpp`
- `LastFPS/Source/LastFPS/Data/Definitions/LastFPSCharacterRoster.h`
- `LastFPS/Source/LastFPS/Data/Definitions/LastFPSCharacterDefinition.h`
- `LastFPS/Source/LastFPS/Game/LastFPSGameModeBase.cpp`
- `LastFPS/Source/LastFPS/Economy/LastFPSEconomySubsystem.cpp`
- `LastFPS/Source/LastFPS/Inventory/LastFPSLoadoutSubsystem.cpp`
- `LastFPS/Source/LastFPS/Quest/LastFPSQuestSubsystem.cpp`
- `LastFPS/Source/LastFPS/Skills/LastFPSSkillDataSubsystem.cpp`
- `LastFPS/Source/LastFPS/Weapons/LastFPSWeaponDataSubsystem.cpp`
- `LastFPS/Source/LastFPS/AbilitySystem/Status/LastFPSStatusEffectDataSubsystem.cpp`
- `LastFPS/Source/LastFPS/UI/Framework/LastFPSUIManagerSubsystem.cpp`

가장 먼저 구현할 범위는 `ContentLoadSubsystem`, Travel Gate, Character Roster 분리다. 이 기반을 만든 뒤 기능 서브시스템과 UI를 순서대로 옮겨야 중간 단계에서 로딩 책임이 더 복잡해지는 것을 방지할 수 있다.
