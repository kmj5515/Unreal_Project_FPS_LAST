# 순찰 NPC — 정석(프로덕션 표준) 구현 가이드

> 작성: 2026-06-30 · 대상: 허브/필드용 "걸어다니는" NPC를 실무 표준 구조로 제작
> 관련: `Hub/LastFPSNPCBase.h`(현 AActor 상호작용 NPC) · `Hub/ILastFPSInteractable.h` · 홀드 인터랙션(`LastFPSPlayerController`)
> 가벼운 프로토 방식은 다루지 않음 — 이 문서는 **정석만**.

> **구현 상태 (2026-06-30)**: 코드 스캐폴딩 완료 — `Hub/LastFPSPatrolNPC.{h,cpp}`, `Hub/LastFPSPatrolAIController.{h,cpp}`, `Hub/BTTask_SelectNextPatrolPoint.{h,cpp}`, `LastFPS.Build.cs`에 `NavigationSystem` 추가.
> **단, 1단계(상호작용 컴포넌트 추출)는 보류**하고 `ALastFPSPatrolNPC`를 **자기완결형**(구체·마커·OnInteract 자체 보유)으로 작성함.
> 이유: NPCBase의 UPROPERTY(DisplayName/DialogueRow/ScreenToOpen 등)를 컴포넌트로 옮기면 **이미 설정된 NPC BP/인스턴스 데이터가 초기화**됨 → 작동 중인 정지 NPC를 깨뜨림. 정지 NPC 설정이 안정된 뒤 공통 컴포넌트로 통합하는 게 안전. (그 전까진 OnInteract 등 ~20줄이 NPCBase와 중복)
> 남은 것: 빌드 → 레벨 NavMesh/TargetPoint → BB/BT/AnimBP 에셋 작성 (아래 4~9 단계).

---

## 0. 정석 구조 한눈에

```
[Pawn = 몸]   ALastFPSPatrolNPC : ACharacter
                 ├─ CharacterMovementComponent  (이동·회전·중력)
                 ├─ SkeletalMesh + 로코모션 AnimBP (idle↔walk 스테이트머신)
                 ├─ InteractionComponent        (구체·마커·인터페이스 — 기존과 공유)
                 └─ (선택) AIPerceptionComponent

[Brain = 뇌] ALastFPSPatrolAIController : AAIController
                 ├─ Blackboard  (상태)
                 └─ BehaviorTree (MoveTo → Wait → 다음 지점)

[World]      NavMeshBoundsVolume + 순찰 지점(TargetPoint 액터들)
```

**핵심 원칙**
- **몸/뇌 분리**: 이동·애님은 Pawn, 의사결정은 AIController. (실무 표준)
- **데이터·컴포넌트화**: 상호작용은 컴포넌트로 재사용, 순찰 지점은 레벨 데이터.
- **NPC 중요도에 맞는 복잡도**: 이 정석은 "움직이고 반응하는" NPC용. 가만히 선 앰비언트 NPC엔 과함(그건 idle AnimBP면 충분).

---

## 1. 상호작용 재사용 — 컴포넌트로 추출 (선행 권장)

현재 상호작용(구체 감지 + 마커 + `ILastFPSInteractable`)이 `ALastFPSNPCBase`(AActor)에 박혀 있다. 순찰 NPC는 `ACharacter`라 **상속으로 못 받는다.** 정석은 **중복 대신 컴포넌트로 추출**:

`Hub/LastFPSInteractionComponent.h` (개념)
```cpp
UCLASS(ClassGroup=(LastFPS), meta=(BlueprintSpawnableComponent))
class LASTFPS_API ULastFPSInteractionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    // 기존 NPCBase의 상호작용 책임을 그대로 이전:
    //  - USphereComponent 감지 (BeginPlay에서 소유자에 attach/생성)
    //  - UWidgetComponent 마커 + ULastFPSNPCMarkerWidget 제어
    //  - PC SetNearestInteractable / Clear
    //  - ScreenToOpen / DialogueRow / Notice 폴백 (OnInteract)
    //  - SetInteractionProgress 포워딩(홀드 게이지)
    UPROPERTY(EditAnywhere, Category="Interaction") FText DisplayName;
    UPROPERTY(EditAnywhere, Category="Interaction") FText NPCRole;
    UPROPERTY(EditAnywhere, Category="Interaction", meta=(Categories="UI.Screen")) FGameplayTag ScreenToOpen;
    UPROPERTY(EditAnywhere, Category="Interaction", meta=(RowType="/Script/LastFPS.LastFPSDialogueData")) FDataTableRowHandle DialogueRow;
    // ...
    void HandleInteract(APlayerController* PC);
    void SetProgress(float P); // 홀드 게이지
};
```
- **`ALastFPSNPCBase`(정지 NPC)도 이 컴포넌트를 쓰도록 리팩터링** → 정지/순찰 NPC가 같은 상호작용 코드 공유.
- 인터페이스(`ILastFPSInteractable`)는 액터가 구현하되 본문은 컴포넌트에 위임.

