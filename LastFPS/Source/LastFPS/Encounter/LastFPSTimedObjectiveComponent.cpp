#include "Encounter/LastFPSTimedObjectiveComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Character/AI/LastFPSCombatTargetRegistry.h"
#include "Character/LastFPSCharacterBase.h"
#include "Components/ShapeComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
// TSubclassOf<UGameplayEffect> 유효성 검사와 MakeOutgoingSpec 에 완전 타입이 필요하다.
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "Quest/LastFPSQuestSubsystem.h"
#include "TimerManager.h"
#include "UI/HUD/LastFPSObjectiveHudSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSObjective, Log, All);

ULastFPSTimedObjectiveComponent::ULastFPSTimedObjectiveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULastFPSTimedObjectiveComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 클라이언트가 게이지·라벨·결과를 계산하는 데 필요한 것만 복제한다.
	// 진행 조건(RequiredVolume/bBlockedByEnemies)은 서버 판정 전용이라 제외한다.
	DOREPLIFETIME(ULastFPSTimedObjectiveComponent, Settings);
	DOREPLIFETIME(ULastFPSTimedObjectiveComponent, FailureWatchTarget);
	DOREPLIFETIME(ULastFPSTimedObjectiveComponent, bWatchTargetAssigned);
	DOREPLIFETIME(ULastFPSTimedObjectiveComponent, ElapsedSeconds);
	DOREPLIFETIME(ULastFPSTimedObjectiveComponent, Result);
	DOREPLIFETIME(ULastFPSTimedObjectiveComponent, bActive);
}

void ULastFPSTimedObjectiveComponent::ApplySettings(const FLastFPSTimedObjectiveSettings& InSettings)
{
	Settings = InSettings;
}

void ULastFPSTimedObjectiveComponent::SetRequiredVolume(UShapeComponent* InVolume)
{
	RequiredVolume = InVolume;
}

void ULastFPSTimedObjectiveComponent::SetFailureWatchTarget(AActor* InTarget)
{
	FailureWatchTarget = InTarget;
	bWatchTargetAssigned = InTarget != nullptr;
}

void ULastFPSTimedObjectiveComponent::SetTargetInitEffect(TSubclassOf<UGameplayEffect> InEffect)
{
	TargetInitEffect = InEffect;
}

void ULastFPSTimedObjectiveComponent::SetTargetMaxHealth(const float InMaxHealth)
{
	TargetMaxHealth = InMaxHealth;
}

void ULastFPSTimedObjectiveComponent::SetBlockedByEnemies(const bool bInBlocked)
{
	bBlockedByEnemies = bInBlocked;
}

void ULastFPSTimedObjectiveComponent::StartObjective()
{
	if (!HasAuthority() || bActive || IsResolved())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	bActive = true;
	ElapsedSeconds = 0.f;
	ResetWatchTarget();
	BindFailureWatch();

	// 지킬 대상은 Pawn 이 아니라 퍼셉션에 잡히지 않는다 — 적대 타깃으로 직접 등록한다.
	if (IsValid(FailureWatchTarget))
	{
		if (ULastFPSCombatTargetRegistry* Registry = World->GetSubsystem<ULastFPSCombatTargetRegistry>())
		{
			Registry->RegisterTarget(*FailureWatchTarget);
		}
	}

	World->GetTimerManager().SetTimer(
		ObjectiveTimerHandle,
		this,
		&ULastFPSTimedObjectiveComponent::TickObjective,
		FMath::Max(Settings.UpdateInterval, 0.05f),
		true);

	OnProgressChanged.Broadcast(GetProgress01());
	RefreshHudPresentation();
	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
}

void ULastFPSTimedObjectiveComponent::StopObjective()
{
	bActive = false;
	RefreshHudPresentation();
	UnbindFailureWatch();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ObjectiveTimerHandle);

		if (IsValid(FailureWatchTarget))
		{
			if (ULastFPSCombatTargetRegistry* Registry = World->GetSubsystem<ULastFPSCombatTargetRegistry>())
			{
				Registry->UnregisterTarget(*FailureWatchTarget);
			}
		}
	}
}

