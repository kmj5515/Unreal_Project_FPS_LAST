# [Plan] 인카운터 ↔ 목표(디펜스/점령) 연동

## 요약 (Executive Summary)

현재 룸 인카운터(웨이브 스폰)와 디펜스/점령 목표는 각자 독립적으로 동작한다. 인카운터는 "웨이브 소진 + 전멸"만을 완료 조건으로 알고, 디펜스/점령 컴포넌트는 `bAutoStart`로 BeginPlay에 스스로 시작한다. 둘 사이에 수명·판정·실패 경로가 전혀 연결돼 있지 않다.

이 계획은 인카운터 런타임이 **목표 계약(인터페이스)만 알고** 목표의 수명과 완료/실패를 통제하도록 연결부를 만든다. 인카운터 런타임에는 "디펜스"나 "점령"이라는 단어가 들어가지 않으며, 신규 목표 유형은 인터페이스 구현 추가만으로 확장된다.

## 컨텍스트 앵커 (Context Anchor)

| 차원 | 내용 |
|---|---|
| WHY | 디펜스/점령이 웨이브 전투와 한 몸으로 동작해야 콘텐츠로 성립한다. 현재는 두 시스템이 서로를 모른다. |
| WHO | 게임플레이 프로그래머(연결부·AI), 레벨 디자이너(마커 배치), 콘텐츠 디자이너(웨이브·밸런스 데이터) |
| RISK | 1. 기존 섬멸형 인카운터 회귀. 2. 웨이브 순환으로 퀘스트 진행 표시(처치 수 기준)가 깨짐. 3. 적 AI가 Pawn이 아닌 장치를 타깃으로 잡는 부분이 범용 AI 코드를 오염시킬 위험. 4. 화면 마커 구조 개정(`bIsRoutePoint` → 앵커 enum)이 기존 위치 목표 마커·동선 지점을 깨뜨릴 위험. |
| SUCCESS | 디펜스 방 하나가 처음부터 끝까지 플레이 가능: 진입 → 배리어 → 웨이브 순환 → 적이 장치 공격 → 버티면 성공(퀘스트 진행) / 장치 파괴되면 미션 실패 → 허브 귀환. HUD에 장치 체력·버티기 진행 표시. **방어 대상에 화면 마커가 인카운터 클리어까지 상시 부착.** |
| SCOPE | 목표 인터페이스, 레벨 마커 계약 확장, 완료·실패 판정, 장치 GAS 전환, AI 타깃 등록소, 실패→귀환 경로, 디펜스 HUD, 방어 대상 화면 마커. 점령전은 같은 배관을 타지만 이번 범위에서는 배선까지만. |

## 확정된 방향 (의사결정)

| 항목 | 결정 |
|---|---|
| 장치 피격 | **GAS ASC 부여** — 장치에 `AbilitySystemComponent` + `ULastFPSAttributeSet`을 붙여 기존 적 어빌리티/데미지 파이프라인을 그대로 재사용 |
| 실패 처리 | **미션 실패 → 허브 귀환** — `GameInstance::RequestTravelToHub()` 기존 경로 사용 |
| 웨이브 규칙 | **목표 해결까지 웨이브 순환** — 웨이브 소진 시 인덱스를 되감아 반복. 완료 조건은 논리곱이라 별도 모드 enum 불필요 |
| 범위 | **디펜스 한 방 끝까지** |

---

## 1. 현재 구조와 결손 지점

### 1.1 이미 있는 것

| 영역 | 클래스 | 상태 |
|---|---|---|
| 웨이브 스폰 | `ALastFPSRoomEncounterRuntime` | 서버 권한, 배리어, 비동기 프리로드, 진행 복제까지 완성 |
| 인카운터 해석 | `ULastFPSRoomEncounterSubsystem` | 레벨 마커 태그 + `DT_RoomEncounter` 행으로 런타임 생성 |
| 점령 | `ULastFPSCaptureZoneComponent` / `ALastFPSCaptureZoneActor` | 판정·복제·퀘스트 통지 완성 |
| 방어 | `ULastFPSDefendObjectiveComponent` / `ALastFPSDefendableDeviceActor` | 승패 판정 완성, 데미지 유입만 없음 |
| 퀘스트 | `ELastFPSObjectiveType::CaptureZone / DefendZone` + `FTagEventTracker` | 코드 수정 없이 데이터만으로 동작 |

### 1.2 결손 지점 (이 계획이 메우는 것)

| # | 결손 | 근거 |
|---|---|---|
| D1 | **수명 미연결** — 목표가 `bAutoStart`로 BeginPlay에 시작, 인카운터와 무관 | `LastFPSDefendObjectiveComponent.cpp:31` |
| D2 | **완료 조건 하드코딩** — `EvaluateWaveCompletion()`이 "웨이브 소진 + 전멸"만 안다 | `LastFPSRoomEncounterRuntime.cpp:818` |
| D3 | **게임플레이 실패 경로 부재** — `FailEncounterOpen()`은 구성 오류 전용 | `LastFPSRoomEncounterRuntime.cpp:917` |
| D4 | **장치 피격 불가** — `ApplyIntegrityDamage()` 호출부가 프로젝트 전체에 0건. 적 공격은 GAS 어빌리티 → GE 경로이고 장치는 ASC가 없다 | `BTTask_EnemyAttack.cpp:57` |
| D5 | **AI가 장치를 타깃으로 잡지 못함** — 타깃은 Sight 퍼셉션으로 잡힌 Pawn뿐 | `LastFPSEnemyAIController.h:16` |
| D6 | **HUD 부재** — `UI/HUD/Presenters/`에 목표 게이지 프레젠터 없음 | 디렉터리 확인 |
| D7 | **레벨 계약 미확장** — 프로파일에 목표 마커 태그가 없어 목표 액터를 인카운터에 귀속시킬 방법이 없음 | `LastFPSRoomEncounterProfile.h` |
| D8 | **화면 마커가 퀘스트 위치 목표 전용** — `GetActiveWaypoints()`가 `ReachLocation`만 수집하고, 앵커가 바닥 투영 2종뿐이며, 게이지 표시 항목이 없다 | `LastFPSQuestSubsystem.cpp` / `LastFPSObjectiveMarkerWidget.h` |

---

## 2. 기술 전략

### 2.1 목표 계약 — `ILastFPSEncounterObjective` (D1, D2 해결)

신규 `Source/LastFPS/Encounter/LastFPSEncounterObjectiveInterface.h`

```cpp
UENUM(BlueprintType)
enum class ELastFPSEncounterObjectiveResult : uint8
{
    Succeeded,
    Failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnLastFPSEncounterObjectiveResolved,
    UObject*, Objective,
    ELastFPSEncounterObjectiveResult, Result);

UINTERFACE(MinimalAPI)
class ULastFPSEncounterObjective : public UInterface { GENERATED_BODY() };

class ILastFPSEncounterObjective
{
    GENERATED_BODY()
public:
    /** 인카운터 시작 시 서버가 호출한다. 진행 초기화 후 판정을 시작한다. */
    virtual void StartEncounterObjective() = 0;

    /** 인카운터 종료(완료/실패/언로드) 시 호출한다. 판정을 중단하며 결과를 통지하지 않는다. */
    virtual void StopEncounterObjective() = 0;

    /** 성공으로 해결됐는가. 미해결이면 인카운터는 웨이브를 순환하며 완료를 보류한다. */
    virtual bool IsObjectiveSucceeded() const = 0;

    /** 0~1 진행률 — HUD 및 인카운터 진행 이벤트가 공유하는 단일 계약. */
    virtual float GetObjectiveProgress01() const = 0;

    /** 성공/실패 1회 통지. 런타임이 구독한다. */
    virtual FOnLastFPSEncounterObjectiveResolved& GetOnObjectiveResolved() = 0;
};
```