> 시간이 없으면 1단계는 건너뛰고 순찰 NPC에 상호작용을 임시 중복 구현해도 되지만, **정석은 컴포넌트 공유**다.

---

## 2. Pawn — ALastFPSPatrolNPC (`ACharacter`)

`Hub/LastFPSPatrolNPC.h` (스케치)
```cpp
UCLASS(Blueprintable)
class LASTFPS_API ALastFPSPatrolNPC : public ACharacter, public ILastFPSInteractable
{
    GENERATED_BODY()
public:
    ALastFPSPatrolNPC();

    // ILastFPSInteractable — 컴포넌트에 위임
    virtual void Interact_Implementation(APlayerController* PC) override;
    virtual FText GetInteractionLabel_Implementation() const override;
    virtual void SetInteractionProgress_Implementation(float Progress) override;

    /** 순찰 지점 (레벨의 TargetPoint들). AIController가 읽는다. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Patrol")
    TArray<TObjectPtr<AActor>> PatrolPoints;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patrol")
    float WaitAtPointSec = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patrol")
    bool bLoop = true;             // true=순환, false=왕복

protected:
    UPROPERTY(VisibleAnywhere) TObjectPtr<class ULastFPSInteractionComponent> Interaction;
};
```
생성자 포인트:
- `GetCharacterMovement()->MaxWalkSpeed = 150.f;` (산책 속도)
- `AIControllerClass = ALastFPSPatrolAIController::StaticClass();`
- `AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;`
- `bUseControllerRotationYaw = false;` + `GetCharacterMovement()->bOrientRotationToMovement = true;` → 이동 방향으로 자연 회전.
- `Interaction = CreateDefaultSubobject<ULastFPSInteractionComponent>(...)`.

---

## 3. NavMesh (월드)

1. 레벨에 **NavMeshBoundsVolume** 배치 → 허브 바닥을 덮게 스케일.
2. `P` 키로 네비 영역(초록) 확인. 바닥이 초록으로 덮여야 `MoveTo` 작동.
3. (선택) Project Settings → Navigation Mesh에서 에이전트 반경/높이 조정. RecastNavMesh 생성 확인.

---

## 4. Blackboard (BB_PatrolNPC)

| 키 | 타입 | 용도 |
|---|---|---|
| `TargetLocation` | Vector | MoveTo 목표 |
| `WaitTime` | Float | 지점 도착 후 대기 |
| `bIsInteracting` | Bool | 대화/상호작용 중 → 순찰 중단 |
| `TargetActor` | Object | (선택) Perception으로 감지한 플레이어 |

---

## 5. AIController — ALastFPSPatrolAIController (`AAIController`)

`Hub/LastFPSPatrolAIController.h/.cpp` (핵심)
```cpp
void ALastFPSPatrolAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    if (BehaviorTreeAsset)                 // EditDefaultsOnly UBehaviorTree*
    {
        RunBehaviorTree(BehaviorTreeAsset); // Blackboard도 함께 초기화됨
    }
}
```
- `BehaviorTreeAsset`을 BP 디폴트에서 `BT_PatrolNPC`로 지정.
- (선택) `AIPerceptionComponent` + Sight Config → `OnTargetPerceptionUpdated`에서 `BB->SetValueAsObject("TargetActor", Player)`.

---

## 6. BTTask — 다음 순찰 지점 선택 (C++)

`AI/BTTask_SelectNextPatrolPoint.cpp` (개념)
```cpp
EBTNodeResult::Type UBTTask_SelectNextPatrolPoint::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp, uint8*)
{
    AAIController* AICon = OwnerComp.GetAIOwner();
    auto* NPC = Cast<ALastFPSPatrolNPC>(AICon ? AICon->GetPawn() : nullptr);
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!NPC || BB == nullptr || NPC->PatrolPoints.Num() == 0)
        return EBTNodeResult::Failed;

    AActor* Point = NPC->PatrolPoints[CurrentIndex];        // 멤버 인덱스
    BB->SetValueAsVector(TEXT("TargetLocation"), Point->GetActorLocation());
    BB->SetValueAsFloat(TEXT("WaitTime"), NPC->WaitAtPointSec);

    // 다음 인덱스 (순환/왕복)
    if (NPC->bLoop) CurrentIndex = (CurrentIndex + 1) % NPC->PatrolPoints.Num();
    else { /* ping-pong: Direction 뒤집기 */ }
    return EBTNodeResult::Succeeded;
}
```
> 인덱스를 NPC가 아니라 BB(Int 키)나 Task 인스턴스 메모리에 둬도 됨.

---

