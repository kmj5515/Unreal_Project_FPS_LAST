#include "Character/LastFPSHero.h"
#include "Character/Components/WeaponComponent.h"
#include "Input/LastFPSInputConfig.h"
#include "UI/LastFPSHUD.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "AbilitySystemComponent.h"

ALastFPSHero::ALastFPSHero()
{
    PrimaryActorTick.bCanEverTick = true;

    // ── 스프링 암 ──────────────────────────────────────────────
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
    CameraBoom->TargetArmLength          = DefaultArmLength;
    CameraBoom->SocketOffset             = DefaultSocketOffset;
    CameraBoom->bUsePawnControlRotation  = true;
    CameraBoom->bEnableCameraLag         = true;
    CameraBoom->CameraLagSpeed           = CameraLagSpeed;
    CameraBoom->bEnableCameraRotationLag = false;

    // ── 카메라 ────────────────────────────────────────────────
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
    FollowCamera->FieldOfView             = DefaultFOV;

    // ── 캐릭터 회전 설정 ──────────────────────────────────────
    // 항상 컨트롤러 Yaw를 따라 몸이 돌아야 다른 클라이언트에 회전이 복제됨
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw   = true;
    bUseControllerRotationRoll  = false;

    // bOrientRotationToMovement와 bUseControllerRotationYaw는 동시에 쓰면 충돌 —
    // 컨트롤러 Yaw 고정 방식이므로 이동 방향 자동 회전은 끔
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->RotationRate              = FRotator(0.f, 500.f, 0.f);
    GetCharacterMovement()->MaxWalkSpeed              = 400.f;
    GetCharacterMovement()->MaxWalkSpeedCrouched      = 200.f;
    GetCharacterMovement()->JumpZVelocity             = 700.f;
    GetCharacterMovement()->AirControl                = 0.4f;
    GetCharacterMovement()->GravityScale              = 1.5f;

    // 더블점프: 1차 점프 + 공중에서 1회 추가 허용
    JumpMaxCount = 2;

    WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));

    // 보간 목표값 초기화
    TargetArmLength    = DefaultArmLength;
    TargetSocketOffset = DefaultSocketOffset;
    TargetFOV          = DefaultFOV;
}

void ALastFPSHero::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (DefaultMappingContext)
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}

void ALastFPSHero::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    TickCameraInterp(DeltaTime);
}

// ── 카메라 보간 ────────────────────────────────────────────────
void ALastFPSHero::TickCameraInterp(float DeltaTime)
{
    CameraBoom->TargetArmLength = FMath::FInterpTo(
        CameraBoom->TargetArmLength, TargetArmLength, DeltaTime, ADSInterpSpeed);

    CameraBoom->SocketOffset = FMath::VInterpTo(
        CameraBoom->SocketOffset, TargetSocketOffset, DeltaTime, ADSInterpSpeed);

    FollowCamera->FieldOfView = FMath::FInterpTo(
        FollowCamera->FieldOfView, TargetFOV, DeltaTime, ADSInterpSpeed);
}

void ALastFPSHero::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EIC || !InputConfig) return;

    TryBindNativeTriggered(EIC, FGameplayTag::RequestGameplayTag("InputTag.Move"), &ALastFPSHero::Move);
    TryBindNativeTriggered(EIC, FGameplayTag::RequestGameplayTag("InputTag.Look"), &ALastFPSHero::Look);

    TryBindNativeStartStop(EIC, FGameplayTag::RequestGameplayTag("InputTag.Sprint"),
        &ALastFPSHero::StartSprint,     &ALastFPSHero::StopSprint);
    TryBindNativeStartStop(EIC, FGameplayTag::RequestGameplayTag("InputTag.ADS"),
        &ALastFPSHero::StartADS,        &ALastFPSHero::StopADS);
    TryBindNativeStartStop(EIC, FGameplayTag::RequestGameplayTag("InputTag.Jump"),
        &ALastFPSHero::StartJump,       &ALastFPSHero::StopJump);
    TryBindNativeStartStop(EIC, FGameplayTag::RequestGameplayTag("InputTag.Fire"),
        &ALastFPSHero::StartFire,       &ALastFPSHero::StopFire);
    TryBindNativeStartStop(EIC, FGameplayTag::RequestGameplayTag("InputTag.Scoreboard"),
        &ALastFPSHero::StartScoreboard, &ALastFPSHero::StopScoreboard);

    TryBindAbilityStart(EIC, FGameplayTag::RequestGameplayTag("InputTag.Skill1"),   &ALastFPSHero::StartSkill1);
    TryBindAbilityStart(EIC, FGameplayTag::RequestGameplayTag("InputTag.Skill2"),   &ALastFPSHero::StartSkill2);
    TryBindAbilityStart(EIC, FGameplayTag::RequestGameplayTag("InputTag.Ultimate"), &ALastFPSHero::StartUltimate);
}

void ALastFPSHero::TryBindNativeTriggered(
    UEnhancedInputComponent* EIC, const FGameplayTag& Tag,
    void(ALastFPSHero::*Func)(const FInputActionValue&))
{
    if (const UInputAction* IA = InputConfig->FindNativeInputActionByTag(Tag))
        EIC->BindAction(IA, ETriggerEvent::Triggered, this, Func);
}