#if !UE_BUILD_SHIPPING
void ULastFPSTimedObjectiveComponent::DebugForceSucceed()
{
	if (!HasAuthority() || IsResolved())
	{
		return;
	}

	// 게이지도 가득 채워 HUD·마커 표시가 실제 성공과 같아지도록 한다.
	ElapsedSeconds = Settings.Duration;
	Resolve(ELastFPSObjectiveResult::Succeeded);
}
#endif

float ULastFPSTimedObjectiveComponent::GetProgress01() const
{
	return Settings.Duration > 0.f
		? FMath::Clamp(ElapsedSeconds / Settings.Duration, 0.f, 1.f)
		: 0.f;
}

void ULastFPSTimedObjectiveComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopObjective();
	// StopObjective 가 이미 반납하지만, 파괴 경로에서 슬롯이 남지 않도록 한 번 더 확인한다.
	ReleaseHudPresentation();
	Super::EndPlay(EndPlayReason);
}

void ULastFPSTimedObjectiveComponent::TickObjective()
{
	if (!HasAuthority() || !bActive || IsResolved())
	{
		return;
	}

	// 실패는 진행보다 먼저 본다 — 마지막 주기에 동시 성립하면 실패가 우선이다.
	// 델리게이트(체력 0·파괴)가 놓치는 소실 경로가 있으므로 폴링으로도 한 번 더 확인한다.
	// 그렇지 않으면 목표가 영원히 미해결로 남아 웨이브가 무한 순환한다.
	if (bWatchTargetAssigned)
	{
		if (!IsValid(FailureWatchTarget))
		{
			Resolve(ELastFPSObjectiveResult::Failed);
			return;
		}

		if (const UAbilitySystemComponent* ASC = GetWatchTargetAbilitySystem())
		{
			if (ASC->GetNumericAttribute(ULastFPSAttributeSet::GetHealthAttribute()) <= 0.f)
			{
				Resolve(ELastFPSObjectiveResult::Failed);
				return;
			}
		}
	}

	if (!ShouldAdvance())
	{
		return;
	}

	ElapsedSeconds = FMath::Min(
		ElapsedSeconds + FMath::Max(Settings.UpdateInterval, 0.05f),
		Settings.Duration);
	OnProgressChanged.Broadcast(GetProgress01());
	// 서버는 OnRep 이 오지 않으므로 재시도도 여기서 함께 한다.
	RefreshHudPresentation();

	if (ElapsedSeconds >= Settings.Duration)
	{
		Resolve(ELastFPSObjectiveResult::Succeeded);
	}
}

bool ULastFPSTimedObjectiveComponent::ShouldAdvance() const
{
	// 진행 조건이 없으면 시작한 이후로 계속 흐른다(방어형).
	if (!IsValid(RequiredVolume))
	{
		return true;
	}

	TArray<AActor*> OverlappingActors;
	RequiredVolume->GetOverlappingActors(OverlappingActors, APawn::StaticClass());

	bool bPlayerInside = false;
	for (const AActor* OverlappingActor : OverlappingActors)
	{
		const APawn* Pawn = Cast<APawn>(OverlappingActor);
		if (!Pawn)
		{
			continue;
		}

		if (Pawn->IsPlayerControlled())
		{
			bPlayerInside = true;
			continue;
		}

		// 적 경합 — 구역을 다투는 동안에는 진행을 멈춘다.
		if (bBlockedByEnemies)
		{
			const ALastFPSCharacterBase* Character = Cast<ALastFPSCharacterBase>(Pawn);
			if (Character && Character->IsAlive())
			{
				return false;
			}
		}
	}

	return bPlayerInside;
}

void ULastFPSTimedObjectiveComponent::Resolve(const ELastFPSObjectiveResult NewResult)
{
	if (!HasAuthority() || IsResolved() || NewResult == ELastFPSObjectiveResult::None)
	{
		return;
	}

	Result = NewResult;
	StopObjective();
	HandleResolved();

	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
}