`ULastFPSDefendObjectiveComponent`와 `ULastFPSCaptureZoneComponent`가 이 인터페이스를 구현한다. 기존 `StartDefense()`/`StopDefense()`는 인터페이스 구현으로 흡수하고 **`bAutoStart`는 제거**한다 — 목표의 수명 소유권을 인카운터로 일원화하기 위함이다.

> **설계 이유**: 인카운터 런타임이 `ULastFPSDefendObjectiveComponent`를 직접 알면 "호위/해킹/생존" 목표를 추가할 때마다 런타임을 수정해야 한다. 인터페이스로 두면 신규 목표는 구현 추가만으로 붙는다(OCP). 런타임 코드에 목표 유형을 열거하는 switch/cast가 생기지 않는지가 이 설계의 검증 기준이다.

#### 2.1.1 점령과 방어는 진행 계산이 동일하다 — 중복 추출

두 컴포넌트의 진행 로직은 사실상 같은 코드다.

| | 점령 | 방어 |
|---|---|---|
| 진행 공식 | `ElapsedSeconds / CaptureDuration` | `ElapsedSeconds / HoldDuration` |
| 시간이 흐르는 조건 | **플레이어가 볼륨 안일 때만** | 시작 후 항상 |
| 볼륨 밖으로 나가면 | 타이머 정지 (진행 유지, 감소 없음) | 해당 없음 |
| 실패 조건 | **없음** (성공만 존재) | 장치 파괴 |
| 기본값 | 8초 | 60초 |

공통으로 중복된 것: `ElapsedSeconds`(ReplicatedUsing) · `UpdateInterval` · `GetProgress01()` · `BroadcastProgress()` · `OnRep_Elapsed()` · 타이머 핸들.

**차이는 딱 두 가지다** — ① 타이머를 돌릴지 판단하는 조건, ② 실패 개념의 유무. 그 외는 전부 같으므로, 시간 누적·복제·진행률 브로드캐스트를 공통 기반(예: `ULastFPSTimedObjectiveComponentBase`)으로 올리고 파생 클래스는 "지금 시간이 흘러야 하는가"만 답하게 한다. CLAUDE.md의 *"기능 전용 클래스를 만들기 전에 공통으로 재사용할 수 있는 책임인지 먼저 확인한다"* 에 해당하는 지점이다.

> 인터페이스는 이 비대칭을 이미 수용한다. 점령은 `OnObjectiveResolved`로 `Succeeded`만 내보내고 `Failed`를 쓰지 않는다. 런타임은 실패를 받으면 처리할 뿐 "점령에는 실패가 없다"를 알 필요가 없다.

> **미구현 확인**: 점령의 적 경합(구역 안에 적이 있으면 진행 정지)은 현재 없다. 헤더에 *"추후 확장 지점"* 으로 명시돼 있으며, 점령전을 실제 콘텐츠로 쓸 때 필요해질 항목이다.

### 2.2 레벨 마커 계약 확장 (D7 해결)

`ULastFPSRoomEncounterProfile`에 추가:

```cpp
UPROPERTY(EditDefaultsOnly, Category="Level Contract")
FName ObjectiveMarkerTag = TEXT("RoomEncounter.Objective");
```

`ULastFPSRoomEncounterSubsystem::InitializeRuntimeEncounters`가 배리어·스폰포인트를 모으는 방식과 동일하게, `ObjectiveMarkerTag`와 `EncounterId` 두 액터 태그를 모두 가진 액터를 수집해 `InitializeEncounter`로 넘긴다. 런타임은 그 액터들에서 `ILastFPSEncounterObjective` 구현 컴포넌트를 뽑아 보관한다.

**성능 주의**: 목표 액터는 임의 클래스라 `GetAllActorsOfClass(AActor::StaticClass())` 전수 스캔은 비싸다. `UGameplayStatics::GetAllActorsWithTag(World, Profile.ObjectiveMarkerTag, Out)` 로 태그 스캔 1회만 수행한다.

**식별자 역할 분리** (이원화가 아님):

| 식별자 | 타입 | 의미 | 소비자 |
|---|---|---|---|
| 액터 태그 = `EncounterId` | `FName` | "어느 방 소속인가" | 인카운터 서브시스템 |
| `ZoneTag` | `FGameplayTag` | "어느 퀘스트 목표인가" | 퀘스트 서브시스템 |

기존 `ZoneTag` → `NotifyObjectiveDefended` 경로는 그대로 유지한다.

### 2.3 완료 조건 — 논리곱, 모드 enum 없음 (D2 해결)

`EvaluateWaveCompletion()` 개정:

```
웨이브 스폰 중이거나 생존 적이 있으면 → 대기
남은 웨이브가 있으면            → 다음 웨이브
모든 목표가 성공했으면          → CompleteEncounter()
그 외(목표 미해결)              → CurrentWave = 0 으로 되감고 순환, LoopCount 증가
```

- **목표가 없는 인카운터**는 `AreAllObjectivesSucceeded()`가 `true`를 반환 → 기존 섬멸형 동작 그대로. 회귀 없음.
- 목표가 **성공 델리게이트로 먼저 해결**될 수도 있으므로(버티기 타이머가 적 전멸보다 먼저 끝나는 경우) `HandleObjectiveResolved(Succeeded)`에서도 `EvaluateWaveCompletion()`을 다시 태운다.
- **성공 시 잔여 적 처리**: 배리어가 열린 뒤에도 적이 따라오면 어색하므로, 목표 성공으로 완료할 때는 `AliveEnemies`를 정리한다. (연출은 후속 — 우선 즉시 제거)
- `LoopCount`는 복제해 HUD에 노출하고, 추후 순환 난이도 스케일링의 훅으로 남긴다.

### 2.4 장치를 GAS 대상으로 전환 (D4 해결)

`ALastFPSDefendableDeviceActor` 개정:

- `IAbilitySystemInterface` 구현 + `UAbilitySystemComponent` 부착
- `ULastFPSAttributeSet`의 `Health` / `MaxHealth` 사용
- **제거**: `Integrity`, `MaxIntegrity`, `ApplyIntegrityDamage()`, `ResetIntegrity()`, `OnRep_Integrity`
- **대체**: 파괴 감지는 `GetGameplayAttributeValueChangeDelegate(Health)` 구독 → 0 도달 시 `OnDeviceDestroyed` 1회
- 초기값은 하드코딩된 `MaxIntegrity = 1000.f` 대신 **신규 `ULastFPSDefendableDeviceDefinition` (Data Asset)** 이 소유하는 초기화 GameplayEffect로 적용 — `ULastFPSCharacterDefinition` 패턴과 동일. 밸런스 값을 레벨 배치 인스턴스에 박아두지 않는다.
- 리셋은 초기화 GE 재적용