void ALastFPSHero::TryBindNativeStartStop(
    UEnhancedInputComponent* EIC, const FGameplayTag& Tag,
    void(ALastFPSHero::*StartFunc)(), void(ALastFPSHero::*StopFunc)())
{
    if (const UInputAction* IA = InputConfig->FindNativeInputActionByTag(Tag))
    {
        EIC->BindAction(IA, ETriggerEvent::Started,   this, StartFunc);
        EIC->BindAction(IA, ETriggerEvent::Completed, this, StopFunc);
    }
}

void ALastFPSHero::TryBindAbilityStart(
    UEnhancedInputComponent* EIC, const FGameplayTag& Tag,
    void(ALastFPSHero::*Func)())
{
    if (const UInputAction* IA = InputConfig->FindAbilityInputActionByTag(Tag))
        EIC->BindAction(IA, ETriggerEvent::Started, this, Func);
}

// ── 이동 ──────────────────────────────────────────────────────
void ALastFPSHero::Move(const FInputActionValue& Value)
{
    const FVector2D MovementVector = Value.Get<FVector2D>();
    if (!Controller) return;

    const FRotator Rotation    = Controller->GetControlRotation();
    const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

    const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDir   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    AddMovementInput(ForwardDir, MovementVector.Y);
    AddMovementInput(RightDir,   MovementVector.X);
}

// ── 시점 ──────────────────────────────────────────────────────
void ALastFPSHero::Look(const FInputActionValue& Value)
{
    const FVector2D LookVector = Value.Get<FVector2D>();
    AddControllerYawInput(LookVector.X);
    AddControllerPitchInput(LookVector.Y);
}

void ALastFPSHero::TryActivateAbilityByTag(const FGameplayTag& AbilityTag)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        return;
    }

    FGameplayTagContainer AbilityTags;
    AbilityTags.AddTag(AbilityTag);
    ASC->TryActivateAbilitiesByTag(AbilityTags);
}

void ALastFPSHero::CancelAbilityByTag(const FGameplayTag& AbilityTag)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        return;
    }

    FGameplayTagContainer AbilityTags;
    AbilityTags.AddTag(AbilityTag);
    ASC->CancelAbilities(&AbilityTags);
}

// ── 달리기 ────────────────────────────────────────────────────
void ALastFPSHero::StartSprint()
{
    static const FGameplayTag SprintTag = FGameplayTag::RequestGameplayTag("Ability.Sprint");
    TryActivateAbilityByTag(SprintTag);
}

void ALastFPSHero::StopSprint()
{
    static const FGameplayTag SprintTag = FGameplayTag::RequestGameplayTag("Ability.Sprint");
    CancelAbilityByTag(SprintTag);
}

// ── ADS ───────────────────────────────────────────────────────
void ALastFPSHero::StartADS()
{
    bIsADS = true;

    // 목표값만 바꾸면 Tick에서 부드럽게 보간됨
    TargetArmLength    = ADSArmLength;
    TargetSocketOffset = ADSSocketOffset;
    TargetFOV          = ADSFOV;
}

void ALastFPSHero::StopADS()
{
    bIsADS = false;

    TargetArmLength    = DefaultArmLength;
    TargetSocketOffset = DefaultSocketOffset;
    TargetFOV          = DefaultFOV;
}

// ── 점프 / 더블점프 ───────────────────────────────────────────
void ALastFPSHero::StartJump()
{
    static const FGameplayTag JumpTag = FGameplayTag::RequestGameplayTag("Ability.Jump");
    TryActivateAbilityByTag(JumpTag);
}

void ALastFPSHero::StopJump()
{
    // GAS가 아닌 직접 호출 유지 — 버튼 해제 시 가변 점프높이 컷오프를 위해
    StopJumping();
}

// ── 사격 ──────────────────────────────────────────────────────
void ALastFPSHero::StartFire()
{
    if (!IsAlive()) return;

    static const FGameplayTag FireTag = FGameplayTag::RequestGameplayTag("Ability.Fire");
    TryActivateAbilityByTag(FireTag);
}

void ALastFPSHero::StopFire()
{
    static const FGameplayTag FireTag = FGameplayTag::RequestGameplayTag("Ability.Fire");
    CancelAbilityByTag(FireTag);
}

// ── 스킬 슬롯 ─────────────────────────────────────────────────
void ALastFPSHero::StartSkill1()
{
    static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Ability.Skill1");
    TryActivateAbilityByTag(Tag);
}

void ALastFPSHero::StartSkill2()
{
    static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Ability.Skill2");
    TryActivateAbilityByTag(Tag);
}

void ALastFPSHero::StartUltimate()
{
    static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Ability.Ultimate");
    TryActivateAbilityByTag(Tag);
}

// ── 스코어보드 ─────────────────────────────────────────────────
void ALastFPSHero::StartScoreboard()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;
    if (ALastFPSHUD* HUD = Cast<ALastFPSHUD>(PC->GetHUD()))
        HUD->ShowScoreboard();
}

void ALastFPSHero::StopScoreboard()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;
    if (ALastFPSHUD* HUD = Cast<ALastFPSHUD>(PC->GetHUD()))
        HUD->HideScoreboard();
}
