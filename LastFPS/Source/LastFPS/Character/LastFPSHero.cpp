#include "Character/LastFPSHero.h"
#include "Character/Components/LastFPSGrapplingAnimationComponent.h"
#include "Character/Components/LastFPSGrapplingTargetingComponent.h"
#include "Character/Components/WeaponComponent.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "Game/LastFPSGameModeBase.h"
#include "Inventory/LastFPSEquipmentSubsystem.h"
#include "Character/LastFPSPet.h"
#include "Blueprint/UserWidget.h"
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
    GrapplingTargetingComponent = CreateDefaultSubobject<ULastFPSGrapplingTargetingComponent>(
        TEXT("GrapplingTargetingComponent"));
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
	EnsureDefaultInputMapping();
	ApplyEquipmentWeaponLoadout();
	TrySpawnPet();
}

void ALastFPSHero::ApplyEquipmentWeaponLoadout()
{
	// 서버 권한에서만 장착한다. 클라이언트는 WeaponComponent 복제로 결과를 받는다.
	if (!HasAuthority() || !WeaponComponent)
	{
		return;
	}

	const ALastFPSGameModeBase* GameMode =
		GetWorld() ? GetWorld()->GetAuthGameMode<ALastFPSGameModeBase>() : nullptr;
	if (!GameMode || !GameMode->ShouldEquipWeaponLoadout())
	{
		// 허브처럼 무기를 들지 않는 맵. 이전 맵에서 들고 있던 무기가 남지 않도록 명시적으로 내려놓는다.
		WeaponComponent->UnequipWeapon();
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULastFPSEquipmentSubsystem* Equipment =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSEquipmentSubsystem>() : nullptr;
	if (!Equipment)
	{
		UE_LOG(LogTemp, Error,
			TEXT("%s: EquipmentSubsystem 을 찾지 못해 무기 로드아웃을 장착하지 못했습니다."),
			*GetName());
		return;
	}

	const int32 SlotCount = Equipment->GetSlotCount(ELastFPSEquipmentSlotType::Weapon);
	TArray<ULastFPSWeaponDefinition*> SlotDefinitions;
	SlotDefinitions.Reserve(SlotCount);
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		SlotDefinitions.Add(Equipment->GetWeaponDefinitionForSlot(SlotIndex));
	}

	WeaponComponent->SetWeaponLoadout(SlotDefinitions);
}

void ALastFPSHero::TrySpawnPet()
{
	if (!HasAuthority() || !PetClass)
	{
		return;
	}

	const ALastFPSGameModeBase* GameMode =
		GetWorld() ? GetWorld()->GetAuthGameMode<ALastFPSGameModeBase>() : nullptr;
		
	// 배틀(전투)이 가능한 씬인지 체크
	if (!GameMode || !GameMode->ShouldEquipWeaponLoadout())
	{
		return;
	}

	if (!SpawnedPet)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		// 영웅 왼쪽에서 스폰
		FVector SpawnLocation = GetActorLocation() + (GetActorRightVector() * -150.f);
		SpawnedPet = GetWorld()->SpawnActor<ALastFPSPet>(PetClass, SpawnLocation, GetActorRotation(), SpawnParams);
		if (SpawnedPet)
		{
			if (!SpawnedPet->GetController())
			{
				SpawnedPet->SpawnDefaultController();
			}
			// AIController가 Possess 시 Owner를 덮어쓰므로 별도 변수에 주인 보관
			SpawnedPet->SetOwnerHero(this);
		}
	}
}

void ALastFPSHero::PawnClientRestart()
{
	Super::PawnClientRestart();
	EnsureDefaultInputMapping();
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

	// 속도는 MoveSpeed 어트리뷰트(스탯 테이블 + 버프/디버프 GE)가 전담한다.
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
        WeaponComponent->OnWeaponReloadStarted.AddUniqueDynamic(this, &ALastFPSHero::HandleWeaponReloadStarted);
    }

	ApplyRotationModeSettings();
	BindSpeedBoostCameraTag();
	EnsureDefaultInputMapping();
}

