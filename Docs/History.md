# LastFPS — 구현 기록 & 동작 원리

> 기능을 추가할 때마다 여기에 정리. "왜 이렇게 동작하는가"에 집중.

---

## 목차

1. [입력 시스템](#1-입력-시스템)
2. [이동 & 달리기](#2-이동--달리기)
3. [카메라 & ADS](#3-카메라--ads)
4. [점프 & 더블점프](#4-점프--더블점프)
5. [GAS AttributeSet](#5-gas-attributeset)
6. [무기 시스템](#6-무기-시스템)

---

## 1. 입력 시스템

**관련 파일:** `Input/LastFPSInputConfig.h`, `Character/LastFPSHero.cpp`

Enhanced Input + GameplayTag 기반으로 InputAction과 바인딩을 분리한다.

```
IA_Move ──────┐
IA_Look ──────┤  InputMappingContext (IMC)
IA_Sprint ────┤        ↓
IA_ADS ───────┤  ULastFPSInputConfig (DataAsset)
IA_Jump ──────┘        ↓
                 FindNativeInputActionByTag("InputTag.XXX")
                        ↓
                 ALastFPSHero::SetupPlayerInputComponent 에서 바인딩
```

**두 종류의 액션 배열**

| 배열 | 용도 |
|------|------|
| `NativeInputActions` | C++ 함수 직접 호출 (Move, Look, Sprint, ADS, Jump) |
| `AbilityInputActions` | GAS 어빌리티 활성화 (Q/E/F 스킬, 사격) |

InputAction 에셋을 DataAsset에서 GameplayTag로 참조하기 때문에, 키 변경 시 C++ 재컴파일 없이 에디터에서만 수정하면 된다.

---

## 2. 이동 & 달리기

**관련 파일:** `Character/LastFPSHero.cpp`

### 이동

```cpp
// Controller의 Yaw만 추출해 방향 벡터 계산 → Pitch/Roll 영향 없음
const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
AddMovementInput(ForwardDir, MovementVector.Y);
AddMovementInput(RightDir,   MovementVector.X);
```

`bOrientRotationToMovement = true` 로 캐릭터 메시가 이동 방향을 자동으로 바라본다.  
ADS 진입 시 `bOrientRotationToMovement = false` + `bUseControllerRotationYaw = true` 로 전환해 조준 방향을 고정한다.

### 달리기 (GA_Sprint)

**관련 파일:** `AbilitySystem/Abilities/GA_Sprint.h/.cpp`, `Character/LastFPSCharacterBase.cpp`

`StartSprint()` → `TryActivateAbilitiesByTag("Ability.Sprint")`, `StopSprint()` → `CancelAbilities`.  
속도 변경은 MoveSpeed 어트리뷰트를 경유해 CMC에 반영 → 멀티플레이어 복제 안전.

```
Left Shift 누름 → TryActivateAbilitiesByTag("Ability.Sprint")
  → GA_Sprint::ActivateAbility()
      ├── GE_SprintSpeed 적용 → MoveSpeed +300
      │       ↓
      │   CharacterBase::OnMoveSpeedChanged(700) → CMC MaxWalkSpeed = 700
      ├── GE_SprintStaminaDrain 적용 (Stamina −20/s)
      └── Stamina 변경 델리게이트 등록

Left Shift 뗌 → CancelAbilities → EndAbility 호출
Stamina = 0   → OnStaminaChanged → EndAbility 자동 호출

  → GA_Sprint::EndAbility()
      ├── GE_SprintSpeed 제거 → MoveSpeed −300
      │       ↓
      │   OnMoveSpeedChanged(400) → CMC MaxWalkSpeed = 400
      ├── GE_SprintStaminaDrain 제거
      └── 델리게이트 해제
```

**MoveSpeed → CMC 연결 (CharacterBase)**

```cpp
// InitAbilitySystem() 에서 바인딩
ASC->GetGameplayAttributeValueChangeDelegate(GetMoveSpeedAttribute())
    .AddUObject(this, &ALastFPSCharacterBase::OnMoveSpeedChanged);

void OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
    GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
}
```

GAS GE가 MoveSpeed 어트리뷰트를 바꾸면 이 콜백이 서버/클라 모두에서 즉시 실행된다.

**스태미나 자동 회복**

`BP_GE_StaminaRegen`을 Hero BP `DefaultEffects`에 추가해 항시 적용.  
스프린트 중: 드레인(−20/s) + 리젠(+5/s) = 실질 **−15/s**  
정지 중: 리젠(+5/s)만 작동 → 천천히 회복

| 설정 | 값 | 비고 |
|------|----|------|
| `InstancingPolicy` | InstancedPerActor | 인스턴스 상태 보존 필요 |
| `NetExecutionPolicy` | LocalPredicted | 클라이언트 선반영 |
| MoveSpeed 기본값 | 400 | AttributeSet 초기값, CMC 기본값과 일치 |

**에디터 설정**

| 에셋 | Duration | Period | Modifier |
|------|----------|--------|----------|
| `BP_GE_SprintSpeed` | Infinite | — | MoveSpeed Add **+300** |
| `BP_GE_SprintStaminaDrain` | Infinite | 0.1s | Stamina Add **−2.0** (= −20/s) |
| `BP_GE_StaminaRegen` | Infinite | 0.2s | Stamina Add **+1.0** (= +5/s) |

- `BP_GA_Sprint` (Parent: GA_Sprint) → SprintSpeedEffect, StaminaDrainEffect 할당
- Hero BP `DefaultAbilities`에 `BP_GA_Sprint` 추가
- Hero BP `DefaultEffects`에 `BP_GE_StaminaRegen` 추가
- GameplayTag `Ability.Sprint` 등록 필요

---

## 3. 카메라 & ADS

**관련 파일:** `Character/LastFPSHero.cpp` — `TickCameraInterp`, `StartADS`, `StopADS`

### 기본 구조

```
CameraBoom (SpringArm)
  └── FollowCamera (Camera)
```

SpringArm의 `TargetArmLength`, `SocketOffset` 과 Camera의 `FieldOfView` 세 값을 보간해서 ADS 전환을 표현한다.

### 보간 흐름

```
StartADS() 호출
  → TargetArmLength / TargetSocketOffset / TargetFOV 목표값 변경
  → Tick마다 FInterpTo / VInterpTo 로 현재값을 목표값에 수렴
  → StopADS() 호출 시 기본값으로 복귀
```

`ADSInterpSpeed` 값이 높을수록 전환이 빠르다. 기본 10.f.

| 상태 | ArmLength | SocketOffset | FOV |
|------|-----------|--------------|-----|
| 기본 | 300 | (0, 60, 20) | 90° |
| ADS  | 120 | (0, 55, 25) | 75° |

---

## 4. 점프 & 더블점프

**관련 파일:** `Character/LastFPSHero.cpp` — `StartJump`, `StopJump`  
**관련 설정:** `JumpMaxCount = 2` (생성자)

### 동작 원리

UE5 `ACharacter` 에 내장된 멀티점프 시스템을 그대로 사용한다.

```
지상에서 Space 누름
  → Jump() 호출 → JumpCurrentCount = 1
  → 공중에서 Space 누름
  → JumpCurrentCount < JumpMaxCount(2) 이므로 2차 점프 허용
  → JumpCurrentCount = 2
  → 착지 시 Landed() 에서 JumpCurrentCount 자동 리셋 → 0
```

| 변수 | 값 | 역할 |
|------|----|------|
| `JumpMaxCount` | 2 | 허용 점프 횟수 |
| `JumpCurrentCount` | 0~2 (자동 관리) | 현재 누적 점프 수 |
| `JumpZVelocity` | 700 | 점프 초속 (상향) |
| `AirControl` | 0.4 | 공중 이동 제어력 (0=없음, 1=지상과 동일) |
| `GravityScale` | 1.5 | 중력 배율 (높을수록 빠르게 떨어짐) |

`StartJump()` → `Jump()`, `StopJump()` → `StopJumping()` 으로 래핑만 함.  
추후 더블점프 파티클/사운드 피드백을 여기에 추가할 예정.

---

## 5. GAS AttributeSet

**관련 파일:** `AbilitySystem/AttributeSets/LastFPSAttributeSet.h`

### 어트리뷰트 목록

| 어트리뷰트 | 용도 | 복제 |
|-----------|------|------|
| Health / MaxHealth | 체력 | O |
| Stamina / MaxStamina | 달리기·특수이동 자원 | O |
| UltimateGauge / MaxUltimateGauge | 궁극기 게이지 | O |
| AttackDamage | 기본 공격력 | O |
| Defense | 피해 감소 | O |
| MoveSpeed | 이동속도 배율 | O |
| Damage | 피해 임시 저장 (Meta) | X |

`Damage` 는 `GE_Damage` 적용 시 잠깐 저장 후 `Health` 감산에 사용하는 Meta Attribute. 복제하지 않음.

### 초기화 흐름

```
PossessedBy (서버) / OnRep_PlayerState (클라이언트)
  → InitAbilitySystem()
  → ApplyDefaultEffects()   ← GE_Init 등으로 초기 스탯 세팅
  → GiveDefaultAbilities()  ← DefaultAbilities 배열 순회 부여
```

---

*Last updated: 2026-05-01*