**이 선택의 이득**: `BTTask_ChaseTarget`(위치 기반)과 `BTTask_EnemyAttack`(ASC 어빌리티 발동)은 타깃이 Pawn인지 묻지 않는다. 장치에 ASC만 있으면 **추격·공격·데미지 경로를 한 줄도 안 고치고** 재사용할 수 있다.

**확인 필요**: 적 공격 어빌리티 내부에서 `Cast<ALastFPSCharacterBase>`나 Pawn 전용 콜리전 채널을 쓰고 있는지 점검. 있다면 그 지점이 실제 수정 대상이 된다.

### 2.5 AI 타깃 등록소 (D5 해결)

문제: `UBTService_UpdateCombatTarget`은 Sight 퍼셉션으로 잡힌 플레이어만 타깃으로 삼는다. 장치는 Pawn이 아니라 퍼셉션에 안 잡힌다. 그렇다고 서비스에 "방어 장치" 분기를 넣으면 범용 AI가 특정 콘텐츠를 알게 된다.

해결: **`ULastFPSCombatTargetRegistry` (WorldSubsystem)** 신설.

```cpp
/** 퍼셉션으로 잡히지 않지만 적대 타깃이 되어야 하는 액터의 등록소. */
void RegisterTarget(AActor& Target, int32 Priority);
void UnregisterTarget(AActor& Target);
AActor* FindBestTarget(const FVector& From, float MaxDistance) const;
```

- 장치는 `StartEncounterObjective()`에서 등록, `StopEncounterObjective()`에서 해제 (수명 명확)
- `UBTService_UpdateCombatTarget`은 "퍼셉션 결과 ∪ 등록소 질의" 중 우선순위·거리 규칙으로 선택
- 우선순위는 AI 프로파일이 소유하는 데이터 값 — 서비스에 "디펜스" 문자열이 등장하지 않는다

### 2.6 실패 경로 — 미션 실패 → 귀환 (D3 해결)

전파 체인:

```
장치 Health 0
  → ULastFPSDefendObjectiveComponent  : OnObjectiveResolved(Failed)
  → ALastFPSRoomEncounterRuntime      : 웨이브 정지 · 적 정리 · 배리어 해제 · 목표 Stop
  → ULastFPSRoomEncounterSubsystem    : OnEncounterFailed(EncounterId)   ← 신설
  → ALastFPSGameModeBase              : HandleMissionFailed(Reason)      ← 신설
  → 실패 화면 표시 후 GameInstance::RequestTravelToHub()
```

**귀환 결정 주체를 GameMode에 두는 이유**: 인카운터 런타임이 직접 Travel을 호출하면 전투 시스템이 맵 전환을 알게 되어 결합이 과하다. GameMode는 이미 `InitialScreenTag`, `EscMenuScreenTag`, `LevelRestrictionEffect`처럼 "맵마다 다른 규칙"을 소유하는 자리이므로 실패 화면 태그·귀환 목적지도 같은 패턴으로 GameMode BP에서 데이터로 지정한다.

기존 `FailEncounterOpen()`은 구성 오류 전용임이 이름에서 드러나지 않으므로 `AbortEncounterOnConfigurationError()`로 개명해 게임플레이 실패(`FailEncounter()`)와 구분한다.

**퀘스트 실패 상태는 이번 범위에서 만들지 않는다.** `ELastFPSQuestStatus`는 단조 전이(NotStarted→InProgress→Completed→Claimed)만 지원하며 실패 상태가 없다. 허브 귀환 후 재입장하면 인카운터가 재생성되므로, 퀘스트는 InProgress 유지로 충분하다. 실패 카운트·페널티가 필요해지면 별도 계획으로 다룬다.

### 2.7 진행 이벤트와 HUD (D6 해결)

`ULastFPSRoomEncounterSubsystem`에 목표 진행 이벤트 신설:

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FLastFPSOnEncounterObjectiveProgress,
    FName, EncounterId,
    float, Progress01,
    int32, LoopCount);
```

HUD는 이 이벤트만 구독하므로 위젯이 `ULastFPSDefendObjectiveComponent` 구체 클래스를 몰라도 되고, 점령전이 같은 위젯을 재사용한다.

신규 `LastFPSDefenseObjectivePresenter` (기존 `LastFPSVitalsGaugePresenter` 패턴):

- 장치 체력 게이지 — Health/MaxHealth 어트리뷰트 변경 구독
- 버티기 진행 게이지 — 위 진행 이벤트
- 웨이브 / 순환 횟수 표시

### 2.8 방어 대상 화면 마커 — 클리어까지 상시 표시 (D8 해결)

**요구**: 방어해야 할 장치에 마커가 붙어 있어야 하며, 인카운터가 클리어될 때까지 유지된다.

기존 마커 파이프라인은 이렇게 돈다:

```
ULastFPSObjectiveMarkerWidget::NativeTick
  → QuestSubsystem::GetActiveWaypoints()      // ReachLocation 목표만
  → 바닥 투영 (bIsRoutePoint면 스킵)
  → 화면 투영 · 가장자리 클램프 · 거리(m)
  → EntryWidget::UpdateMarker()
```

여기에 방어 장치를 얹으려면 **네 군데가 걸린다**:

| 충돌 | 현재 | 필요 |
|---|---|---|
| 마커 소스 | `Obj.Type != ReachLocation`이면 건너뜀 (하드코딩된 타입 검사) | 인카운터 목표도 마커를 낼 수 있어야 함 |
| 수명 | 퀘스트 목표 진행(`Progress >= RequiredCount`)에 묶임 | **인카운터 목표 수명**에 묶여야 함 (퀘스트 없이 배치된 방어 방도 마커가 떠야 함) |
| 앵커 | `bIsRoutePoint` bool → 바닥 투영 / 그대로 2종 | 장치는 **바운즈 상단**에 떠야 한다. 바닥에 붙이면 장치에 가려짐 |
| 표시 항목 | 거리 · 라벨 · 방향 화살표 | 장치 체력 · 버티기 진행률 게이지 |

#### 2.8.1 마커 등록소 — 소스 통합

퀘스트 서브시스템이 인카운터를 역으로 조회하게 만들면 새 의존 방향이 생긴다. 대신 **마커를 띄우려는 주체가 스스로 등록**하는 등록소를 둔다. 프로젝트에 이미 `RegisterLocationMarker` / `UnregisterLocationMarker` 등록소 패턴이 있으므로 새 개념이 아니라 그 확장이다.

신규 `Source/LastFPS/Quest/LastFPSWorldMarkerSubsystem.h` (WorldSubsystem)

```cpp
/** 마커 1건의 등록 요청 — 위치 소스는 액터 추적(약참조)이라 이동체도 지원한다. */
USTRUCT()
struct FLastFPSWorldMarkerRequest
{
    TWeakObjectPtr<const AActor> AnchorActor;
    FText Label;
    ELastFPSMarkerAnchor Anchor = ELastFPSMarkerAnchor::ActorTop;
    float VerticalOffset = 0.f;
};

