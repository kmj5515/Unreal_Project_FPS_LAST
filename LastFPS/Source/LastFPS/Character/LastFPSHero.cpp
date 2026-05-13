#include "Character/LastFPSHero.h"
#include "Character/Components/WeaponComponent.h"
#include "Input/LastFPSInputConfig.h"
#include "UI/LastFPSHUD.h"
#include "Game/LastFPSMatchGameState.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
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

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
    CameraBoom->TargetArmLength          = DefaultArmLength;
    CameraBoom->SocketOffset             = DefaultSocketOffset;
    CameraBoom->bUsePawnControlRotation  = true;
    CameraBoom->bEnableCameraLag         = true;
    CameraBoom->CameraLagSpeed           = CameraLagSpeed;
    CameraBoom->bEnableCameraRotationLag = false;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
    FollowCamera->FieldOfView             = DefaultFOV;

    // Yaw만 컨트롤러 따라가야 다른 클라에 회전 복제됨. bOrientRotationToMovement와 동시 사용 금지.
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw   = true;
    bUseControllerRotationRoll  = false;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->RotationRate              = FRotator(0.f, 500.f, 0.f);
    GetCharacterMovement()->MaxWalkSpeed              = 400.f;
    GetCharacterMovement()->MaxWalkSpeedCrouched      = 200.f;
    GetCharacterMovement()->JumpZVelocity             = 700.f;
    GetCharacterMovement()->AirControl                = 0.4f;
    GetCharacterMovement()->GravityScale              = 1.5f;

    JumpMaxCount = 2;

    WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));

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
    TickLocalMatchIntro();
    TickCameraInterp(DeltaTime);
}

void ALastFPSHero::TickLocalMatchIntro()
{
    if (!IsLocallyControlled())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    ALastFPSMatchGameState* MatchGS = World->GetGameState<ALastFPSMatchGameState>();
    if (!MatchGS)
    {
        if (bLocalMatchIntroInputDisabled)
        {
            if (APlayerController* PC = Cast<APlayerController>(GetController()))
            {
                EnableInput(PC);
            }
            bLocalMatchIntroInputDisabled = false;
        }
        bLocalMatchIntroFxStarted = false;
        return;
    }

    const bool bIntro = MatchGS->IsDropIntroActive();

    if (bIntro && !bLocalMatchIntroFxStarted)
    {
        bLocalMatchIntroFxStarted = true;

        if (MatchIntroMontage && GetMesh())
        {
            if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
            {
                AnimInst->Montage_Play(MatchIntroMontage);
            }
        }

        if (MatchIntroCameraShake)
        {
            if (APlayerController* PC = Cast<APlayerController>(GetController()))
            {
                if (APlayerCameraManager* PCM = PC->PlayerCameraManager)
                {
                    PCM->StartCameraShake(MatchIntroCameraShake, MatchIntroCameraShakeScale);
                }
            }
        }

        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            DisableInput(PC);
            bLocalMatchIntroInputDisabled = true;
        }
    }

    if (!bIntro && bLocalMatchIntroInputDisabled)
    {
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            EnableInput(PC);
        }
        bLocalMatchIntroInputDisabled = false;
    }

    if (!bIntro)
    {
        bLocalMatchIntroFxStarted = false;
    }
}

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

void ALastFPSHero::StartADS()
{
    bIsADS = true;

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

void ALastFPSHero::StartJump()
{
    static const FGameplayTag JumpTag = FGameplayTag::RequestGameplayTag("Ability.Jump");
    TryActivateAbilityByTag(JumpTag);
}

void ALastFPSHero::StopJump()
{
    // 가변 점프 높이: GAS 경유 없이 StopJumping 직접 호출
    StopJumping();
}

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

void ALastFPSHero::Multicast_PlayWeaponFireEffects_Implementation()
{
    if (GetNetMode() == NM_DedicatedServer)
        return;
    if (IsLocallyControlled())
        return;
    if (WeaponComponent)
        WeaponComponent->PlayFireEffects();
}