void ULastFPSTimedObjectiveComponent::HandleResolved()
{
	if (bResolvedBroadcast || !IsResolved())
	{
		return;
	}
	bResolvedBroadcast = true;

	// 퀘스트 진행은 각 머신의 로컬 서브시스템이 소유하므로 서버·클라 양쪽에서 통지한다.
	if (Result == ELastFPSObjectiveResult::Succeeded && Settings.ZoneTag.IsValid())
	{
		if (ULastFPSQuestSubsystem* Quest = GetQuestSubsystem())
		{
			Quest->NotifyTaggedObjective(Settings.QuestObjectiveType, Settings.ZoneTag);
		}
	}

	UE_LOG(
		LogLastFPSObjective,
		Log,
		TEXT("[Objective] 목표 확정 (Owner=%s, Zone=%s, Result=%s)"),
		*GetNameSafe(GetOwner()),
		*Settings.ZoneTag.ToString(),
		Result == ELastFPSObjectiveResult::Succeeded ? TEXT("성공") : TEXT("실패"));

	OnObjectiveResolved.Broadcast(this, Result);
}

void ULastFPSTimedObjectiveComponent::OnRep_ElapsedSeconds()
{
	OnProgressChanged.Broadcast(GetProgress01());
	// 선점된 슬롯이 비었을 수 있으므로 진행 갱신마다 재시도한다.
	// 요청이 한 번 거부되면 그대로 포기하는 문제를 여기서 흡수한다.
	RefreshHudPresentation();
}

void ULastFPSTimedObjectiveComponent::OnRep_Result()
{
	HandleResolved();
	RefreshHudPresentation();
}

void ULastFPSTimedObjectiveComponent::OnRep_Settings()
{
	// 활성 플래그가 설정보다 먼저 도착했다면 이제서야 표시 모드를 알게 된다.
	RefreshHudPresentation();
}

void ULastFPSTimedObjectiveComponent::OnRep_Active()
{
	RefreshHudPresentation();
}

void ULastFPSTimedObjectiveComponent::RefreshHudPresentation()
{
	const bool bShouldPresent =
		bActive
		&& !IsResolved()
		&& Settings.HudMode != ELastFPSObjectiveHudMode::None;

	if (bShouldPresent)
	{
		RequestHudPresentation();
	}
	else
	{
		ReleaseHudPresentation();
	}
}

void ULastFPSTimedObjectiveComponent::RequestHudPresentation()
{
	if (bHudPresentationActive)
	{
		return;
	}

	ULastFPSObjectiveHudSubsystem* Hud = GetHudSubsystem();
	if (!Hud)
	{
		return;
	}

	bHudPresentationActive = Hud->RequestPresentation(Settings.HudMode, this);
}

void ULastFPSTimedObjectiveComponent::ReleaseHudPresentation()
{
	if (!bHudPresentationActive)
	{
		return;
	}
	bHudPresentationActive = false;

	if (ULastFPSObjectiveHudSubsystem* Hud = GetHudSubsystem())
	{
		Hud->ReleasePresentation(this);
	}
}

ULastFPSObjectiveHudSubsystem* ULastFPSTimedObjectiveComponent::GetHudSubsystem() const
{
	UWorld* World = GetWorld();
	return World ? World->GetSubsystem<ULastFPSObjectiveHudSubsystem>() : nullptr;
}

float ULastFPSTimedObjectiveComponent::GetWatchTargetHealth01() const
{
	const UAbilitySystemComponent* ASC = GetWatchTargetAbilitySystem();
	if (!ASC)
	{
		return -1.f;
	}

	const float MaxHealth = ASC->GetNumericAttribute(ULastFPSAttributeSet::GetMaxHealthAttribute());
	if (MaxHealth <= 0.f)
	{
		return -1.f;
	}

	const float Health = ASC->GetNumericAttribute(ULastFPSAttributeSet::GetHealthAttribute());
	return FMath::Clamp(Health / MaxHealth, 0.f, 1.f);
}

