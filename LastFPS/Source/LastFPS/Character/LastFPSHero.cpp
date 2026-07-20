#include "Character/LastFPSHero.h"
#include "Character/Components/LastFPSGrapplingAnimationComponent.h"
#include "Character/Components/WeaponComponent.h"
#include "Input/LastFPSInputConfig.h"
#include "Utility/LastFPSTags.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
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
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "AbilitySystem/Abilities/LastFPSConfirmableAbility.h"
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

    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
    bUseControllerRotationYaw = false;

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->bOrientRotationToMovement = true;
        Movement->bUseControllerDesiredRotation = false;
        Movement->RotationRate = FRotator(0.f, 500.f, 0.f);
    }

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

    WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComp"));
    GrapplingAnimationComponent = CreateDefaultSubobject<ULastFPSGrapplingAnimationComponent>(
        TEXT("GrapplingAnimationComponent"));
    if (USkeletalMeshComponent* CharacterMesh = GetMesh())
    {
        CharacterMesh->AddTickPrerequisiteComponent(GrapplingAnimationComponent);
    }

    TargetArmLength    = DefaultArmLength;
    TargetSocketOffset = DefaultSocketOffset;
    TargetFOV          = DefaultFOV;
	SpeedBoostCameraTag = LastFPSGameplayTags::Status_Movement_SpeedBoost;
}

void ALastFPSHero::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALastFPSHero, CombatState);
    DOREPLIFETIME(ALastFPSHero, JumpStartSequence);
    DOREPLIFETIME(ALastFPSHero, bIsSprinting);
}

void ALastFPSHero::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	RefreshMaxWalkSpeed();
	BindSpeedBoostCameraTag();
}

void ALastFPSHero::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	RefreshMaxWalkSpeed();
	BindSpeedBoostCameraTag();
}

void ALastFPSHero::GiveDefaultAbilities()
{
    Super::GiveDefaultAbilities();
}

void ALastFPSHero::OnCombatEngagedChanged()
{
    Super::OnCombatEngagedChanged();
    ApplyRotationModeSettings();
}

void ALastFPSHero::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	Super::OnMoveSpeedChanged(Data);
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	UpdateSpeedBoostCameraOffset(Movement ? Movement->MaxWalkSpeed : Data.NewValue);
}

float ALastFPSHero::ResolveMaxWalkSpeed(const float AttributeMoveSpeed) const
{
	if (GetWantsToWalk())
	{
		return FMath::Max(WalkMaxWalkSpeed, 0.f);
	}

	if (bIsADS)
	{
		return FMath::Max(ADSWalkSpeed, 0.f);
	}

	return Super::ResolveMaxWalkSpeed(AttributeMoveSpeed);
}

void ALastFPSHero::BeginPlay()
{
    Super::BeginPlay();

    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
    bUseControllerRotationYaw = false;

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->bOrientRotationToMovement = true;
        Movement->bUseControllerDesiredRotation = false;
    }

    if (WeaponComponent)
    {
        WeaponComponent->OnWeaponEquippedChanged.AddUniqueDynamic(this, &ALastFPSHero::HandleWeaponEquippedChanged);
    }

    ApplyRotationModeSettings();
	BindSpeedBoostCameraTag();

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

void ALastFPSHero::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindSpeedBoostCameraTag();

    if (WeaponComponent)
    {
        WeaponComponent->OnWeaponEquippedChanged.RemoveDynamic(this, &ALastFPSHero::HandleWeaponEquippedChanged);
    }

    Super::EndPlay(EndPlayReason);
}

void ALastFPSHero::SetGameplayInputEnabled(bool bEnabled)
{
    if (!DefaultMappingContext)
    {
        return;
    }

    const APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return; // 로컬 조종 중이 아니면 매핑 컨텍스트를 건드릴 대상이 없다.
    }

    UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
    if (!Subsystem)
    {
        return;
    }

    if (bEnabled)
    {
        // 상호작용 종료 시 게임 입력을 복원한다. BeginPlay와 동일 우선순위로 다시 추가한다.
        if (!Subsystem->HasMappingContext(DefaultMappingContext))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
    else
    {
        // 상호작용 진입 시 게임 입력 컨텍스트를 제거해 이동과 전투 입력을 막는다.
        Subsystem->RemoveMappingContext(DefaultMappingContext);
        // ADS 홀드 중 컨텍스트가 제거되면 해제 입력이 오지 않을 수 있어 방어적으로 해제한다.
        SetADS(false);
    }
}