UCLASS()
class ULastFPSWorldMarkerSubsystem : public UWorldSubsystem
{
    /** 해제 전까지 HUD에 유지된다. 같은 Owner로 다시 등록하면 갱신한다. */
    void RegisterMarker(const UObject& Owner, const FLastFPSWorldMarkerRequest& Request);
    /** 게이지 값 갱신 — 음수는 "표시 안 함". */
    void UpdateMarkerGauges(const UObject& Owner, float Progress01, float Health01);
    void UnregisterMarker(const UObject& Owner);
    void CollectMarkers(TArray<FLastFPSObjectiveWaypoint>& Out) const;
};
```

HUD 컨테이너 위젯은 `GetActiveWaypoints()`(퀘스트) + `CollectMarkers()`(월드) 두 소스를 합쳐 렌더한다. 위젯은 "방어"를 모르고 웨이포인트 계약만 안다.

#### 2.8.2 수명 — 인터페이스에 그대로 매핑

요구사항("클리어될 때까지")이 §2.1 인터페이스 수명과 정확히 일치하므로 추가 상태 없이 붙는다:

| 시점 | 동작 |
|---|---|
| `StartEncounterObjective()` | `RegisterMarker(장치, ActorTop, 라벨)` |
| 진행 갱신 타이머 | `UpdateMarkerGauges(버티기 진행률, 장치 체력)` |
| `StopEncounterObjective()` / 성공 / 실패 | `UnregisterMarker()` |

인카운터 클리어 시 런타임이 모든 목표에 `Stop`을 호출하므로 마커는 자동으로 사라진다. 실패·레벨 언로드 경로도 같은 지점을 지나므로 누수가 없다.

#### 2.8.3 앵커 방식 — bool → enum 승격

`bIsRoutePoint` bool로는 3종을 표현할 수 없다. `FLastFPSObjectiveWaypoint`를 개정한다:

```cpp
UENUM(BlueprintType)
enum class ELastFPSMarkerAnchor : uint8
{
    Ground   UMETA(DisplayName="바닥 투영"),   // 기존 목적지 마커
    Exact    UMETA(DisplayName="지정 좌표"),   // 기존 동선 지점 (bIsRoutePoint == true)
    ActorTop UMETA(DisplayName="액터 상단")    // 방어 장치·호위 대상
};
```

- `bIsRoutePoint` 제거 → `Anchor` 필드로 대체
- 게이지 표시용 `Progress01` / `Health01` 추가 (음수 = 미표시)
- `FLastFPSObjectiveMarkerDisplay`에도 대응 필드 추가, 엔트리 위젯은 `BindWidgetOptional` 게이지 바인딩

> **회귀 주의**: `bIsRoutePoint`는 `BlueprintReadOnly`로 노출돼 있어 WBP에서 참조 중일 수 있다. 제거 전에 Blueprint 참조를 검색하고, 있으면 함께 갱신한다.

`ActorTop`은 매 프레임 `AnchorActor->GetComponentsBoundingBox()` 상단 + 오프셋으로 계산한다. 바닥 트레이스 캐시(`GroundLocationCache`) 경로는 타지 않으므로 추가 트레이스 비용이 없다.

#### 2.8.4 표시 규칙

- **오클루전 검사 없음** — 벽 뒤에서도 보여야 한다. 현재 마커도 검사하지 않으므로 그대로 둔다.
- **근거리 거리 텍스트 억제** — 장치 바로 옆에서 "3m"가 뜨면 거슬린다. 임계 거리 이하에서 숨기며, 값은 위젯 프로퍼티로 노출한다.
- **화면 밖 클램프** — 기존 가장자리 화살표 로직을 그대로 재사용한다. 방어 중 장치가 뒤에 있을 때 방향을 알려주는 것이 이 요구의 핵심 가치다.

### 2.9 퀘스트 진행 표시 회귀 방지 ⚠️

`ULastFPSQuestSubsystem::NotifyEncounterProgress(EncounterId, Defeated, Total)`는 `ClearEncounter` 목표의 진행을 **처치 수**로 계산한다. 웨이브가 순환하면 `Defeated`가 `Total`을 넘어 의미가 깨진다.

대응:

1. 디펜스 방은 퀘스트 목표를 `ClearEncounter`가 아니라 **`DefendZone`** 으로 작성한다 (기존 `NotifyObjectiveDefended` 경로 그대로 사용).
2. 목표가 붙은 인카운터는 `BroadcastEncounterProgress()`에서 처치 수 대신 목표 진행률을 보낸다.
3. `ULastFPSQuestSubsystem::ResolveObjectiveRequiredCount()`가 `ClearEncounter`에서 Encounter Profile의 적 수 합계를 읽는 경로가 순환 인카운터에 영향받지 않는지 확인한다.

### 2.10 데이터 소유권 — 방마다 섬멸/점령/방어를 데이터로 고르게 한다

> **개정 이력**: 이 절은 초안에서 "행에 목표 설정을 넣지 않는다"였다. 그 판단은 **구체 필드**(`HoldDuration`, `DeviceDefinition` 등)를 행에 직접 박는 형태를 전제로 한 것이고, 그 전제에서는 지금도 유효하다. 그러나 **다형 정의 참조 배열**을 두는 형태는 그 문제를 일으키지 않으면서 이점이 더 크다. 아래로 개정한다.

#### 2.10.1 이름은 `Encounter`가 맞다 — `Dungeon`으로 바꾸지 않는다

"Encounter(조우)"는 **한 공간에서 벌어지는 전투 교전**을 뜻하는 일반 용어이지 "섬멸"을 뜻하지 않는다. 그 안의 목표가 섬멸이든 점령이든 방어든 여전히 조우다. 지금 이름이 섬멸형처럼 읽히는 건 이름 탓이 아니라 **완료 조건이 코드에 박혀 있기 때문**이며, §2.3에서 그걸 걷어내면 이름과 실체가 맞아떨어진다.

`Dungeon`으로 바꾸면 오히려 나빠진다.

- 던전은 **맵/레벨 단위** 개념인데 이건 **방 단위** 구성이다. 스코프가 어긋난다.
- 야외·기지·호위 구간 등 던전이 아닌 목적지에도 쓸 구조인데 이름이 용도를 좁힌다.
- 에셋 리네임 + 리다이렉터 + 전 참조 수정 비용이 드는 데 비해 얻는 게 없다.

`RoomEncounter` = "방 단위 조우 구성"으로 유지한다.

#### 2.10.2 행이 목표 **정의 참조**를 소유한다 (개정)

```cpp
USTRUCT(BlueprintType)
struct FLastFPSRoomEncounterData : public FTableRowBase
{
    TArray<FLastFPSRoomEncounterWaveDefinition> Waves;   // 기존

    /**
     * 이 방이 요구하는 목표들. 비우면 섬멸형(웨이브 소진 = 클리어).
     * 목표의 종류와 밸런스는 각 정의 에셋이 소유하고, 위치는 레벨 배치물이 소유한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
    TArray<TSoftObjectPtr<ULastFPSEncounterObjectiveDefinition>> Objectives;
};
```

정의는 **다형 Data Asset**이다.

```cpp
UCLASS(Abstract, BlueprintType)
class ULastFPSEncounterObjectiveDefinition : public UDataAsset
{
public:
    /** 레벨의 어느 배치물과 짝지어지는가. 배치물이 같은 태그를 들고 있어야 한다. */
    UPROPERTY(EditAnywhere, Category="Objective")
    FGameplayTag ObjectiveTag;