void ALastFPSHero::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindSpeedBoostCameraTag();

    if (WeaponComponent)
    {
        WeaponComponent->OnWeaponEquippedChanged.RemoveDynamic(this, &ALastFPSHero::HandleWeaponEquippedChanged);
        WeaponComponent->OnWeaponReloadStarted.RemoveDynamic(this, &ALastFPSHero::HandleWeaponReloadStarted);
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
        EnsureDefaultInputMapping();
    }
    else
    {
        // 상호작용 진입 시 게임 입력 컨텍스트를 제거해 이동과 전투 입력을 막는다.
        Subsystem->RemoveMappingContext(DefaultMappingContext);
        // ADS 홀드 중 컨텍스트가 제거되면 해제 입력이 오지 않을 수 있어 방어적으로 해제한다.
        SetADS(false);
    }
}

void ALastFPSHero::EnsureDefaultInputMapping()
{
	if (!DefaultMappingContext || !IsLocallyControlled())
	{
		return;
	}

	const APlayerController* PC = Cast<APlayerController>(GetController());
	UEnhancedInputLocalPlayerSubsystem* Subsystem = PC
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
			PC->GetLocalPlayer())
		: nullptr;
	if (Subsystem && !Subsystem->HasMappingContext(DefaultMappingContext))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void ALastFPSHero::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    TickCameraInterp(DeltaTime);
    TickAutoSprint(DeltaTime);
}

bool ALastFPSHero::IsForwardSprintInputAngle() const
{
    // CachedMoveInput 은 카메라 기준 입력. Y = 전방, X = 우측.
    // 캐릭터가 이동 방향으로 회전해도 카메라 기준 입력은 그대로이므로
    // 액터 회전 대신 이 값을 써야 판정이 흔들리지 않는다.
    if (CachedMoveInput.IsNearlyZero())
    {
        return false;
    }

    // atan2(|X|, Y) : 0 = 정면, 90 = 좌우, 180 = 후진
    const float AngleFromForward =
        FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(CachedMoveInput.X), CachedMoveInput.Y));

    return AngleFromForward <= AutoSprintForwardAngle;
}

void ALastFPSHero::TickAutoSprint(float DeltaTime)
{
    if (!bAutoSprintBySpeed)
    {
        return;
    }

    // 권한이 있는 쪽에서만 상태를 바꾼다 (bIsSprinting 은 복제됨)
    if (!IsLocallyControlled() && !HasAuthority())
    {
        return;
    }

    const UCharacterMovementComponent* Movement = GetCharacterMovement();
    if (!Movement)
    {
        return;
    }

    // 사격 등으로 끊긴 직후에는 자동 재진입을 막는다
    if (AutoSprintLockoutTimer > 0.f)
    {
        AutoSprintLockoutTimer = FMath::Max(AutoSprintLockoutTimer - DeltaTime, 0.f);
        if (bIsSprinting)
        {
            SetSprinting(false);
        }
        return;
    }

    FVector Velocity = Movement->Velocity;
    Velocity.Z = 0.f;
    const float GroundSpeed = Velocity.Size();

    // 진입과 해제 임계값을 다르게 두어 경계에서 모션이 떨리지 않게 한다
    const float Threshold = bIsSprinting ? AutoSprintExitSpeed : AutoSprintEnterSpeed;
    bool bShouldSprint = GroundSpeed > Threshold;
    // 조준 · 전투 · 공중에서는 스프린트하지 않는다.
    // 전투 상태를 빼먹으면 사격 중에도 몸이 이동 방향으로 돌아가 조준이 깨진다.
    // 어빌리티가 SetWantsToSprint(false) 를 불러도 bIsSprinting 은 여기서 꺼야 한다.
    if (bIsADS || Movement->IsFalling()
        || CombatState == EMMCombatState::Attacking)
    {
        bShouldSprint = false;
    }

    // 앞 · 앞대각선 입력에서만 스프린트한다. 좌우 · 후진은 제외.
    if (!IsForwardSprintInputAngle())
    {
        bShouldSprint = false;
    }

    if (bShouldSprint)
    {
        AutoSprintExitTimer = 0.f;
        if (!bIsSprinting)
        {
            SetSprinting(true);
        }
        return;
    }

    if (!bIsSprinting)
    {
        AutoSprintExitTimer = 0.f;
        return;
    }

    // 경사나 벽 스침으로 잠깐 느려진 것은 무시하고, 유예 시간이 지나야 해제한다
    AutoSprintExitTimer += DeltaTime;
    if (AutoSprintExitTimer >= AutoSprintExitGrace)
    {
        AutoSprintExitTimer = 0.f;
        SetSprinting(false);
    }
}