void ULastFPSTimedObjectiveComponent::BindFailureWatch()
{
	if (!HasAuthority() || !IsValid(FailureWatchTarget))
	{
		return;
	}

	FailureWatchTarget->OnDestroyed.AddUniqueDynamic(
		this,
		&ULastFPSTimedObjectiveComponent::HandleWatchTargetDestroyed);

	// 체력 개념이 있는 대상(GAS)은 0 도달을 즉시 잡는다. 없으면 파괴 이벤트만 본다.
	if (UAbilitySystemComponent* ASC = GetWatchTargetAbilitySystem())
	{
		WatchTargetHealthHandle = ASC
			->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetHealthAttribute())
			.AddUObject(this, &ULastFPSTimedObjectiveComponent::HandleWatchTargetHealthChanged);
	}
}

void ULastFPSTimedObjectiveComponent::UnbindFailureWatch()
{
	if (IsValid(FailureWatchTarget))
	{
		FailureWatchTarget->OnDestroyed.RemoveDynamic(
			this,
			&ULastFPSTimedObjectiveComponent::HandleWatchTargetDestroyed);
	}

	if (WatchTargetHealthHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetWatchTargetAbilitySystem())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetHealthAttribute())
				.Remove(WatchTargetHealthHandle);
		}
		WatchTargetHealthHandle.Reset();
	}
}

void ULastFPSTimedObjectiveComponent::HandleWatchTargetHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!HasAuthority() || !bActive || Data.NewValue > 0.f)
	{
		return;
	}

	Resolve(ELastFPSObjectiveResult::Failed);
}

void ULastFPSTimedObjectiveComponent::HandleWatchTargetDestroyed(AActor* /*DestroyedActor*/)
{
	if (!HasAuthority() || !bActive)
	{
		return;
	}

	Resolve(ELastFPSObjectiveResult::Failed);
}

void ULastFPSTimedObjectiveComponent::ResetWatchTarget()
{
	if (!HasAuthority() || !IsValid(FailureWatchTarget))
	{
		return;
	}

	if (!TargetInitEffect && TargetMaxHealth <= 0.f)
	{
		// 실패 감시 대상이 없는 목표(점령 등)는 초기화할 것이 없다 — 정상 경로다.
		return;
	}

	// 초기화에 실패하면 체력이 0 인 채로 시작해 첫 주기에 즉시 실패로 확정된다.
	// 원인 추적이 불가능해지므로 조용히 넘기지 않는다.
	UAbilitySystemComponent* ASC = GetWatchTargetAbilitySystem();
	if (!ASC)
	{
		UE_LOG(
			LogLastFPSObjective,
			Error,
			TEXT("[Objective] 감시 대상에 AbilitySystemComponent 가 없어 체력을 초기화하지 못했습니다: 대상=%s"),
			*GetNameSafe(FailureWatchTarget));
		return;
	}

	// 초기화 효과가 없으면 어트리뷰트를 직접 세팅한다.
	// 대상이 저항·감쇠 같은 부가 스탯을 쓰지 않는 단순 장치라면 이 경로로 충분하다.
	if (!TargetInitEffect)
	{
		ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetMaxHealthAttribute(), TargetMaxHealth);
		ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetHealthAttribute(), TargetMaxHealth);
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(TargetInitEffect, 1.f, Context);
	if (!Spec.IsValid())
	{
		UE_LOG(
			LogLastFPSObjective,
			Error,
			TEXT("[Objective] 초기화 효과 Spec 생성에 실패했습니다: 대상=%s, 효과=%s"),
			*GetNameSafe(FailureWatchTarget),
			*GetNameSafe(TargetInitEffect));
		return;
	}

	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

bool ULastFPSTimedObjectiveComponent::HasAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

UAbilitySystemComponent* ULastFPSTimedObjectiveComponent::GetWatchTargetAbilitySystem() const
{
	if (!IsValid(FailureWatchTarget))
	{
		return nullptr;
	}

	const IAbilitySystemInterface* AsAbilitySystem = Cast<IAbilitySystemInterface>(FailureWatchTarget);
	return AsAbilitySystem ? AsAbilitySystem->GetAbilitySystemComponent() : nullptr;
}

ULastFPSQuestSubsystem* ULastFPSTimedObjectiveComponent::GetQuestSubsystem() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<ULastFPSQuestSubsystem>() : nullptr;
}