void ALastFPSHero::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    TickCameraInterp(DeltaTime);
}

void ALastFPSHero::TickCameraInterp(float DeltaTime)
{
	const float ArmInterpSpeed = !bIsADS && bSpeedBoostCameraTransition
		? SpeedBoostCameraInterpSpeed
		: ADSInterpSpeed;
    CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArmLength, DeltaTime, ArmInterpSpeed);
    CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetSocketOffset, DeltaTime, ADSInterpSpeed);
    FollowCamera->FieldOfView = FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaTime, ADSInterpSpeed);

	if (bSpeedBoostCameraTransition
		&& FMath::IsNearlyEqual(CameraBoom->TargetArmLength, TargetArmLength, 0.5f))
	{
		bSpeedBoostCameraTransition = false;
	}

    TickTemporaryCameraEffect(DeltaTime);
}

bool ALastFPSHero::BeginTemporaryCameraEffect(
    UObject* RequestOwner,
    const FLastFPSTemporaryCameraEffectOptions& Options)
{
    if (!IsLocallyControlled() || !IsValid(RequestOwner) || !CameraBoom || !FollowCamera)
    {
        return false;
    }

    if (bTemporaryCameraEffectOverrideActive
        && ActiveCameraEffectOwner.IsValid()
        && ActiveCameraEffectOwner.Get() != RequestOwner)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Temporary camera effect rejected: Hero=%s, ActiveOwner=%s, RequestOwner=%s, reason=another effect owns the camera override."),
            *GetNameSafe(this),
            *GetNameSafe(ActiveCameraEffectOwner.Get()),
            *GetNameSafe(RequestOwner));
        return false;
    }

    if (!bTemporaryCameraEffectOverrideActive)
    {
        SavedCameraLagSpeed = CameraBoom->CameraLagSpeed;
        bSavedCameraLagEnabled = CameraBoom->bEnableCameraLag;
        SavedMotionBlurAmount = FollowCamera->PostProcessSettings.MotionBlurAmount;
        bSavedMotionBlurOverride = FollowCamera->PostProcessSettings.bOverride_MotionBlurAmount;
    }

    bTemporaryCameraEffectOverrideActive = true;
    bTemporaryCameraEffectBlendOutActive = false;
    CameraEffectBlendOutDuration = FMath::Max(0.f, Options.BlendOutDuration);
    CameraEffectBlendOutElapsed = 0.f;
    ActiveCameraEffectOwner = RequestOwner;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = FMath::Max(0.01f, Options.CameraLagSpeed);
    FollowCamera->PostProcessSettings.bOverride_MotionBlurAmount = true;
    FollowCamera->PostProcessSettings.MotionBlurAmount = FMath::Clamp(
        Options.MotionBlurAmount,
        0.f,
        1.f);
    return true;
}

void ALastFPSHero::EndTemporaryCameraEffect(UObject* RequestOwner)
{
    if (!bTemporaryCameraEffectOverrideActive
        || !ActiveCameraEffectOwner.IsValid()
        || ActiveCameraEffectOwner.Get() != RequestOwner)
    {
        return;
    }

    ActiveCameraEffectOwner.Reset();
    if (CameraEffectBlendOutDuration <= KINDA_SMALL_NUMBER)
    {
        RestoreTemporaryCameraEffect();
        return;
    }

    CameraEffectBlendOutElapsed = 0.f;
    CameraEffectBlendOutStartLagSpeed = CameraBoom
        ? CameraBoom->CameraLagSpeed
        : SavedCameraLagSpeed;
    CameraEffectBlendOutStartMotionBlurAmount = FollowCamera
        ? FollowCamera->PostProcessSettings.MotionBlurAmount
        : SavedMotionBlurAmount;
    bTemporaryCameraEffectBlendOutActive = true;
}