void ALastFPSHero::TickCameraInterp(float DeltaTime)
{
	const float AimInterpSpeed = ResolveAimInterpSpeed();
	const float ArmInterpSpeed = !bIsADS && bSpeedBoostCameraTransition
		? SpeedBoostCameraInterpSpeed
		: AimInterpSpeed;
    CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArmLength, DeltaTime, ArmInterpSpeed);
    CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetSocketOffset, DeltaTime, AimInterpSpeed);
    FollowCamera->FieldOfView = FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaTime, AimInterpSpeed);

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

	float AimFOV = ADSFOV;
	if (const FLastFPSWeaponScopeSettings* ScopeSettings = WeaponComponent ? WeaponComponent->GetScopeSettings() : nullptr)
	{
		AimFOV = ScopeSettings->ScopeFOV;
	}
	TargetFOV = bIsADS ? AimFOV : DefaultFOV;
}

void ALastFPSHero::RefreshAimSensitivity()
{
	const FLastFPSWeaponScopeSettings* ScopeSettings =
		WeaponComponent ? WeaponComponent->GetScopeSettings() : nullptr;

	if (!bIsADS || !ScopeSettings || DefaultFOV <= 0.f)
	{
		ActiveAimSensitivityScale = 1.f;
		return;
	}

	const float ZoomRatio = FMath::Clamp(ScopeSettings->ScopeFOV / DefaultFOV, 0.f, 1.f);
	ActiveAimSensitivityScale =
		FMath::Max(ZoomRatio * ScopeSettings->ScopeSensitivityMultiplier, KINDA_SMALL_NUMBER);
}

float ALastFPSHero::ResolveAimInterpSpeed() const
{
	const FLastFPSWeaponScopeSettings* ScopeSettings =
		WeaponComponent ? WeaponComponent->GetScopeSettings() : nullptr;

	if (ScopeSettings && ScopeSettings->ScopeInterpSpeed > 0.f)
	{
		return ScopeSettings->ScopeInterpSpeed;
	}

	return ADSInterpSpeed;
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
        // 스프린트 중에는 방향 전환이 굼뜨지 않도록 회전 속도를 올린다
        Movement->RotationRate = FRotator(0.f, bIsSprinting ? SprintRotationYaw : NormalRotationYaw, 0.f);
    }
}

