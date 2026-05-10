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
7. [GAS 네트워크 & Prediction](#7-gas-네트워크--prediction)
8. [HUD 시스템](#8-hud-시스템)

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

`bUseControllerRotationYaw = true` (항상) — 캐릭터 몸이 컨트롤러 Yaw를 따라 회전하며, 이 값이 다른 클라이언트에 복제된다.  
`bOrientRotationToMovement = false` — 이동 방향 자동 회전을 끄고 스트라이프 애니메이션으로 대신 처리한다.  
두 플래그는 동시에 활성화하면 충돌하므로 반드시 하나만 사용해야 한다.

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

> **회전 설정은 ADS와 무관하다.** `bUseControllerRotationYaw = true`가 항상 켜져 있으므로 ADS 진입/해제 시 회전 플래그를 토글하지 않는다. 이전 구현에서 ADS 토글로 회전 방식을 전환하던 코드는 멀티플레이어 회전 동기화 문제를 일으켜 제거됐다.

| 상태 | ArmLength | SocketOffset | FOV |
|------|-----------|--------------|-----|
| 기본 | 300 | (0, 60, 20) | 90° |
| ADS  | 120 | (0, 55, 25) | 75° |

---

## 4. 점프 & 더블점프

**관련 파일:** `AbilitySystem/Abilities/GA_Jump.h/.cpp`, `Character/LastFPSHero.cpp`  
**관련 설정:** `JumpMaxCount = 2` (생성자)

### 동작 원리

UE5 `ACharacter` 내장 멀티점프 시스템 위에 GAS 어빌리티(`GA_Jump`)를 래핑해 예측·태그 차단을 통합한다.

```
Space 누름
  → TryActivateAbilitiesByTag("Ability.Jump")
  → GA_Jump::CanActivateAbility()
      └── Character->CanJump() 위임 (JumpCurrentCount / 공중 상태 CMC가 검사)
  → GA_Jump::ActivateAbility()
      └── Character->Jump() 호출
      └── EndAbility() 즉시 (one-shot)

Space 뗌
  → LastFPSHero::StopJump() → StopJumping() 직접 호출
     (GAS 경유 X — 버튼 해제 시 가변 점프높이 컷오프 보장)
```

**더블점프 카운트 흐름**
```
지상: JumpCurrentCount = 0
  → Jump() → JumpCurrentCount = 1
공중: JumpCurrentCount(1) < JumpMaxCount(2) → 2차 허용
  → Jump() → JumpCurrentCount = 2
착지: Landed() → JumpCurrentCount 자동 리셋 = 0
```

| 변수 | 값 | 역할 |
|------|----|------|
| `JumpMaxCount` | 2 | 허용 점프 횟수 |
| `JumpZVelocity` | 700 | 점프 초속 (상향) |
| `AirControl` | 0.4 | 공중 이동 제어력 |
| `GravityScale` | 1.5 | 중력 배율 |
| `InstancingPolicy` | NonInstanced | one-shot, 인스턴스 상태 없음 |
| `NetExecutionPolicy` | LocalPredicted | 클라이언트 즉시 점프, CMC가 물리 예측 |

**에디터 설정**
- `BP_GA_Jump` (Parent: `GA_Jump`) → Hero BP `DefaultAbilities`에 추가
- GameplayTag `Ability.Jump` 등록

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

---

## 6. 무기 시스템

### 구조

```
ALastFPSHero
  └── UWeaponComponent
        ├── WeaponMesh (SkeletalMeshComponent) — BeginPlay에서 동적 생성, WeaponSocket 부착
        ├── WeaponSkeletalMesh                 — BP에서 할당할 메시 에셋
        ├── ProjectileClass                    — BP_Projectile 할당 (VFX 전용)
        ├── FireSound (USoundBase)             — 발사음
        ├── MuzzleFlashEffect (UParticleSystem) — 총구 Cascade 파티클
        ├── CurrentHeat (Replicated, float)    — 현재 열량
        ├── bIsOverheated (Replicated, bool)   — 오버히트 상태
        ├── MaxHeat / HeatPerShot / CooldownRate / FireRate
        └── GetMuzzleTransform() / PlayFireEffects()

ALastFPSProjectile  ← VFX 전용, 데미지 없음
  ├── CollisionComp (BoxComponent)             — 2.5×1×1 cm, NoCollision (충돌 비활성화)
  ├── ProjectileMovementComponent              — 30000 cm/s (300 m/s), 중력 0
  ├── TrailParticle (UParticleSystemComponent) — 비행 중 Cascade 트레일
  └── TrailEffect (UParticleSystem)            — BP에서 할당할 트레일 에셋
```

### WeaponComponent — 오버히트 시스템

**관련 파일:** `Character/Components/WeaponComponent.h/.cpp`

탄약 대신 **열 게이지**로 발사를 제한한다. 장전 없음.

```
발사 1회 → AddHeat() → CurrentHeat += HeatPerShot
                          ↓
               CurrentHeat >= MaxHeat?
                  YES → bIsOverheated = true → CanFire() = false → 발사 중단
                  NO  → 계속 발사 가능

Tick (서버 전용)
  → CurrentHeat -= CooldownRate × DeltaTime
  → bIsOverheated && CurrentHeat <= 0
      → bIsOverheated = false → 발사 재개
```

| 프로퍼티 | 기본값 | 의미 |
|---------|--------|------|
| `HeatPerShot` | 10 | 발사 1회당 열량 (기본 10발에 오버히트) |
| `MaxHeat` | 100 | 최대 게이지 |
| `CooldownRate` | 20/s | 오버히트 후 5초 대기 |
| `FireRate` | 0.1s | 연사 간격 |

- `CurrentHeat`, `bIsOverheated` 모두 `DOREPLIFETIME` 복제 → HUD에서 `GetCurrentHeat() / GetMaxHeat()` 비율로 게이지 바 표시

### WeaponComponent — 발사 이펙트

`PlayFireEffects()` — 서버/클라 모두 호출 (코스메틱):

```cpp
UGameplayStatics::SpawnSoundAttached(FireSound, WeaponMesh, MuzzleSocketName);
UGameplayStatics::SpawnEmitterAttached(MuzzleFlashEffect, WeaponMesh, MuzzleSocketName, ...);
```

소켓(`MuzzleFlash`)이 없으면 경고 없이 무시됨. 무기 Skeleton에 `MuzzleFlash` 소켓 추가 필요.

### GA_BasicShoot — Hitscan + LocalFire/ServerFire

**관련 파일:** `AbilitySystem/Abilities/GA_BasicShoot.h/.cpp`

발사 판정은 **LineTrace(Hitscan)** 로 처리하고, 투사체는 순수 VFX(탄도 트레일)로만 사용한다.

```
LMB 누름 → TryActivateAbilitiesByTag("Ability.Fire")
  → GA_BasicShoot::ActivateAbility()
      ├── CommitAbility()
      ├── WeaponComponent::CanFire() 확인 (오버히트 여부)
      ├── Fire() 즉시 호출
      └── bIsAutoFire == true → 타이머로 FireRate마다 Fire() 반복

  → Fire()
      ├── [IsLocallyControlled] LocalFire()
      │     └── PlayFireEffects() — 사운드 + 머즐플래시 즉시 재생 (레이턴시 0, 클라이언트 예측)
      ├── [HasAuthority] ServerFire()
      │     ├── GetPlayerViewPoint() → 카메라 조준 방향 획득
      │     ├── LineTraceSingleByObjectType(WorldStatic+WorldDynamic+Pawn+PhysicsBody)
      │     ├── VFX 투사체 스폰 (서버 스폰 → bReplicates로 모든 클라이언트 복제)
      │     ├── 히트 대상 ASC에 DamageEffectClass 적용
      │     └── Client_NotifyHitMarker() — 발사자 클라이언트에만 히트마커 표시
      └── AddHeat() → 오버히트 도달 시 EndAbility

LMB 뗌 → CancelAbilities → EndAbility (타이머 해제)
```

**LocalFire / ServerFire 분리 이유**

| 역할 | 실행 위치 | 내용 |
|------|---------|------|
| `LocalFire` | `IsLocallyControlled()` | 사운드·머즐플래시 즉시 재생 — 레이턴시 없는 클라이언트 예측 |
| `ServerFire` | `HasAuthority()` | LineTrace 히트 판정 → 데미지 GE → VFX 투사체 스폰 |

`LocalPredicted` 어빌리티는 소유 클라이언트와 서버 양쪽에서 모두 실행된다. `IsLocallyControlled()`는 클라이언트(+리슨서버 호스트)에서 참이고, `HasAuthority()`는 서버에서만 참이므로 두 조건으로 역할을 나눌 수 있다.

| 설정 | 값 | 비고 |
|------|----|------|
| `InstancingPolicy` | InstancedPerActor | 타이머 상태 보존 필요 |
| `NetExecutionPolicy` | LocalPredicted | 클라이언트 선반영 |
| `bIsAutoFire` | false (기본) | BP에서 연사로 변경 가능 |
| `DamageEffectClass` | (BP에서 할당) | 기존 투사체의 DamageEffect를 이쪽으로 이전 |

### ALastFPSProjectile — VFX 전용

**관련 파일:** `Weapons/LastFPSProjectile.h/.cpp`

데미지 로직이 완전히 제거된 순수 시각 연출용 액터.

**충돌 없음 (`NoCollision`)**  
- `BoxComponent` (2.5×1×1 cm) — 루트 컴포넌트 역할만, 충돌 비활성화
- `OnHit` 바인딩 없음 — 아무것도 막지 않음

**이동**
- 속도: 30,000 cm/s (300 m/s) — 눈으로 트레일을 볼 수 있는 속도
- 중력: 0 — 데미지가 LineTrace에서 나오므로 중력 시뮬레이션 불필요
- 수명: 1.5초 (최대 비행 거리 약 450 m)

**복제**  
`bReplicates = true` — 서버에서 스폰 후 모든 클라이언트에 자동 복제됨

**비주얼**
- `UParticleSystemComponent TrailParticle` — `BeginPlay`에서 `TrailEffect` 에셋을 `SetTemplate`으로 연결, 스폰 즉시 자동 재생

### GE_Damage 세팅

| 항목 | 값 |
|------|----|
| Duration Policy | `Instant` |
| Attribute | `LastFPSAttributeSet.Damage` |
| Modifier Op | `Add` |
| Magnitude | `20.0` |

### 에디터 설정

- `BP_Projectile` — `TrailEffect` = 트레일 파티클 에셋 (DamageEffect 항목 없어짐)
- `BP_GA_BasicShoot` — `DamageEffectClass` = `BP_GE_Damage`, `bIsAutoFire`, Hero BP `DefaultAbilities`에 추가
- `BP_GE_Damage` — Instant, Damage Add 20
- Hero BP → WeaponComponent: `WeaponSkeletalMesh`, `FireSound`, `MuzzleFlashEffect`, `ProjectileClass` 할당
- GameplayTag `Ability.Fire`, `InputTag.Fire` 등록

---

---

## 7. GAS 네트워크 & Prediction

### PlayerState ASC 이전

**관련 파일:** `Game/LastFPSPlayerState.h/.cpp`, `Character/LastFPSCharacterBase.h/.cpp`

GAS 멀티플레이어 표준 구조: ASC와 AttributeSet을 **PlayerState**가 소유한다.

```
Before (Phase 1):
  ALastFPSCharacterBase
    └── UAbilitySystemComponent  ← 캐릭터 소유, 리스폰 시 소멸

After (Phase 3):
  ALastFPSPlayerState
    ├── UAbilitySystemComponent  ← PlayerState 소유, 리스폰 후에도 유지
    └── ULastFPSAttributeSet

  ALastFPSCharacterBase
    └── GetAbilitySystemComponent()
          → GetPlayerState<ALastFPSPlayerState>()->GetAbilitySystemComponent()
    └── AttributeSet (캐시 포인터 — InitAbilitySystem에서 PlayerState에서 가져와 저장)
```

**InitAbilityActorInfo 호출 시점**

| 시점 | 함수 | 대상 |
|------|------|------|
| 서버 빙의 시 | `PossessedBy()` | 서버 |
| 클라이언트 복제 완료 시 | `OnRep_PlayerState()` | 클라이언트 |

```cpp
// Owner = PlayerState, Avatar = Character
ASC->InitAbilityActorInfo(PS, this);
```

Owner/Avatar 분리 이유: GAS가 어빌리티 활성화 권한(Owner)과 물리적 실체(Avatar)를 구분해 리스폰 후에도 어빌리티/어트리뷰트 상태를 유지할 수 있다.

**에디터 설정**
- `BP_GameMode` → Player State Class = `LastFPSPlayerState`

---

### GAS Prediction

**관련 어빌리티:** `GA_Jump`, `GA_Sprint`, `GA_BasicShoot`

`LocalPredicted` 정책 시 클라이언트가 서버 응답을 기다리지 않고 즉시 실행한다.

| 어빌리티 | 예측 방식 |
|---------|---------|
| `GA_Jump` | `LocalPredicted` + CMC 자체 물리 예측 — 클라에서 즉시 `Jump()`, 서버가 검증 |
| `GA_Sprint` | `LocalPredicted` — `GE_SprintSpeed`(Infinite) 즉시 적용 → `OnMoveSpeedChanged` 콜백으로 CMC 속도 즉시 반영 |
| `GA_BasicShoot` | `LocalPredicted` — `LocalFire`(이펙트)는 `IsLocallyControlled()`, `ServerFire`(LineTrace+데미지+VFX투사체)는 `HasAuthority()` |

**롤백 조건**

서버가 어빌리티를 거부하면 GAS가 클라이언트에서 적용한 GE를 자동 롤백한다:
- Stamina 부족 → Sprint 롤백
- `CanActivateAbility()` false → Jump 롤백
- 오버히트 상태 → Fire 롤백 (Heat는 서버 권한)

---

---

## 8. HUD 시스템

**관련 파일:** `UI/LastFPSHUDWidget.h/.cpp`, `UI/LastFPSHUD.h/.cpp`

### 구조

```
ALastFPSHUD (AHUD 서브클래스)
  └── BeginPlay() → CreateWidget<ULastFPSHUDWidget> → AddToViewport()

ULastFPSHUDWidget (UUserWidget C++ 베이스)
  ├── GAS 어트리뷰트 델리게이트 구독 (Health / Stamina / UltimateGauge)
  ├── WeaponComponent::OnHeatChanged 구독
  └── BlueprintImplementableEvent → WBP_HUD (Blueprint 자식)에서 Progress Bar 연결
```

### 초기화 흐름 & 클라이언트 재시도

멀티플레이어에서 클라이언트의 `NativeConstruct`는 `PlayerState` 복제보다 먼저 호출될 수 있다.

```
NativeConstruct()
  → InitializeHUD()
      ├── 성공 (PlayerState 준비됨)
      │     ├── ASC 어트리뷰트 델리게이트 바인딩 (Health, Stamina, UltGauge)
      │     ├── WeaponComponent::OnHeatChanged 바인딩
      │     └── 현재값으로 바 초기화 호출
      └── 실패 (PlayerState 미준비)
            → SetTimer(0.1s, RetryInitialize, 반복)
            → RetryInitialize() → InitializeHUD() 성공 시 타이머 해제
```

**서버는 즉시 성공, 클라이언트는 PlayerState 복제 완료 후 0.1~0.3초 내에 재시도 성공.**

### 오버히트 게이지 — RepNotify 구조

`CurrentHeat`, `bIsOverheated` 를 단순 `Replicated` → `ReplicatedUsing=OnRep_HeatState`로 변경해 클라이언트에서도 값 도착 즉시 HUD에 알린다.

```
서버: AddHeat() / TickComponent()
  → CurrentHeat, bIsOverheated 변경
  → OnHeatChanged.Broadcast() (서버 HUD 즉시 반영)
  → 복제 전송

클라이언트: 복제 수신
  → OnRep_HeatState()
  → OnHeatChanged.Broadcast() (클라 HUD 반영)
```

### Blueprint 연결 (WBP_HUD)

| 이벤트 | 연결 대상 | 추가 로직 |
|---|---|---|
| `OnHealthChanged(Current, Max)` | `PB_Health.SetPercent(Current/Max)` | 없음 |
| `OnStaminaChanged` | `PB_Stamina.SetPercent` | 없음 |
| `OnUltimateGaugeChanged` | `PB_Ultimate.SetPercent` | 없음 |
| `OnHeatChanged(Current, Max, bOverheated)` | `PB_Heat.SetPercent` | bOverheated로 Fill Color 분기 (주황↔빨강) |

### 에디터 설정

- `BP_HUD` (Parent: `LastFPSHUD`) → `HUD Widget Class` = `WBP_HUD`
- `BP_GameMode` → **HUD Class** = `BP_HUD`
- `WBP_HUD` (Parent: `LastFPSHUDWidget`) → Progress Bar 4개 배치, 각 이벤트 구현

---

*Last updated: 2026-05-11 — 발사 구조 Hitscan 전환(LocalFire+ServerFire), 투사체 VFX 전용 전환(30,000 cm/s·충돌 없음), DamageEffectClass GA_BasicShoot으로 이동, 캐릭터 회전 동기화 수정(bUseControllerRotationYaw 항시 활성화)*