void ALastFPSHero::TickTemporaryCameraEffect(const float DeltaTime)
{
    if (!bTemporaryCameraEffectBlendOutActive)
    {
        return;
    }

    CameraEffectBlendOutElapsed += FMath::Max(0.f, DeltaTime);
    const float LinearAlpha = FMath::Clamp(
        CameraEffectBlendOutElapsed / CameraEffectBlendOutDuration,
        0.f,
        1.f);
    const float SmoothAlpha = LinearAlpha * LinearAlpha * (3.f - 2.f * LinearAlpha);

    if (CameraBoom)
    {
        CameraBoom->bEnableCameraLag = true;
        CameraBoom->CameraLagSpeed = FMath::Lerp(
            CameraEffectBlendOutStartLagSpeed,
            SavedCameraLagSpeed,
            SmoothAlpha);
    }
    if (FollowCamera)
    {
        FollowCamera->PostProcessSettings.bOverride_MotionBlurAmount = true;
        FollowCamera->PostProcessSettings.MotionBlurAmount = FMath::Lerp(
            CameraEffectBlendOutStartMotionBlurAmount,
            SavedMotionBlurAmount,
            SmoothAlpha);
    }

    if (LinearAlpha >= 1.f)
    {
        RestoreTemporaryCameraEffect();
    }
}

void ALastFPSHero::RestoreTemporaryCameraEffect()
{
    if (CameraBoom)
    {
        CameraBoom->bEnableCameraLag = bSavedCameraLagEnabled;
        CameraBoom->CameraLagSpeed = SavedCameraLagSpeed;
    }
    if (FollowCamera)
    {
        FollowCamera->PostProcessSettings.bOverride_MotionBlurAmount = bSavedMotionBlurOverride;
        FollowCamera->PostProcessSettings.MotionBlurAmount = SavedMotionBlurAmount;
    }

    ActiveCameraEffectOwner.Reset();
    bTemporaryCameraEffectOverrideActive = false;
    bTemporaryCameraEffectBlendOutActive = false;
    CameraEffectBlendOutDuration = 0.f;
    CameraEffectBlendOutElapsed = 0.f;
}

void ALastFPSHero::BindSpeedBoostCameraTag()
{
	if (!IsLocallyControlled() || !SpeedBoostCameraTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || (SpeedBoostCameraASC.Get() == ASC && SpeedBoostCameraTagDelegateHandle.IsValid()))
	{
		return;
	}

	UnbindSpeedBoostCameraTag();
	SpeedBoostCameraASC = ASC;
	SpeedBoostCameraTagDelegateHandle = ASC->RegisterGameplayTagEvent(
		SpeedBoostCameraTag,
		EGameplayTagEventType::AnyCountChange).AddUObject(
			this,
			&ALastFPSHero::HandleSpeedBoostCameraTagChanged);

	HandleSpeedBoostCameraTagChanged(SpeedBoostCameraTag, ASC->GetTagCount(SpeedBoostCameraTag));
}

void ALastFPSHero::UnbindSpeedBoostCameraTag()
{
	if (SpeedBoostCameraTagDelegateHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = SpeedBoostCameraASC.Get())
		{
			ASC->RegisterGameplayTagEvent(
				SpeedBoostCameraTag,
				EGameplayTagEventType::AnyCountChange).Remove(SpeedBoostCameraTagDelegateHandle);
		}
		SpeedBoostCameraTagDelegateHandle.Reset();
	}

	SpeedBoostCameraASC.Reset();
}

void ALastFPSHero::HandleSpeedBoostCameraTagChanged(const FGameplayTag Tag, const int32 NewCount)
{
	if (Tag != SpeedBoostCameraTag)
	{
		return;
	}

	bSpeedBoostCameraActive = bIsSprinting || NewCount > 0;
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	UpdateSpeedBoostCameraOffset(Movement ? Movement->MaxWalkSpeed : 0.f);
}

void ALastFPSHero::UpdateSpeedBoostCameraOffset(const float EffectiveMoveSpeed)
{
	float NewOffset = 0.f;
	if (bSpeedBoostCameraActive)
	{
		const float SpeedRange = SpeedBoostCameraFullSpeed - SpeedBoostCameraStartSpeed;
		const float SpeedAlpha = SpeedRange > KINDA_SMALL_NUMBER
			? FMath::Clamp((EffectiveMoveSpeed - SpeedBoostCameraStartSpeed) / SpeedRange, 0.f, 1.f)
			: (EffectiveMoveSpeed > SpeedBoostCameraStartSpeed ? 1.f : 0.f);
		NewOffset = FMath::Max(SpeedBoostArmLengthOffset, 0.f) * SpeedAlpha;
	}

	if (!FMath::IsNearlyEqual(CurrentSpeedBoostArmLengthOffset, NewOffset))
	{
		CurrentSpeedBoostArmLengthOffset = NewOffset;
		bSpeedBoostCameraTransition = true;
	}

	RefreshCameraTargets();
}

