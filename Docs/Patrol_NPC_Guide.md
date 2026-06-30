# 순찰 NPC 만들기 가이드

> 작성: 2026-06-30 · 대상: 허브 맵 NPC에 "걸어다니는 순찰" 추가
> 관련: `Hub/LastFPSNPCBase.h` (`AActor` 기반 상호작용 NPC) · `Hub/ILastFPSInteractable.h` · 홀드 인터랙션(`LastFPSPlayerController`)

---

## 0. 결론 먼저 (어느 방식?)

| 방식 | 작업량 | 이동 품질 | 추천 |
|---|---|---|---|
| **A. 웨이포인트 이동** (AI/Nav 없음) | 0.5~1일 | 정해진 경로만, 장애물 회피 X | ✅ **포폴용 추천** |
| **B. AIController + NavMesh + BT** | 1~2일 | 진짜 길찾기·회피 | 시간 여유 / AI 어필 원할 때 |

> **핵심 전제**: 현재 `ALastFPSNPCBase`는 `AActor`라 **이동 능력(MovementComponent)도 컨트롤러도 없다.** 그래서 둘 다 "이동을 어떻게 줄까"가 1순위 과제.
> **단축 레버**: NPC가 **플레이어와 같은 스켈레톤** + **로코모션 AnimBP 재사용** 가능하면 애니메이션 작업이 거의 사라져 A는 반나절 안쪽.

---

## 공통 1 — 메쉬 / 애니메이션 준비

순찰은 "이동 + 걷는 모션"이 세트다. idle만으론 미끄러지듯 움직여 어색하다.

1. **SkeletalMeshComponent** 가 NPC에 있어야 함 (메쉬 교체 시 이미 추가했을 것)
2. **walk 애니메이션** 확보 — 없으면 구해서 NPC 스켈레톤으로 **리타게팅** (여기서 시간 제일 많이 듦)
3. **로코모션 AnimBP**
   - `Speed`(float) 변수 → idle ↔ walk **블렌드스페이스**(1D, 0~최대속도)
   - `AnimGraph`: 블렌드스페이스 출력
   - `EventGraph`: 매 프레임 `Speed` 갱신 (아래 방식별로 소스가 다름)

> 플레이어용 로코모션 AnimBP가 이미 있으면 **그대로 재사용**하고 NPC 메쉬에만 할당 → 2·3번 생략.

---

## 방식 A — 웨이포인트 이동 (추천)

AI/NavMesh 없이, 정해둔 점들을 따라 직접 이동시킨다. 평평한 허브에 충분.

### A-1. PatrolComponent 추가 (재사용 가능)

기존 NPC(`AActor`)를 건드리지 않고 **컴포넌트로 이동 능력만 부착**한다. 정지 NPC엔 안 붙이면 그만.

`Hub/LastFPSPatrolComponent.h` (스케치)
```cpp
UCLASS(ClassGroup=(LastFPS), meta=(BlueprintSpawnableComponent))
class LASTFPS_API ULastFPSPatrolComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    ULastFPSPatrolComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick, FActorComponentTickFunction*) override;

    /** 순찰 지점(월드 좌표). 레벨에서 TargetPoint 액터를 찍어 채우거나, 상대좌표로 입력 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Patrol")
    TArray<FVector> Waypoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Patrol", meta=(ClampMin="10"))
    float MoveSpeed = 150.f;          // cm/s

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Patrol")
    float AcceptanceRadius = 20.f;    // 도착 판정

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Patrol")
    float TurnRateDeg = 360.f;        // 초당 회전 각

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Patrol")
    float WaitAtPointSec = 1.0f;      // 지점 도착 후 대기

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Patrol")
    bool bPingPong = false;           // true=왕복, false=순환

    /** AnimBP가 읽을 현재 속도(걷기 블렌드용) */
    UPROPERTY(BlueprintReadOnly, Category="Patrol")
    float CurrentSpeed = 0.f;

    /** 외부에서 일시정지 (대화 중 멈추기 등) */
    UFUNCTION(BlueprintCallable, Category="Patrol")
    void SetPatrolPaused(bool bPaused) { bIsPaused = bPaused; }

private:
    int32 CurrentIndex = 0;
    int32 Direction = 1;              // ping-pong용
    float WaitTimer = 0.f;
    bool bIsPaused = false;
};
```