    /** HUD 마커·트래커에 쓸 표시 라벨(§2.8). */
    UPROPERTY(EditAnywhere, Category="Objective")
    FText MarkerLabel;

    /** 배치물에 런타임 목표를 구성해 붙인다. 런타임은 이 가상 호출만 안다. */
    virtual ULastFPSEncounterObjectiveComponent* CreateRuntimeObjective(AActor& Anchor) const
        PURE_VIRTUAL(ULastFPSEncounterObjectiveDefinition::CreateRuntimeObjective, return nullptr;);

    virtual void CollectRequiredPaths(TArray<FSoftObjectPath>& OutPaths) const {}
};

// 방어: 버티기 시간 · 장치 초기화 GE · 실패 규칙
UCLASS() class ULastFPSDefendObjectiveDefinition  : public ULastFPSEncounterObjectiveDefinition { ... };
// 점령: 점령 시간 · 경합 규칙
UCLASS() class ULastFPSCaptureObjectiveDefinition : public ULastFPSEncounterObjectiveDefinition { ... };
```

**런타임에 유형 분기가 생기지 않는다.** 인카운터 런타임은 행에서 정의 목록을 받아 각 정의에 `CreateRuntimeObjective()`를 호출하고, 돌려받은 컴포넌트를 §2.1 인터페이스로만 다룬다. 호위·해킹·생존을 추가할 때 **행 구조도 런타임도 건드리지 않는다** — 서브클래스와 에셋만 늘어난다.

#### 2.10.3 세 갈래 소유권 (개정)

| 데이터 | 소유자 | 이유 |
|---|---|---|
| 웨이브 구성, 스폰 간격/배치, VFX + **이 방의 목표 목록** | `DT_RoomEncounter` 행 | 방 단위 **조우 구성**. 한 곳에서 방 전체가 읽힌다 |
| 버티기 시간, 장치 체력, 점령 시간, 실패 규칙 | `DA_*ObjectiveDefinition` | **목표 유형별 밸런스**. 여러 방이 공유·재사용 |
| 목표 위치·크기, 어느 방 소속인가 | 레벨 배치 (액터 태그) | 본질적으로 **공간 데이터** |
| 목표 문구, 보상, 무전 | `DT_QuestData` 행 | **임무 서술** |

레벨 배치물의 역할이 **앵커로 가벼워진다.** 밸런스 프로퍼티를 들고 있지 않고 `EncounterId` + `ObjectiveTag` 두 태그만 단 채 "여기가 그 목표 자리"라고 표시할 뿐이다. 배리어·스폰포인트가 이미 `EncounterId` 태그로 매칭되는 기존 레벨 계약과 정확히 같은 패턴이다.

#### 2.10.4 개정으로 얻는 것

- **`ExpectedObjectiveCount`가 불필요해진다.** 행의 `Objectives` 배열이 곧 기대치이고, 태그로 배치물을 못 찾으면 그 자리에서 어느 목표가 빠졌는지 이름을 찍어 에러를 낼 수 있다. 초안의 "조용한 실패" 위험이 사라진다.
- **테이블 한 곳에서 방 유형이 보인다.** 레벨을 열지 않고도 어느 방이 방어이고 어느 방이 섬멸인지 읽힌다.
- **비동기 프리로드가 공짜로 붙는다.** `ULastFPSRoomEncounterProfile::CollectRequiredPaths()`는 이미 모든 행을 순회하며 웨이브의 적 정의 소프트 경로를 모은다. 같은 루프에서 목표 정의 경로를 모으면 되고, 정의가 참조하는 GE·메시는 정의의 `CollectRequiredPaths()`가 재귀로 넘긴다. 새 로딩 기계가 필요 없다.
- **한 방에 목표 여럿**이 자연스럽다 (`Objectives` 배열 + 태그별 배치물).

> **불변 설정 / 런타임 상태 분리**: 정의 에셋은 읽기 전용이며 `ElapsedSeconds` 같은 진행 상태는 런타임 컴포넌트가 소유한다. 공유 에셋에 일시적 상태를 저장하지 않는다.

#### 2.10.5 남는 비용

정의(테이블·에셋)와 배치(레벨)가 `ObjectiveTag`로 매칭되므로 **태그 오타가 런타임 에러로만 잡힌다.** 이건 배리어·스폰포인트가 이미 감수하는 비용과 동일하며, 초기화 시 누락 목표 이름을 로그로 남기는 것으로 대응한다. 에디터 검증(월드 파티션 스캔 등)은 후속 과제로 남긴다.

### 2.11 퀘스트 진행·다음 목표 표기는 어디서 오는가

**전제 정정**: 퀘스트 체인과 목표 표기는 원래부터 Encounter 테이블이 소유한 적이 없다. `DT_QuestData`가 소유한다. Encounter 테이블은 다음 두 지점에서만 참조된다.

| 참조 지점 | 용도 |
|---|---|
| `ClearEncounter` 목표의 `TargetId` | `ResolveObjectiveRequiredCount()`가 `GetTotalEnemyCount()`로 **요구 수량을 자동 계산** |
| `ReachLocation` 목표의 `TargetId` (선택) | 위치 목표를 **방 트리거와 연결** |

즉 Encounter 테이블은 "요구 수량 자동 계산"과 "방 연결" 용도였고, **다음 퀘스트 표기와는 무관하다.** 표기 경로는 이렇게 그대로 살아 있다.

| 표기 대상 | 소유 필드 / 클래스 |
|---|---|
| 다음 퀘스트 해금·자동 수락 | `FLastFPSQuestData::NextQuestId` / `PrereqQuestId` / `QuestGiverNPC` → `AdvanceToNext()` |
| 같은 퀘스트 내 다음 목표 | `bSequentialObjectives = true` → 배열 순서대로 하나씩 활성 (`GetActiveWaypoints()`도 첫 미완료에서 `break`) |
| HUD 목록 표시 | `ULastFPSQuestTrackerWidget` (`Status == InProgress` 필터) + `ULastFPSQuestEntryWidget` |
| 단계 전환 연출 | 목표별 `RadioOnStart` / `RadioOnComplete` |

#### 2.11.1 디펜스 방의 목표 작성 형태

이동 단계는 여전히 `ReachLocation` + `TargetId = EncounterId`로 방 트리거와 연결하고, 그 다음 목표가 `DefendZone`이다. `bSequentialObjectives = true`로 두면 단계 전환이 그대로 동작한다.

```
[0] ReachLocation  TargetTag = Location.Quest.Dungeon.DefenseRoom
                   TargetId  = Enc_DefenseRoom        ← Encounter 행과 연결 유지
                   RadioOnComplete = 도착 무전
[1] DefendZone     TargetTag = Objective.Zone.Defense.Core
                   RequiredCount = 1
                   RadioOnStart / RadioOnComplete
