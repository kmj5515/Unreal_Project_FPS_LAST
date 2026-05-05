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
    CameraBoom->CameraLagSpeed           = 15.f;
    CameraBoom->bEnableCameraRotationLag = false;

    // ── 카메라 ────────────────────────────────────────────────
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
    FollowCamera->FieldOfView             = DefaultFOV;

    // ── 캐릭터 회전 설정 ──────────────────────────────────────
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw   = false;
    bUseControllerRotationRoll  = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
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

    if (const UInputAction* IA = InputConfig->FindNativeInputActionByTag(FGameplayTag::RequestGameplayTag("InputTag.Move")))
        EIC->BindAction(IA, ETriggerEvent::Triggered, this, &ALastFPSHero::Move);

    if (const UInputAction* IA = InputConfig->FindNativeInputActionByTag(FGameplayTag::RequestGameplayTag("InputTag.Look")))
        EIC->BindAction(IA, ETriggerEvent::Triggered, this, &ALastFPSHero::Look);

    if (const UInputAction* IA = InputConfig->FindNativeInputActionByTag(FGameplayTag::RequestGameplayTag("InputTag.Sprint")))
    {
        EIC->BindAction(IA, ETriggerEvent::Started,   this, &ALastFPSHero::StartSprint);
        EIC->BindAction(IA, ETriggerEvent::Completed, this, &ALastFPSHero::StopSprint);
    }

    if (const UInputAction* IA = InputConfig->FindNativeInputActionByTag(FGameplayTag::RequestGameplayTag("InputTag.ADS")))
    {
        EIC->BindAction(IA, ETriggerEvent::Started,   this, &ALastFPSHero::StartADS);
        EIC->BindAction(IA, ETriggerEvent::Completed, this, &ALastFPSHero::StopADS);
    }

    if (const UInputAction* IA = InputConfig->FindNativeInputActionByTag(FGameplayTag::RequestGameplayTag("InputTag.Jump")))
    {
        EIC->BindAction(IA, ETriggerEvent::Started,   this, &ALastFPSHero::StartJump);
        EIC->BindAction(IA, ETriggerEvent::Completed, this, &ALastFPSHero::StopJump);
    }

    if (const UInputAction* IA = InputConfig->FindNativeInputActionByTag(FGameplayTag::RequestGameplayTag("InputTag.Fire")))
    {
        EIC->BindAction(IA, ETriggerEvent::Started,   this, &ALastFPSHero::StartFire);
        EIC->BindAction(IA, ETriggerEvent::Completed, this, &ALastFPSHero::StopFire);
    }

    if (const UInputAction* IA = InputConfig->FindAbilityInputActionByTag(FGameplayTag::RequestGameplayTag("InputTag.Skill1")))
        EIC->BindAction(IA, ETriggerEvent::Started, this, &ALastFPSHero::StartSkill1);
    if (const UInputAction* IA = InputConfig->FindAbilityInputActionByTag(FGameplayTag::RequestGameplayTag("InputTag.Skill2")))
        EIC->BindAction(IA, ETriggerEvent::Started, this, &ALastFPSHero::StartSkill2);
    if (const UInputAction* IA = InputConfig->FindAbilityInputActionByTag(FGameplayTag::RequestGameplayTag("InputTag.Ultimate")))
        EIC->BindAction(IA, ETriggerEvent::Started, this, &ALastFPSHero::StartUltimate);

    if (const UInputAction* IA = InputConfig->FindNativeInputActionByTag(FGameplayTag::RequestGameplayTag("InputTag.Scoreboard")))
    {
        EIC->BindAction(IA, ETriggerEvent::Started,   this, &ALastFPSHero::StartScoreboard);
        EIC->BindAction(IA, ETriggerEvent::Completed, this, &ALastFPSHero::StopScoreboard);
    }
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

// ── 달리기 ────────────────────────────────────────────────────
void ALastFPSHero::StartSprint()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC) return;

    static const FGameplayTag SprintTag = FGameplayTag::RequestGameplayTag("Ability.Sprint");
    FGameplayTagContainer SprintTags;
    SprintTags.AddTag(SprintTag);
    ASC->TryActivateAbilitiesByTag(SprintTags);
}

void ALastFPSHero::StopSprint()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC) return;

    static const FGameplayTag SprintTag = FGameplayTag::RequestGameplayTag("Ability.Sprint");
    FGameplayTagContainer SprintTags;
    SprintTags.AddTag(SprintTag);
    ASC->CancelAbilities(&SprintTags);
}

// ── ADS ───────────────────────────────────────────────────────
void ALastFPSHero::StartADS()
{
    bIsADS = true;

    // 목표값만 바꾸면 Tick에서 부드럽게 보간됨
    TargetArmLength    = ADSArmLength;
    TargetSocketOffset = ADSSocketOffset;
    TargetFOV          = ADSFOV;

    bUseControllerRotationYaw                         = true;
    GetCharacterMovement()->bOrientRotationToMovement = false;
}

void ALastFPSHero::StopADS()
{
    bIsADS = false;

    TargetArmLength    = DefaultArmLength;
    TargetSocketOffset = DefaultSocketOffset;
    TargetFOV          = DefaultFOV;

    bUseControllerRotationYaw                         = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
}

// ── 점프 / 더블점프 ───────────────────────────────────────────
void ALastFPSHero::StartJump()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC) return;

    static const FGameplayTag JumpTag = FGameplayTag::RequestGameplayTag("Ability.Jump");
    FGameplayTagContainer JumpTags;
    JumpTags.AddTag(JumpTag);
    ASC->TryActivateAbilitiesByTag(JumpTags);
}

void ALastFPSHero::StopJump()
{
    // GAS가 아닌 직접 호출 유지 — 버튼 해제 시 가변 점프높이 컷오프를 위해
    StopJumping();
}

// ── 사격 ──────────────────────────────────────────────────────
void ALastFPSHero::StartFire()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC) return;

    static const FGameplayTag FireTag = FGameplayTag::RequestGameplayTag("Ability.Fire");
    FGameplayTagContainer FireTags;
    FireTags.AddTag(FireTag);
    ASC->TryActivateAbilitiesByTag(FireTags);
}

void ALastFPSHero::StopFire()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC) return;

    static const FGameplayTag FireTag = FGameplayTag::RequestGameplayTag("Ability.Fire");
    FGameplayTagContainer FireTags;
    FireTags.AddTag(FireTag);
    ASC->CancelAbilities(&FireTags);
}

// ── 스킬 슬롯 ─────────────────────────────────────────────────
void ALastFPSHero::StartSkill1()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC) return;

    static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Ability.Skill1");
    FGameplayTagContainer Skill1Tags;
    Skill1Tags.AddTag(Tag);
    ASC->TryActivateAbilitiesByTag(Skill1Tags);
}

void ALastFPSHero::StartSkill2()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC) return;

    static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Ability.Skill2");
    FGameplayTagContainer Skill2Tags;
    Skill2Tags.AddTag(Tag);
    ASC->TryActivateAbilitiesByTag(Skill2Tags);
}

void ALastFPSHero::StartUltimate()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC) return;

    static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Ability.Ultimate");
    FGameplayTagContainer UltimateTags;
    UltimateTags.AddTag(Tag);
    ASC->TryActivateAbilitiesByTag(UltimateTags);
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