`Hub/LastFPSPatrolComponent.cpp` (핵심 Tick 로직 스케치)
```cpp
void ULastFPSPatrolComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    AActor* Owner = GetOwner();
    if (bIsPaused || !Owner || Waypoints.Num() < 2) { CurrentSpeed = 0.f; return; }

    // 도착 후 대기
    if (WaitTimer > 0.f) { WaitTimer -= DeltaTime; CurrentSpeed = 0.f; return; }

    const FVector Cur = Owner->GetActorLocation();
    FVector Target = Waypoints[CurrentIndex];
    Target.Z = Cur.Z;                                   // 높이 무시(평지 가정)

    const FVector ToTarget = Target - Cur;
    const float Dist = ToTarget.Size();

    if (Dist <= AcceptanceRadius)                       // 도착 → 다음 지점
    {
        WaitTimer = WaitAtPointSec;
        if (bPingPong)
        {
            if (CurrentIndex + Direction < 0 || CurrentIndex + Direction >= Waypoints.Num()) Direction *= -1;
            CurrentIndex += Direction;
        }
        else { CurrentIndex = (CurrentIndex + 1) % Waypoints.Num(); }
        CurrentSpeed = 0.f;
        return;
    }

    const FVector Dir = ToTarget / Dist;
    const float Step = FMath::Min(MoveSpeed * DeltaTime, Dist);
    Owner->SetActorLocation(Cur + Dir * Step, /*bSweep=*/true); // sweep로 벽 관통 완화
    CurrentSpeed = MoveSpeed;

    // 진행 방향으로 부드럽게 회전
    const FRotator Want = Dir.Rotation();
    const FRotator New = FMath::RInterpConstantTo(Owner->GetActorRotation(),
        FRotator(0, Want.Yaw, 0), DeltaTime, TurnRateDeg);
    Owner->SetActorRotation(New);
}
```
> 생성자에서 `PrimaryComponentTick.bCanEverTick = true;` 필수.

### A-2. 애니메이션 연결
- NPC AnimBP의 `EventGraph`에서 **PatrolComponent를 Get → `CurrentSpeed`를 `Speed` 변수에 대입** → 블렌드스페이스가 idle↔walk 전환.
- (대안) 컴포넌트가 직접 AnimInstance 변수를 Set 해도 됨.

### A-3. 레벨 배치
1. 순찰시킬 NPC BP에 `LastFPSPatrolComponent` 추가
2. `Waypoints`에 경로 좌표 입력
   - 쉬운 방법: 레벨에 `TargetPoint` 몇 개 배치 → BP에서 그 위치를 읽어 배열에 채움
   - 또는 디테일 패널에서 상대/절대 좌표 직접 입력
3. `MoveSpeed`/`WaitAtPointSec`/`bPingPong` 취향대로

### A-4. 상호작용과의 공존 (중요)
- **대화 중 걸어가버리면 어색** → NPC 범위 진입 시 순찰 정지 권장.
  - `ALastFPSNPCBase::HandleBeginOverlap`에서 `PatrolComp->SetPatrolPaused(true)`, `HandleEndOverlap`에서 `false`. (또는 상호작용 발동 시 일정시간 정지)
- 홀드 중 NPC가 멀어지는 경우는 이미 PC가 **자동 취소**(`ClearNearestInteractable`) 처리함 → 추가 작업 불필요.

### A 한계
- 장애물 회피 없음(직선 이동). 경로상 충돌 줄이려면 웨이포인트를 벽 피해서 촘촘히.
- 경사/계단 약함(평지 가정). 필요하면 Z를 라인트레이스로 바닥에 스냅.

---

## 방식 B — AIController + NavMesh + BehaviorTree (정석)

진짜 길찾기·회피가 필요하거나 "AI 다룰 줄 안다"를 어필하고 싶을 때.