void ALastFPSHero::RefreshCameraTargets()
{
	TargetArmLength = bIsADS
		? ADSArmLength
		: DefaultArmLength + CurrentSpeedBoostArmLengthOffset;
	TargetSocketOffset = bIsADS ? ADSSocketOffset : DefaultSocketOffset;
	TargetFOV = bIsADS ? ADSFOV : DefaultFOV;
}

void ALastFPSHero::ApplyRotationModeSettings()
{
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    const bool bUseControllerYawRotation = ShouldUseControllerYawRotationMode();
    bUseControllerRotationYaw = bUseControllerYawRotation;

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->bOrientRotationToMovement = ShouldOrientRotationToMovement();
        Movement->bUseControllerDesiredRotation = false;
        Movement->RotationRate = FRotator(0.f, 500.f, 0.f);
    }
}

bool ALastFPSHero::ShouldUseControllerYawRotationMode() const
{
    return HasEquippedWeapon() || CombatState == EMMCombatState::Attacking;
}

bool ALastFPSHero::ShouldOrientRotationToMovement() const
{
    return !ShouldUseControllerYawRotationMode();
}

bool ALastFPSHero::HasEquippedWeapon() const
{
    return WeaponComponent && WeaponComponent->HasWeapon();
}

void ALastFPSHero::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EIC || !InputConfig) return;

    EIC->BindAction(InputConfig->FindNativeInputActionByTag(LastFPSGameplayTags::Input_Move), ETriggerEvent::Triggered, this, &ALastFPSHero::Move);
    EIC->BindAction(InputConfig->FindNativeInputActionByTag(LastFPSGameplayTags::Input_Move), ETriggerEvent::Completed, this, &ALastFPSHero::ClearMoveInput);
    EIC->BindAction(InputConfig->FindNativeInputActionByTag(LastFPSGameplayTags::Input_Move), ETriggerEvent::Canceled, this, &ALastFPSHero::ClearMoveInput);
    EIC->BindAction(InputConfig->FindNativeInputActionByTag(LastFPSGameplayTags::Input_Look), ETriggerEvent::Triggered, this, &ALastFPSHero::Look);

    EIC->BindAction(InputConfig->FindNativeInputActionByTag(LastFPSGameplayTags::Input_ADS), ETriggerEvent::Started, this, &ALastFPSHero::SetADS, true);
    EIC->BindAction(InputConfig->FindNativeInputActionByTag(LastFPSGameplayTags::Input_ADS), ETriggerEvent::Completed, this, &ALastFPSHero::SetADS, false);
    EIC->BindAction(InputConfig->FindNativeInputActionByTag(LastFPSGameplayTags::Input_ADS), ETriggerEvent::Canceled, this, &ALastFPSHero::SetADS, false);
    
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
        if (InputID == LastFPSGameplayTags::Input_Sprint)
        {
            SetWantsToWalk(false);
            return;
        }

        if (!ShouldSkipAbilityCancelOnRelease(InputID) && !ShouldBlockAbilityInputRelease(InputID))
        {
            InputReleased(InputID);
        }
    }
}

void ALastFPSHero::Move(const FInputActionValue& Value)
{
    const FVector2D MovementVector = Value.Get<FVector2D>();
    CachedMoveInput = MovementVector;
    bHasMoveInputAction = true;

    if ((bIsSprinting || bWantsToSprint) && !HasForwardSprintInput())
    {
        SetWantsToSprint(false);
        CancelAbilityByTag(LastFPSGameplayTags::Input_Sprint);
    }

    if (!Controller) return;

    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

    const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDir   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    FVector2D EffectiveMovementVector = MovementVector;
    if (bIsSprinting || bWantsToSprint)
    {
        EffectiveMovementVector.X = 0.f;
        EffectiveMovementVector.Y = FMath::Max(EffectiveMovementVector.Y, 0.f);
    }

    LocomotionDirectionBaseRotation = GetActorRotation();

    AddMovementInput(ForwardDir, EffectiveMovementVector.Y);
    AddMovementInput(RightDir,   EffectiveMovementVector.X);
}

void ALastFPSHero::ClearMoveInput(const FInputActionValue& Value)
{
    CachedMoveInput = FVector2D::ZeroVector;
    bHasMoveInputAction = false;

    if (bIsSprinting || bWantsToSprint)
    {
        SetWantsToSprint(false);
        CancelAbilityByTag(LastFPSGameplayTags::Input_Sprint);
    }

    SetWantsToWalk(false);
}