## 7. Behavior Tree (BT_PatrolNPC)

```
Root
└─ Selector
   ├─ [상호작용 중] Blackboard Decorator: bIsInteracting == true
   │     └─ Wait(0.5) 반복  (= 제자리 정지, idle 유지)
   └─ [순찰] Sequence   ← Blackboard Decorator: bIsInteracting == false (Observer aborts: Self)
         ├─ BTTask_SelectNextPatrolPoint
         ├─ MoveTo  (Blackboard Key = TargetLocation, Acceptance 50)
         └─ Wait    (Blackboard Key = WaitTime)
```
- `bIsInteracting` 데코레이터에 **Observer aborts = Self** → 대화 시작 시 MoveTo가 즉시 중단되고 정지 브랜치로.
- MoveTo가 NavMesh 길찾기·장애물 회피를 자동 처리(방식 A와 결정적 차이).

---

## 8. 로코모션 AnimBP (ABP_PatrolNPC)

- **스테이트 머신**: `Idle` ↔ `Walk` (전이 조건 = `Speed > 10`)
- `EventGraph`: `Speed = GetVelocity().Size()` (Character라 **속도가 자동으로 잡힘** — 좌표 직접 이동과 달리 수동 계산 불필요)
- `Walk` 상태 = idle/walk **블렌드스페이스**(1D, 0~MaxWalkSpeed) 또는 walk 단일 + 속도 보간
- 플레이어용 로코모션 AnimBP가 같은 스켈레톤이면 **재사용** 권장

---

## 9. 상호작용 통합 — 대화 중 정지/재개

순찰 NPC는 움직이므로, 대화 중 걸어가버리지 않게 **BB로 제어**:

- **시작**: 플레이어가 상호작용(또는 범위 진입) → `InteractionComponent`가 AIController의 Blackboard에 `bIsInteracting = true` 세팅 → BT가 MoveTo 중단(데코레이터 Self abort) → 정지.
  - 추가로 `AICon->StopMovement()` + 플레이어를 향해 회전(FocusActor)하면 자연스러움.
- **종료**: 대화 닫힘/범위 이탈 → `bIsInteracting = false` → 순찰 재개.
- 홀드 게이지가 도중 끊기는 건 기존 PC 로직(`ClearNearestInteractable`)이 이미 취소 처리.

```cpp
// InteractionComponent에서
if (auto* AICon = Cast<AAIController>(OwnerPawn->GetController()))
    if (auto* BB = AICon->GetBlackboardComponent())
        BB->SetValueAsBool(TEXT("bIsInteracting"), bActive);
```

---

## 10. 모듈 의존성

`.Build.cs`에 추가 필요: **`AIModule`**, **`NavigationSystem`**, **`GameplayTasks`** (BT/BB/AIController/Perception용).

---

## 테스트 체크리스트

- [ ] NavMesh 초록 영역 위에서만 이동 (P 키 확인)
- [ ] 지점들을 순회하며 도착 시 대기 → 다음
- [ ] 이동 중 **walk**, 정지 시 **idle** (미끄러짐 없음)
- [ ] 장애물을 **돌아서** 감 (방식 A엔 없던 회피)
- [ ] 대화 시작 → 즉시 정지·플레이어 응시, 종료 → 재개
- [ ] 홀드 인터랙션 중 NPC 정지(또는 멀어지면 게이지 취소)

## 흔한 함정

- **안 움직임 90%**: NavMeshBoundsVolume 없음 / `AutoPossessAI` 미설정 / `AIControllerClass` 미지정 / `BehaviorTreeAsset` 미지정.
- **회전 안 함**: `bOrientRotationToMovement=true` + `bUseControllerRotationYaw=false` 누락.
- **빌드 에러**: `.Build.cs`에 `AIModule`/`NavigationSystem` 누락.
- **대화 중 걸어감**: `bIsInteracting` 데코레이터 Observer aborts=Self 누락.
- **상호작용 중복 코드**: 컴포넌트로 추출 안 하고 AActor NPC와 따로 구현하면 유지보수 지옥 → 1단계 권장 이유.

---

## 작업 순서 요약

1. (선행) 상호작용을 `ULastFPSInteractionComponent`로 추출, 기존 NPCBase도 전환
2. `ALastFPSPatrolNPC`(ACharacter) + `ALastFPSPatrolAIController` + `BTTask_SelectNextPatrolPoint` 코드
3. `.Build.cs`에 AIModule/NavigationSystem 추가 → 빌드
4. 레벨: NavMeshBoundsVolume + TargetPoint 배치, NPC에 PatrolPoints 연결
5. BB_PatrolNPC + BT_PatrolNPC 작성, AIController에 지정
6. ABP_PatrolNPC(idle↔walk) 작성/재사용, 메시에 지정
7. 대화 중 정지(bIsInteracting) 배선 → 테스트
