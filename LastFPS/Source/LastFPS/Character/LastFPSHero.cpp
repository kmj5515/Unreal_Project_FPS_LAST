#include "Character/LastFPSHero.h"
#include "Character/Components/WeaponComponent.h"
#include "Input/LastFPSInputConfig.h"
#include "Utility/LastFPSTags.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GA_BasicShoot.h"
#include "AbilitySystem/Abilities/GA_Ultimate.h"
#include "Net/UnrealNetwork.h"

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

    // Action TPS: 캐릭터가 컨트롤러 Yaw를 즉시 따라감 (디비전/퍼스트 디센던트 스타일)
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw   = true;
    bUseControllerRotationRoll  = false;

    GetCharacterMovement()->bOrientRotationToMovement     = false;
    GetCharacterMovement()->bUseControllerDesiredRotation = false;
    GetCharacterMovement()->RotationRate                  = FRotator(0.f, 500.f, 0.f);
    GetCharacterMovement()->MaxWalkSpeed              = 600.f;
    GetCharacterMovement()->MaxWalkSpeedCrouched      = 200.f;
    GetCharacterMovement()->JumpZVelocity             = 700.f;
    GetCharacterMovement()->AirControl                = 0.4f;
    GetCharacterMovement()->GravityScale              = 1.5f;

    JumpMaxCount = 2;

    if (USkeletalMeshComponent* CharacterMesh = GetMesh())
    {
        CharacterMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    }

    WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));

    TargetArmLength    = DefaultArmLength;
    TargetSocketOffset = DefaultSocketOffset;
    TargetFOV          = DefaultFOV;
}

void ALastFPSHero::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALastFPSHero, CombatState);
}

void ALastFPSHero::GiveDefaultAbilities()
{
    Super::GiveDefaultAbilities();
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

void ALastFPSHero::TickCameraInterp(float DeltaTime)
{
    CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArmLength, DeltaTime, ADSInterpSpeed);
    CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetSocketOffset, DeltaTime, ADSInterpSpeed);
    FollowCamera->FieldOfView = FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaTime, ADSInterpSpeed);
}
void ALastFPSHero::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EIC || !InputConfig) return;

    const FLastFPSTags& FPSTags = FLastFPSTags::Get(); 

    EIC->BindAction(InputConfig->FindNativeInputActionByTag(FPSTags.Input_Move), ETriggerEvent::Triggered, this, &ALastFPSHero::Move);
    EIC->BindAction(InputConfig->FindNativeInputActionByTag(FPSTags.Input_Move), ETriggerEvent::Completed, this, &ALastFPSHero::ClearMoveInput);
    EIC->BindAction(InputConfig->FindNativeInputActionByTag(FPSTags.Input_Move), ETriggerEvent::Canceled, this, &ALastFPSHero::ClearMoveInput);
    EIC->BindAction(InputConfig->FindNativeInputActionByTag(FPSTags.Input_Look), ETriggerEvent::Triggered, this, &ALastFPSHero::Look);
    
    EIC->BindAction(InputConfig->FindNativeInputActionByTag(FPSTags.Input_ADS), ETriggerEvent::Started, this, &ALastFPSHero::SetADS, true);
    EIC->BindAction(InputConfig->FindNativeInputActionByTag(FPSTags.Input_ADS), ETriggerEvent::Completed, this, &ALastFPSHero::SetADS, false);
    EIC->BindAction(InputConfig->FindNativeInputActionByTag(FPSTags.Input_ADS), ETriggerEvent::Canceled, this, &ALastFPSHero::SetADS, false);
    
    // 어빌리티 전용 
    for (const FLastFPSInputAction& Action : InputConfig->AbilityInputActions)
    {
        EIC->BindAction(Action.InputAction, ETriggerEvent::Triggered, this, &ALastFPSHero::HandleAbilityInput, Action.InputTag);
        EIC->BindAction(Action.InputAction, ETriggerEvent::Completed, this, &ALastFPSHero::HandleAbilityInput, Action.InputTag);
        EIC->BindAction(Action.InputAction, ETriggerEvent::Canceled, this, &ALastFPSHero::HandleAbilityInput, Action.InputTag);
    }
}

void ALastFPSHero::HandleAbilityInput(const FInputActionValue& value, FGameplayTag InputID)
{
    const bool IsPressed = value.Get<bool>();
    if (IsPressed)
    {
        InputPressed(InputID);
    }
    else
    {
        if (!ShouldCancelAbilityOnRelease(InputID))
        {
            InputReleased(InputID);
        }
    }
}