```

**`ClearEncounter` 대신 `DefendZone`을 써서 잃는 것은 `RequiredCount` 자동 계산 하나뿐이다.** 그런데 디펜스는 "구역 1곳 방어 성공"이라 `RequiredCount = 1`을 직접 쓰면 되고, 애초에 §2.3 웨이브 순환에서는 적 수 합계가 의미를 잃으므로 자동 계산이 없는 편이 옳다.

#### 2.11.2 다만 트래커 진행 표시가 0 → 1로 점프한다 ⚠️

`ClearEncounter`는 `NotifyEncounterProgress`로 중간 진행(예: 3/12 처치)이 트래커에 흐르지만, `DefendZone`은 `FTagEventTracker` push형이라 **성공 순간에 0에서 1로 점프한다.** 버티는 60초 동안 트래커에는 아무 변화가 없다.

이를 목표 진행률(0~100 같은 백분율)로 바꾸면 데이터 작성이 기형이 되고, 퀘스트 목표에 "진행률 표시 소스" 개념을 새로 만들면 과한 일반화다. **역할을 나누는 편이 맞다.**

| 표시 | 담당 | 내용 |
|---|---|---|
| 퀘스트 트래커 | `ULastFPSQuestEntryWidget` | 단계 라벨 + 달성 여부 (0/1) |
| 실시간 버티기·장치 체력 | §2.7 디펜스 HUD 패널 | 게이지 + 웨이브·순환 카운트 |
| 대상 위치·상태 | §2.8 화면 마커 | 방향·거리 + 게이지 |

이미 계획에 두 표시가 모두 있으므로 추가 작업 없이 채워진다. **트래커에 실시간 게이지를 넣으려는 시도는 하지 않는다.**

### 2.12 다중 레벨·목적지 스코프 — 구조는 되어 있고, 실물 에셋이 안 나뉘어 있다

#### 2.12.1 구조: 이미 목적지 단위로 스코프된다

Encounter는 단일 레벨 전제로 짜여 있지 않다. 스코프 체인이 이미 있다.

```
맵(레벨) → GameMode BP(맵마다 다름) → DA_ContentSet_*        (목적지별 콘텐츠)
        → Features[] → ULastFPSRoomEncounterProfile → EncounterTable  (목적지별 테이블)
```

- `ULastFPSRoomEncounterSubsystem`은 **WorldSubsystem**이라 월드마다 하나씩 생기고, 프로파일을 `GameMode->GetDestinationContentSet()->FindFeature<...>()`로 가져온다. 전역 싱글턴 테이블이 아니다.
- 퀘스트도 월드 스코프를 지킨다. `ULastFPSQuestSubsystem::GetEncounterTable()`과 `BindEncounterEvents()` 모두 **현재 월드의** 인카운터 서브시스템을 거친다.
- `DungeonMapQuestMap`(맵 키워드 → 퀘스트 ID)이 이미 다중 던전을 전제한다.

따라서 `EncounterId`는 **자기 테이블 안에서만 유일하면 되고**, 어느 던전 소속인지는 "어느 테이블에 실려 있는가"가 답한다.

#### 2.12.2 실물: 프로파일도 테이블도 하나뿐이다 ⚠️

| 에셋 | 현황 |
|---|---|
| `DA_ContentSet_Hub`, `DA_ContentSet_OrbitalElevator` | 목적지 **2개** |
| `DA_EncounterProfile` | **1개** — 목적지별로 안 나뉨 |
| `DT_EnCounter_New` | **1개** — 모든 방이 한 테이블 |

구조는 다목적지를 지원하는데 **콘텐츠가 아직 안 나뉘어 있다.** 지금 상태로 던전을 하나 더 만들면 같은 테이블에 방이 섞이고, `Enc_Room01` 같은 이름이 충돌할 여지가 생긴다. "어떤 레벨·던전인지 알 수 없다"는 지적은 이 실물 기준에서 정확하다.

#### 2.12.3 §2.10 개정이 이 분리를 더 급하게 만든다

행에 `Objectives`(목표 정의 소프트 참조)를 넣기로 했으므로, `ULastFPSRoomEncounterProfile::CollectRequiredPaths()`가 **테이블의 모든 행을 순회하며** 목표 정의와 그 하위 GE·메시까지 프리로드 경로에 넣는다.

한 테이블에 모든 던전이 실려 있으면 **던전 A에 입장할 때 던전 B의 방어 장치 메시·이펙트까지 로드된다.** 지금은 웨이브의 적 정의만 모으므로 낭비가 눈에 안 띄지만, 목표 에셋이 붙으면 커진다.

#### 2.12.4 이름: `Dungeon`도 `BattleMap`도 아닌 `Destination`

`Dungeon`은 부적절하다. 지하 미궁이라는 장르 픽션을 끌고 오는데, 이 프로젝트의 전투 목적지는 `OrbitalElevator`(궤도 엘리베이터) 같은 SF 시설이다.

`BattleMap`은 그보다 낫고 `Content/Maps/Battle/` 폴더와도 맞지만, 두 가지가 걸린다.

- **`Map`은 엔진 용어(`UWorld`)와 충돌한다.** 목적지가 반드시 1맵이 아니다. `FLastFPSStreamingLevelTransitionRoute`는 `SourceWorld` + `DestinationLevel` 조합이라, 한 목적지가 퍼시스턴트 월드의 서브레벨일 수 있다. `BattleMapId`가 `UWorld` 1:1을 암시하면 실제 구조와 어긋난다.
- **허브가 개념 밖으로 밀려난다.** `DA_ContentSet_Hub`도 목적지다. `BattleMap`이라 부르면 허브만 별도 취급해야 한다.

**프로젝트에 이미 정확한 단어가 있다 — `Destination`.** 헤더 16개에서 쓰이는 확립된 어휘다.

```
ULastFPSDestinationContentSet   ULastFPSDestinationFeature   ULastFPSDestinationContentComponent
ELastFPSTravelDestination       GameMode::GetDestinationContentSet()
```

`DA_ContentSet_Hub` / `DA_ContentSet_OrbitalElevator`가 이미 "목적지별 콘텐츠"다. 새 어휘를 만들지 않고 `DestinationId`로 간다.

전투/허브 구분은 **카테고리 게임플레이 태그**로 한다.

```cpp
/** Destination.Hub / Destination.Battle / Destination.Raid ... */
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Destination", meta=(Categories="Destination"))
FGameplayTag CategoryTag;
```

허브 임무 선택 UI는 `Destination.Battle` 하위만 필터한다. 레이드·침공 이벤트 같은 범주가 늘어도 enum을 고치지 않는다.

#### 2.12.5 결론: 테이블을 쪼개지 않고 행에 `DestinationId`를 둔다 (개정)

> **개정 이력**: 초안은 "목적지마다 프로파일 + 테이블 분리"를 권장했다. **철회한다.** 분리는 프리로드 문제만 풀고 정작 원래 문제("어느 목적지 소속인지 데이터에서 안 보인다")는 *더* 악화시킨다 — 소속이 "어느 테이블에 실려 있는가"라는 **암묵지**가 되어, 테이블을 열어봐야 알 수 있다.

`FLastFPSRoomEncounterData`에 소속을 **명시 필드**로 둔다.

```cpp
/** 이 방이 속한 목적지(DT_Destination 행). 프리로드·초기화 범위를 가르는 기준이다. */
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
FName DestinationId;
```

`ULastFPSRoomEncounterProfile`도 자기 목적지를 선언하고, **행을 순회할 때 `DestinationId`로 걸러낸다.**

```cpp
UPROPERTY(EditDefaultsOnly, Category="Data")
FName DestinationId;
```

| 항목 | 테이블 분리 | **행에 `DestinationId`** |
|---|---|---|
| 프리로드 범위 | 테이블 단위로 갈림 | `CollectRequiredPaths()`가 `DestinationId`로 필터 — **동일하게 해결** |
| 소속 가독성 | 암묵지 (어느 테이블인지 알아야 함) | **행에 그대로 보임** |
| 에셋 관리 | 목적지마다 프로파일+테이블 복제, 동기화 부담 | 테이블 1개 유지 |
| `EncounterId` 충돌 | 테이블이 달라 감지 안 됨 | 같은 테이블이라 **행 이름이 강제로 유일** |

마지막 항목이 결정적이다. 한 테이블이면 UE가 행 이름 중복을 애초에 허용하지 않으므로, 접두사 네이밍 규칙 같은 **사람이 지켜야 하는 약속이 필요 없다.**

> **중복 선언 여지**: 프로파일은 이미 `DA_ContentSet_*`(목적지별 콘텐츠 셋) 안에 들어 있으므로, `DestinationId`를 프로파일이 아니라 **콘텐츠 셋**이 소유하고 기능들이 읽어가는 형태도 가능하다. 그러면 목적지 정체성이 한 곳에만 적힌다. 구현 시 결정한다.

#### 2.12.6 목적지 테이블은 어차피 필요하다 — 허브 진입 흐름이 없다

현재 허브에서 전투 목적지로 가는 흐름은 **데이터로 존재하지 않는다.**

```cpp
// LastFPSTravelTypes.h — 하드코딩된 열거
enum class ELastFPSTravelDestination : uint8 { MainMenu, CharacterSelect, Hub };