### B-1. 클래스 구조
- 현재 `ALastFPSNPCBase`는 `AActor` → **이동 불가**. 두 갈래:
  - (권장) **별도 클래스** `ALastFPSPatrolNPC : public ACharacter, public ILastFPSInteractable` 신설.
    - 단, 상호작용/마커 코드를 중복하지 않도록 **공통 로직을 컴포넌트/공유 함수로** 빼서 양쪽이 쓰게.
  - (지양) `ALastFPSNPCBase`를 `ACharacter`로 승격 → 모든 NPC에 CharacterMovement가 붙어 무거워지고 정지 NPC엔 과함.
- `ACharacter`는 `CharacterMovementComponent`가 있어 NavMesh `MoveTo`를 바로 받는다.

### B-2. NavMesh
1. 레벨에 **NavMeshBoundsVolume** 배치 → 허브 바닥을 덮게 스케일
2. `P` 키로 네비 영역(초록) 확인
3. (선택) Project Settings → Navigation Mesh에서 에이전트 반경/높이 조정

### B-3. AIController + 순찰
- `ALastFPSPatrolAIController : public AAIController` 생성, NPC의 `AIControllerClass`로 지정, `AutoPossessAI = PlacedInWorldOrSpawned`.
- **간단 버전(BT 없이)**: 컨트롤러에서 순찰 지점 배열을 돌며 `MoveToLocation` 호출 → `OnMoveCompleted`에서 다음 지점.
```cpp
void ALastFPSPatrolAIController::GoToNext()
{
    if (Points.Num() == 0) return;
    MoveToLocation(Points[Index]->GetActorLocation(), AcceptanceRadius);
}
void ALastFPSPatrolAIController::OnMoveCompleted(FAIRequestID, const FPathFollowingResult&)
{
    Index = (Index + 1) % Points.Num();
    GetWorld()->GetTimerManager().SetTimer(WaitTimer, this, &ThisClass::GoToNext, WaitSec, false);
}
```
- **BehaviorTree 버전**: BB에 `TargetPoint`, BT는 `MoveTo`(BB키) → `Wait` → 커스텀 Task로 다음 지점 갱신 → 루프. 블랙보드 + BT 에셋 2개 추가.

### B-4. 애니메이션
- `ACharacter`라 `GetVelocity().Size()`로 **Speed**를 바로 얻음 → 로코모션 AnimBP가 그대로 동작 (방식 A처럼 컴포넌트 안 읽어도 됨).

### B-5. 상호작용 공존
- 대화 시 정지: `AIController->StopMovement()` 또는 BT 일시정지. 범위 진입(overlap)에서 호출.

---

## 테스트 체크리스트

- [ ] NPC가 지점들을 따라 이동하고 끝점에서 순환/왕복
- [ ] 이동 중 **walk 애님**, 정지 시 **idle** (미끄러짐 없음)
- [ ] 진행 방향으로 자연스럽게 회전
- [ ] 플레이어 접근/대화 시 **정지**, 대화 종료 후 재개
- [ ] 홀드 인터랙션 중 NPC가 멀어지면 게이지 **자동 취소** (기존 동작)
- [ ] (B) NavMesh 위에서만 이동, 장애물 회피

## 흔한 함정

- **미끄러짐(슬라이딩)**: walk 애님 없이 idle만 → 반드시 속도 기반 블렌드 필요.
- **벽 관통**: 방식 A는 회피가 없음 → sweep 이동 + 경로 보정. 정밀하면 방식 B.
- **회전 튐**: 목표 방향 즉시 SetRotation 대신 `RInterpConstantTo`로 보간.
- **대화 중 이탈**: overlap에서 순찰 일시정지 안 걸면 NPC가 말하다 걸어감.
- **B에서 안 움직임**: NavMeshBoundsVolume 없음 / `AutoPossessAI` 미설정 / `AIControllerClass` 미지정이 90%.

## 포폴 관점 권고

- 영상 인상 대비 비용을 보면 **방식 A로 1~2명만** 순찰시키고 나머지는 idle이 가성비 최고.
- "AI 시스템 다룬다"를 셀링포인트로 넣고 싶다면 **한 명만 방식 B**로 만들어 NavMesh/BT 스크린샷을 확보.