bool ALastFPSHero::ShouldUseControllerYawRotationMode() const
{
    // 스프린트 중에는 몸이 이동 방향을 향하도록 컨트롤러 Yaw 회전 모드를 해제한다.
    // 전진 스프린트 애니메이션 하나로 대각선 이동까지 자연스럽게 처리하기 위함.
    // (Orientation Warping 은 루트 모션이 있는 시퀀스에서만 동작하므로 그 대안)
    if (bIsSprinting)
    {
        return false;
    }

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

void ALastFPSHero::SelectWeaponSlot(const int32 SlotIndex)
{
    if (!WeaponComponent || !IsAlive())
    {
        return;
    }

    if (SlotIndex == WeaponComponent->GetActiveWeaponSlot())
    {
        return;
    }

    // 리로드 도중 무기를 바꾸면 진행 중인 타이머가 새 무기의 탄창을 채워 버린다.
    // 어빌리티를 먼저 취소해 몽타주·탄창 비주얼·전투 상태까지 EndAbility 경로로 정리한다.
    if (CombatState == EMMCombatState::Reloading)
    {
        CancelAbilityByTag(LastFPSGameplayTags::Ability_Reload);
    }

    WeaponComponent->RequestWeaponSlot(SlotIndex);
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

    // 무기 슬롯 전환은 GAS 어빌리티가 아니라 컴포넌트 상태 변경이므로 네이티브 액션으로 처리한다.
    EIC->BindAction(InputConfig->FindNativeInputActionByTag(LastFPSGameplayTags::Input_WeaponSlot1), ETriggerEvent::Started, this, &ALastFPSHero::SelectWeaponSlot, 0);
    EIC->BindAction(InputConfig->FindNativeInputActionByTag(LastFPSGameplayTags::Input_WeaponSlot2), ETriggerEvent::Started, this, &ALastFPSHero::SelectWeaponSlot, 1);

    // 어빌리티 전용
    for (const FLastFPSInputAction& Action : InputConfig->AbilityInputActions)
    {
        EIC->BindAction(Action.InputAction, ETriggerEvent::Started, this, &ALastFPSHero::HandleAbilityInput, Action.InputTag);
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

    // 자동 모드에서는 입력 방향으로 스프린트를 끊지 않는다.
    // 몸이 이동 방향을 향하도록 회전하므로 대각선·측면 입력도 그대로 유지한다.
    if (!bAutoSprintBySpeed
        && (bIsSprinting || bWantsToSprint) && !HasForwardSprintInput())
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

    // 입력 기반 스프린트에서는 전방 전용으로 제한한다.
    // 자동(속도) 모드에서는 몸이 이동 방향으로 회전하므로 전 방향 입력을 그대로 쓴다.
    // 여기서 좌우를 0으로 만들면 입력 크기가 줄어 감속하고, 순수 좌우 입력은
    // 아예 정지해 버려서 스프린트가 계속 끊긴다.
    if (!bAutoSprintBySpeed && (bIsSprinting || bWantsToSprint))
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
    const FVector2D LookVector = Value.Get<FVector2D>() * ActiveAimSensitivityScale;
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

void ALastFPSHero::HandleWeaponReloadStarted(float /*ReloadDuration*/)
{
    // 재장전 중에는 조준을 유지하지 않는다. 복구는 플레이어가 조준 입력을 다시 넣어야 한다.
    if (bIsADS)
    {
        SetADS(false);
    }
}

void ALastFPSHero::SetADS(bool bEnabled)
{
    // 재장전 중에는 조준 진입 자체를 막는다. 해제는 항상 허용해야 상태가 남지 않는다.
    if (bEnabled && CombatState == EMMCombatState::Reloading)
    {
        return;
    }

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

    if (IsLocallyControlled())
    {
        RefreshAimSensitivity();
        OnAimingChanged.Broadcast(bIsADS);
    }

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
    // 자동 모드에서는 입력 방향을 보지 않는다. 속도로만 판정한다.
    if (bAutoSprintBySpeed)
    {
        return !bIsADS;
    }

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
    RefreshMaxWalkSpeed();   // 스프린트 상태에 맞춰 최대 속도 갱신

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
    // 자동 모드의 해제 처리는 조기 리턴보다 먼저 한다.
    // 어빌리티는 bWantsToSprint 가 이미 false 인 상태에서도 반복 호출하는데,
    // 값이 같다고 리턴해 버리면 bIsSprinting 이 영원히 안 내려간다.
    if (bAutoSprintBySpeed && !bEnabled)
    {
        AutoSprintLockoutTimer = AutoSprintReactivationDelay;

        if (bIsSprinting)
        {
            SetSprinting(false);
        }
    }

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
    RefreshCameraTargets();

    if (IsLocallyControlled())
    {
        RefreshAimSensitivity();
    }
}

void ALastFPSHero::UpdateAliveCollisionState(bool bAlive)
{
    Super::UpdateAliveCollisionState(bAlive);

    if (!bAlive && bIsADS)
    {
        SetADS(false);
    }
}

void ALastFPSHero::Multicast_PlayWeaponFireEffects_Implementation()
{
    if (GetNetMode() == NM_DedicatedServer || IsLocallyControlled()) return;
    if (WeaponComponent) WeaponComponent->PlayFireEffects();
}