void ALastFPSHero::Look(const FInputActionValue& Value)
{
    const FVector2D LookVector = Value.Get<FVector2D>();
    AddControllerYawInput(LookVector.X);
    AddControllerPitchInput(LookVector.Y);
}

bool ALastFPSHero::TryActivateAbilityByTag(FGameplayTag AbilityTag)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("TryActivateAbilityByTag failed: ASC is null. Tag=%s"), *AbilityTag.ToString());
        return false;
    }

    //UE_LOG(LogTemp, Warning, TEXT("TryActivateAbilityByTag: %s"), *AbilityTag.ToString());

    bool bAlreadyActive = false;
    for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
    {
        if (!Spec.Ability)
        {
            continue;
        }

        const FGameplayTagContainer& AssetTags = Spec.Ability->GetAssetTags();
        if (AssetTags.HasTagExact(AbilityTag) && Spec.IsActive())
        {
            bAlreadyActive = true;
        }
    }
    
    FGameplayTagContainer AbilityTags;
    AbilityTags.AddTag(AbilityTag);
    const bool bActivated = ASC->TryActivateAbilitiesByTag(AbilityTags);
    return bActivated || bAlreadyActive;
}

bool ALastFPSHero::TryConfirmActiveAbility(FGameplayTag InputID)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        return false;
    }

    for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
    {
        if (!Spec.IsActive())
        {
            continue;
        }

        UGameplayAbility* ActiveAbility = Spec.GetPrimaryInstance();
        ILastFPSConfirmableAbility* ConfirmableAbility = Cast<ILastFPSConfirmableAbility>(ActiveAbility);
        if (!ConfirmableAbility || !ConfirmableAbility->CanConfirmAbilityInput(InputID))
        {
            continue;
        }

        return ConfirmableAbility->ConfirmAbilityInput(InputID);
    }

    return false;
}

bool ALastFPSHero::TryCancelActiveAbility(FGameplayTag InputID)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        return false;
    }

    for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
    {
        if (!Spec.IsActive())
        {
            continue;
        }

        UGameplayAbility* ActiveAbility = Spec.GetPrimaryInstance();
        ILastFPSConfirmableAbility* ConfirmableAbility = Cast<ILastFPSConfirmableAbility>(ActiveAbility);
        if (!ConfirmableAbility || !ConfirmableAbility->CanCancelAbilityInput(InputID))
        {
            continue;
        }

        return ConfirmableAbility->CancelAbilityInput(InputID);
    }

    return false;
}

bool ALastFPSHero::ShouldBlockAbilityInputRelease(FGameplayTag InputID)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        return false;
    }

    for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
    {
        if (!Spec.IsActive())
        {
            continue;
        }

        UGameplayAbility* ActiveAbility = Spec.GetPrimaryInstance();
        const ILastFPSConfirmableAbility* ConfirmableAbility = Cast<ILastFPSConfirmableAbility>(ActiveAbility);
        if (ConfirmableAbility && ConfirmableAbility->ShouldBlockAbilityInputRelease(InputID))
        {
            return true;
        }
    }

    return false;
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
    if (TryCancelActiveAbility(InputID))
    {
        return;
    }

    if (TryConfirmActiveAbility(InputID))
    {
        return;
    }

    if (InputID == LastFPSGameplayTags::Input_Sprint)
    {
        SetWantsToWalk(true);
        return;
    }

    if (ShouldCancelFireBeforeAbilityInput(InputID))
    {
        CancelAbilityByTag(LastFPSGameplayTags::Input_Fire);
    }

    const bool bActivatedOrActive = TryActivateAbilityByTag(InputID);
    if (InputID == LastFPSGameplayTags::Input_Sprint && !bActivatedOrActive && !bIsSprinting)
    {
        SetWantsToSprint(false);
    }
}

void ALastFPSHero::InputReleased(FGameplayTag InputID)
{
    if (InputID == LastFPSGameplayTags::Input_Sprint)
    {
        SetWantsToWalk(false);
        return;
    }

    CancelAbilityByTag(InputID);
}