void ALastFPSHero::Move(const FInputActionValue& Value)
{
    const FVector2D MovementVector = Value.Get<FVector2D>();
    CachedMoveInput = MovementVector;
    if (!Controller) return;

    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

    const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDir   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    AddMovementInput(ForwardDir, MovementVector.Y);
    AddMovementInput(RightDir,   MovementVector.X);
}

void ALastFPSHero::ClearMoveInput(const FInputActionValue& Value)
{
    CachedMoveInput = FVector2D::ZeroVector;
}

void ALastFPSHero::Look(const FInputActionValue& Value)
{
    const FVector2D LookVector = Value.Get<FVector2D>();
    AddControllerYawInput(LookVector.X);
    AddControllerPitchInput(LookVector.Y);
}

void ALastFPSHero::TryActivateAbilityByTag(FGameplayTag AbilityTag)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("TryActivateAbilityByTag failed: ASC is null. Tag=%s"), *AbilityTag.ToString());
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("TryActivateAbilityByTag: %s"), *AbilityTag.ToString());

    for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
    {
        if (!Spec.Ability)
        {
            UE_LOG(LogTemp, Warning, TEXT("ASC AbilitySpec: null ability. Handle=%s"), *Spec.Handle.ToString());
            continue;
        }

        const FGameplayTagContainer& AssetTags = Spec.Ability->GetAssetTags();

        UE_LOG(LogTemp, Warning, TEXT("ASC AbilitySpec: %s | Tags=[%s] | Active=%s"),
            *Spec.Ability->GetName(),
            *AssetTags.ToStringSimple(),
            Spec.IsActive() ? TEXT("true") : TEXT("false"));
    }
    
    FGameplayTagContainer AbilityTags;
    AbilityTags.AddTag(AbilityTag);
    const bool bActivated = ASC->TryActivateAbilitiesByTag(AbilityTags);
    UE_LOG(LogTemp, Warning, TEXT("TryActivateAbilitiesByTag result: %s"), bActivated ? TEXT("true") : TEXT("false"));
}

void ALastFPSHero::CancelAbilityByTag(FGameplayTag AbilityTag)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC) return;

    FGameplayTagContainer AbilityTags;
    AbilityTags.AddTag(AbilityTag);
    ASC->CancelAbilities(&AbilityTags);
}

void ALastFPSHero::InputPressed(FGameplayTag InputID)
{
    TryActivateAbilityByTag(InputID);
}

void ALastFPSHero::InputReleased(FGameplayTag InputID)
{
    CancelAbilityByTag(InputID);
}

void ALastFPSHero::SetADS(bool bEnabled)
{
    bIsADS = bEnabled;
    TargetArmLength    = bIsADS ? ADSArmLength : DefaultArmLength;
    TargetSocketOffset = bIsADS ? ADSSocketOffset : DefaultSocketOffset;
    TargetFOV          = bIsADS ? ADSFOV : DefaultFOV;

    if (CameraBoom)
    {
        CameraBoom->bEnableCameraLag = !bIsADS;
        CameraBoom->CameraLagSpeed = bIsADS ? 60.f : CameraLagSpeed;
    }

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        if (bIsADS)
        {
            PreADSWalkSpeed = Movement->MaxWalkSpeed;
            Movement->MaxWalkSpeed = ADSWalkSpeed;
        }
        else
        {
            Movement->MaxWalkSpeed = PreADSWalkSpeed > 0.f ? PreADSWalkSpeed : 600.f;
        }
    }
}

bool ALastFPSHero::ShouldCancelAbilityOnRelease(FGameplayTag InputID) const
{
    return InputConfig && InputConfig->ReleaseCancelInputTags.HasTagExact(InputID);
}

void ALastFPSHero::SetCombatState(EMMCombatState NewState)
{
    if (CombatState == NewState)
    {
        return;
    }

    CombatState = NewState;
}

void ALastFPSHero::OnRep_CombatState()
{
}

void ALastFPSHero::Multicast_PlayWeaponFireEffects_Implementation()
{
    if (GetNetMode() == NM_DedicatedServer || IsLocallyControlled()) return;
    if (WeaponComponent) WeaponComponent->PlayFireEffects();
}
