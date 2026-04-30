#include "Character/LastFPSHero.h"
#include "Input/LastFPSInputConfig.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

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
    GetCharacterMovement()->MaxWalkSpeed = 700.f;
}

void ALastFPSHero::StopSprint()
{
    GetCharacterMovement()->MaxWalkSpeed = 400.f;
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