void ALastFPSHero::SetADS(bool bEnabled)
{
    if (bEnabled && TryCancelActiveAbility(LastFPSGameplayTags::Input_ADS))
    {
        return;
    }

    if (bEnabled && (bIsSprinting || bWantsToSprint))
    {
        SetWantsToSprint(false);
        CancelAbilityByTag(LastFPSGameplayTags::Input_Sprint);
    }

    if (bEnabled && bWantsToWalk)
    {
        SetWantsToWalk(false);
    }

    bIsADS = bEnabled;
	RefreshCameraTargets();
    ApplyRotationModeSettings();

    if (CameraBoom)
    {
        const bool bTargetCameraLagEnabled = !bIsADS;
        const float TargetCameraLagSpeed = bIsADS ? 60.f : CameraLagSpeed;
        if (bTemporaryCameraEffectOverrideActive)
        {
            bSavedCameraLagEnabled = bTargetCameraLagEnabled;
            SavedCameraLagSpeed = TargetCameraLagSpeed;
        }
        else
        {
            CameraBoom->bEnableCameraLag = bTargetCameraLagEnabled;
            CameraBoom->CameraLagSpeed = TargetCameraLagSpeed;
        }
    }

	RefreshMaxWalkSpeed();
}

bool ALastFPSHero::ShouldCancelFireBeforeAbilityInput(FGameplayTag InputID) const
{
    return InputID == LastFPSGameplayTags::Input_Skill1
        || InputID == LastFPSGameplayTags::Input_Skill2
        || InputID == LastFPSGameplayTags::Input_Skill3
        || InputID == LastFPSGameplayTags::Input_Ultimate
        || InputID == LastFPSGameplayTags::Input_GrapplingHook;
}

bool ALastFPSHero::ShouldSkipAbilityCancelOnRelease(FGameplayTag InputID) const
{
    return InputConfig && InputConfig->ReleaseCancelInputTags.HasTagExact(InputID);
}

bool ALastFPSHero::HasForwardSprintInput() const
{
    return CachedMoveInput.Y > SprintForwardInputThreshold;
}

bool ALastFPSHero::CanStartSprint() const
{
    return HasForwardSprintInput() && !bIsADS;
}

void ALastFPSHero::SetSprinting(bool bEnabled)
{
    if (bIsSprinting == bEnabled)
    {
        if (!bEnabled)
        {
            SetWantsToSprint(false);
        }
        return;
    }

    bIsSprinting = bEnabled;
    ApplyRotationModeSettings();

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	bSpeedBoostCameraActive = bIsSprinting
		|| (ASC && ASC->HasMatchingGameplayTag(SpeedBoostCameraTag));
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	UpdateSpeedBoostCameraOffset(Movement ? Movement->MaxWalkSpeed : 0.f);

    if (!bEnabled)
    {
        SetWantsToSprint(false);
    }
}

void ALastFPSHero::SetWantsToSprint(bool bEnabled)
{
    if (bWantsToSprint == bEnabled)
    {
        return;
    }

    bWantsToSprint = bEnabled;
    ApplyRotationModeSettings();
}

void ALastFPSHero::SetWantsToWalk(bool bEnabled)
{
    if (bWantsToWalk == bEnabled)
    {
        return;
    }

    bWantsToWalk = bEnabled;
	RefreshMaxWalkSpeed();
    ApplyRotationModeSettings();
}

void ALastFPSHero::RefreshMaxWalkSpeed()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!Movement || !ASC)
	{
		return;
	}

	const float AttributeMoveSpeed = ASC->GetNumericAttribute(
		ULastFPSAttributeSet::GetMoveSpeedAttribute());
	Movement->MaxWalkSpeed = ResolveMaxWalkSpeed(AttributeMoveSpeed);
	UpdateSpeedBoostCameraOffset(Movement->MaxWalkSpeed);
}

void ALastFPSHero::SetCombatState(EMMCombatState NewState)
{
    if (CombatState == NewState)
    {
        return;
    }

    CombatState = NewState;
	RefreshMaxWalkSpeed();
    ApplyRotationModeSettings();
}

void ALastFPSHero::NotifyJumpStarted()
{
    ++JumpStartSequence;
}

void ALastFPSHero::OnRep_CombatState()
{
	RefreshMaxWalkSpeed();
    ApplyRotationModeSettings();
}

void ALastFPSHero::HandleWeaponEquippedChanged(bool /*bEquipped*/)
{
    ApplyRotationModeSettings();
}

void ALastFPSHero::Multicast_PlayWeaponFireEffects_Implementation()
{
    if (GetNetMode() == NM_DedicatedServer || IsLocallyControlled()) return;
    if (WeaponComponent) WeaponComponent->PlayFireEffects();
}