// LastFPSGameInstance.cpp — 그 위의 switch
bool ULastFPSGameInstance::ResolveMapURL(ELastFPSTravelDestination Destination, ...) { switch (...) }
```

전투 목적지 항목 자체가 없고, 실제 진입은 `ULastFPSStreamingLevelTransitionSettings`(트리거 박스 → 레벨 스트리밍)로만 이뤄진다. 즉 **"어떤 이벤트로 어느 목적지에 가는가"를 표현할 자리가 없다.** 이 enum + switch는 CLAUDE.md의 *"구체 대상을 모두 열거하는 switch를 포함하지 않아야 한다"* 에 정면으로 걸린다.

`DT_Destination`이 그 자리다.

```cpp
USTRUCT(BlueprintType)
struct FLastFPSDestinationData : public FTableRowBase
{
    FText DisplayName;
    FText Description;
    TSoftObjectPtr<UTexture2D> Thumbnail;

    /** Destination.Hub / Destination.Battle ... — 허브 UI 필터 기준. */
    FGameplayTag CategoryTag;

    /** 어디로 가는가 — 맵 직행 또는 스트리밍 경로 중 하나. */
    TSoftObjectPtr<UWorld> Map;
    FName EntryRouteId;

    /** 언제 열리는가 — 허브 이벤트/퀘스트 게이팅. */
    FName RequiredQuestId;
    ELastFPSQuestStatus RequiredQuestStatus = ELastFPSQuestStatus::Completed;
};
```

`DestinationId`는 이 테이블의 행 이름이므로, 인카운터 행의 `DestinationId`는 **이미 존재할 예정인 키를 재사용하는 외래 키**일 뿐이다. 새 개념을 만드는 게 아니다.

> **시스템 맵 enum은 유지한다.** `MainMenu` / `CharacterSelect` / `Hub`는 개수가 고정된 시스템 화면이라 열거가 타당하다. 전투 목적지만 enum에 추가하지 않고 `RequestTravelToDestination(FName DestinationId)`로 분리한다. (기존 enum 오버로드와 이름이 겹치므로 구현 시 정리)

#### 2.12.7 범위 구분

`DT_Destination`과 허브 임무 선택 UI는 **이 계획의 범위가 아니다.** 별도 계획(`destination-selection.plan.md`)으로 다룬다. 이번 계획은 다음만 가져간다.

- `FLastFPSRoomEncounterData::DestinationId` 필드 추가
- `ULastFPSRoomEncounterProfile::DestinationId`(또는 콘텐츠 셋 소유) + 행 순회 시 필터링 (프리로드 · 런타임 초기화)

`DT_Destination`이 아직 없어도 `DestinationId`는 단순 `FName`이라 먼저 넣어둘 수 있고, 나중에 테이블이 생기면 그대로 외래 키가 된다. **디펜스 한 방을 만드는 데는 지장이 없다** — 현재 테이블에 행 하나를 추가하고 `DestinationId`만 채우면 된다.

---

## 3. 작업 목록 (Tasks)

### Phase 0 — 계약 정의 (동작 변화 없음, 빌드만 통과)
- [ ] `ILastFPSEncounterObjective` 인터페이스 + 결과 enum + 델리게이트 정의
- [ ] `ULastFPSRoomEncounterProfile::ObjectiveMarkerTag` 추가
- [ ] `ULastFPSEncounterObjectiveDefinition` 추상 Data Asset (`ObjectiveTag`, `MarkerLabel`, `CreateRuntimeObjective`, `CollectRequiredPaths`)
- [ ] `FLastFPSRoomEncounterData::Objectives` 추가 (다형 정의 소프트 참조 배열, 빈 배열 = 섬멸형 → 기존 행 무영향)
- [ ] `FLastFPSRoomEncounterData::DestinationId` + `ULastFPSRoomEncounterProfile::DestinationId` 추가 (§2.12)
- [ ] `CollectRequiredPaths()`가 목표 정의 경로도 수집하되 `DestinationId`로 행 필터링
- [ ] `InitializeRuntimeEncounters()`도 `DestinationId` 불일치 행은 건너뜀
- [ ] **회귀 확인**: 기존 행은 `DestinationId`가 비어 있음 — 빈 값을 "필터 안 함"으로 처리해 무영향 보장
- [ ] `ULastFPSRoomEncounterSubsystem`에 `OnEncounterFailed` / `OnEncounterObjectiveProgress` 이벤트 추가

### Phase 1 — 수명 연결
- [ ] `ULastFPSDefendObjectiveComponent`가 인터페이스 구현, `bAutoStart` 제거
- [ ] `ULastFPSCaptureZoneComponent`가 인터페이스 구현 (점령 배선도 같은 배관에 태움)
- [ ] 서브시스템이 목표 마커 액터 수집 → `InitializeEncounter` 시그니처 확장
- [ ] 행의 `Objectives` 정의별로 `ObjectiveTag`가 일치하는 배치물을 찾아 `CreateRuntimeObjective()` 호출
- [ ] 짝을 못 찾은 정의는 어느 목표인지 이름을 찍어 에러 로그 (배치 누락의 조용한 실패 방지)
- [ ] 런타임이 목표 컴포넌트 보관·구독, `StartEncounter`/`CompleteEncounter`/`EndPlay`에서 Start·Stop 호출
- [ ] `EvaluateWaveCompletion()` 논리곱 + 웨이브 순환(`LoopCount` 복제)
- [ ] **회귀 확인**: 목표 없는 기존 섬멸형 인카운터가 그대로 동작

### Phase 2 — 장치 GAS 전환
- [ ] `ULastFPSDefendObjectiveDefinition` 서브클래스 (버티기 시간, 장치 초기화 GE, 표시 메시, 실패 규칙)
- [ ] `HoldDuration` / `MaxIntegrity` 를 배치 인스턴스에서 정의 에셋으로 이동 (배치물은 앵커로 축소)
- [ ] `ALastFPSDefendableDeviceActor`에 ASC + AttributeSet 부착, `IAbilitySystemInterface` 구현
- [ ] `Integrity` 계열 제거 → Health 어트리뷰트 변경 구독으로 파괴 감지
- [ ] 적 공격 어빌리티에 Pawn/Character 가정(`Cast`, 콜리전 채널)이 있는지 점검·수정

### Phase 3 — AI 타깃
- [ ] `ULastFPSCombatTargetRegistry` WorldSubsystem 신설
- [ ] 장치가 목표 Start/Stop에서 등록·해제
- [ ] `UBTService_UpdateCombatTarget`이 등록소를 후보에 포함, 우선순위는 AI 프로파일 데이터로

### Phase 4 — 실패 경로
- [ ] 런타임 `FailEncounter()` 신설, `FailEncounterOpen` → `AbortEncounterOnConfigurationError` 개명
- [ ] 서브시스템 `NotifyEncounterFailed` 전파
- [ ] `ALastFPSGameModeBase::HandleMissionFailed()` + 실패 화면 태그/귀환 목적지 프로퍼티
- [ ] 실패 → 허브 귀환 흐름 연결

### Phase 5 — HUD 패널
- [ ] 진행 이벤트 브로드캐스트 (처치 수 대신 목표 진행률)
- [ ] `LastFPSDefenseObjectivePresenter` + 디펜스 목표 위젯 (장치 체력 / 버티기 / 웨이브·순환)

### Phase 6 — 방어 대상 화면 마커
- [ ] `ELastFPSMarkerAnchor` enum 신설, `FLastFPSObjectiveWaypoint`의 `bIsRoutePoint` → `Anchor` 승격
- [ ] **선행**: `bIsRoutePoint`의 Blueprint 참조 검색 후 함께 갱신
- [ ] `FLastFPSObjectiveWaypoint` / `FLastFPSObjectiveMarkerDisplay`에 `Progress01` · `Health01` 추가
- [ ] `ULastFPSWorldMarkerSubsystem` 신설 (등록 / 게이지 갱신 / 해제 / 수집)
- [ ] `ULastFPSObjectiveMarkerWidget`이 퀘스트 + 월드 두 소스를 합쳐 렌더, `ActorTop` 앵커 처리
- [ ] 엔트리 위젯에 게이지 바인딩(`BindWidgetOptional`) + 근거리 거리 텍스트 억제
- [ ] `ULastFPSDefendObjectiveComponent`가 Start/진행/Stop에서 등록·갱신·해제

### Phase 7 — 데이터 & 레벨
- [ ] `DefaultGameplayTags.ini`에 방어 구역 태그 추가
- [ ] `DT_RoomEncounter`에 디펜스 방 행 (`DestinationId` + 순환 전제 웨이브 + `Objectives = [DA_DefendObjective_Core]`)
- [ ] `DA_DefendObjective_Core` 작성 (`ObjectiveTag`, 버티기 시간, 장치 체력 밸런스)
- [ ] **회귀 확인**: 기존 섬멸형 행들은 `Objectives`가 빈 배열이라 동작 변화 없음
- [ ] `DT_QuestData`에 `bSequentialObjectives = true` + `[0] ReachLocation(TargetId=EncounterId)` → `[1] DefendZone` 목표 행 + 무전 대사
- [ ] 퀘스트 체인 연결 확인 (`PrereqQuestId` / `NextQuestId`) — 디펜스 임무 전후 단계
- [ ] 레벨 배치: 트리거·배리어·스폰포인트 + 목표 마커 태그를 단 장치 액터

### Phase 8 — 검증
- [ ] 정상 경로: 진입 → 순환 웨이브 → 버티기 성공 → 퀘스트 진행 → 배리어 해제
- [ ] 실패 경로: 장치 파괴 → 미션 실패 → 허브 귀환
- [ ] 마커: 인카운터 시작에 등장 → 클리어까지 유지 → 종료 시 사라짐. 화면 밖 클램프, 벽 뒤 가시성, 게이지 갱신
- [ ] 경계: 목표 없는 인카운터 회귀, 기존 `ReachLocation` 마커·동선 지점 회귀, 목표 성공과 마지막 적 사망 동시 발생, 재입장 시 재초기화
- [ ] 네트워크: 서버 / 소유 클라 / 비소유 클라 3관점 — 진행률·장치 체력·배리어·순환 카운트 복제, 마커가 클라이언트에서도 뜨는지
- [ ] 수명: 목표 델리게이트 등록·해제, 타깃/마커 등록소 등록·해제, 레벨 언로드 시 누수 없음
- [ ] 빌드는 사용자 요청 시에만 실행 — 미실행 시 검증 결과에 명시

---

## 4. 열린 질문

- **순환 난이도 스케일링**: `LoopCount`가 오를 때 적 수·능력치를 올릴 것인가. 올린다면 Curve로 데이터화. (현재는 훅만 남기고 미구현)
- **성공 시 잔여 적**: 즉시 제거로 시작하되, 연출(퇴각·소멸 VFX)이 필요하면 후속.
- **멀티플레이 점령 판정**: `ULastFPSCaptureZoneComponent::IsLocalPlayerPawn`이 로컬 플레이어만 점령 주체로 인정한다. 협동에서 다인 점령 가속이 필요하면 별도 논의.
- **마커 게이지 표시 범위**: 장치 체력을 마커에 계속 띄울지, 아니면 피격 중일 때만 띄우고 평소엔 아이콘만 둘지. 상시 표시는 정보량이 늘지만 화면이 지저분해진다.
- **정의 에셋의 입도**: 시간·체력이 정의 에셋으로 올라가면 그 에셋이 "밸런스 프리셋" 단위가 된다. 방마다 미세 조정이 필요하면 에셋 수가 늘어난다. 프리셋 재사용을 기본으로 하되 방별 편차를 어디까지 허용할지 정해야 한다. (행에 오버라이드 필드를 두는 방식은 §2.10.2의 "행이 부푸는" 문제로 되돌아가므로 피한다.)
- **장치가 여럿인 방**: 등록소는 다중 마커를 지원하지만, 방어 대상이 2개 이상일 때 승패 규칙(전부 지켜야 하는가 / 하나만 남아도 되는가)이 미정. 현재 `ULastFPSDefendObjectiveComponent`는 `Device` 단일 참조다.
- **`DT_Destination` 작성 시점**(§2.12.6): 별도 계획으로 분리했으나, `DestinationId`가 실제 외래 키로 검증되려면 목적지 테이블이 먼저 있어야 한다. 이번 계획과 병행할지 이후로 미룰지.
- **목적지 진입 방식 일원화**: 현재 전투 목적지 진입은 스트리밍 경로(`ULastFPSStreamingLevelTransitionSettings`)로만 가능하다. `DT_Destination`이 맵 직행(`ServerTravel`)과 스트리밍 경로 중 무엇을 표준으로 삼을지 결정 필요.
