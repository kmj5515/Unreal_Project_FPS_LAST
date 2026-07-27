#include "AbilitySystem/Abilities/GA_BossWeakpointKnockdown.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/LastFPSEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/Enemies/LastFPSBossKnockdownData.h"
#include "Engine/World.h"
#include "Utility/LastFPSTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSBossWeakpointKnockdown, Log, All);

UGA_BossWeakpointKnockdown::UGA_BossWeakpointKnockdown()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer Tags;
	Tags.AddTag(LastFPSGameplayTags::Ability_Enemy_Boss_WeakpointKnockdown);
	SetAssetTags(Tags);

	// GAS가 어빌리티 수명에 맞춰 상태 부여, 기존 공격 취소, 신규 공격 차단을 자동 관리한다.
	ActivationOwnedTags.AddTag(LastFPSGameplayTags::State_Enemy_KnockedDown);
	CancelAbilitiesWithTag.AddTag(LastFPSGameplayTags::Ability_Enemy);
	BlockAbilitiesWithTag.AddTag(LastFPSGameplayTags::Ability_Enemy);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = LastFPSGameplayTags::Event_Enemy_WeakpointBroken;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

bool UGA_BossWeakpointKnockdown::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ALastFPSEnemyCharacter* Enemy =
		ActorInfo ? Cast<ALastFPSEnemyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const UAnimMontage* Montage = KnockdownData ? KnockdownData->KnockdownMontage : nullptr;
	return Enemy
		&& Enemy->IsAlive()
		&& Montage
		&& Montage->GetSectionIndex(KnockdownData->StartSectionName) != INDEX_NONE
		&& Montage->GetSectionIndex(KnockdownData->LoopSectionName) != INDEX_NONE
		&& Montage->GetSectionIndex(KnockdownData->EndSectionName) != INDEX_NONE;
}

void UGA_BossWeakpointKnockdown::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bEndingAbility = false;
	SourceEnemy = Cast<ALastFPSEnemyCharacter>(GetAvatarActorFromActorInfo());
	if (!SourceEnemy || !SourceEnemy->HasAuthority() || !SourceEnemy->IsAlive() || !KnockdownData)
	{
		FinishCurrentAbility(true);
		return;
	}

	if (AAIController* AIController = Cast<AAIController>(SourceEnemy->GetController()))
	{
		AIController->StopMovement();
	}

	if (!StartKnockdownMontage())
	{
		UE_LOG(
			LogLastFPSBossWeakpointKnockdown,
			Error,
			TEXT("약점 넉다운 시작 실패: Enemy=%s, Data=%s, Montage=%s"),
			*GetNameSafe(SourceEnemy),
			*GetNameSafe(KnockdownData),
			*GetNameSafe(KnockdownData ? KnockdownData->KnockdownMontage : nullptr));
		FinishCurrentAbility(true);
	}
}

void UGA_BossWeakpointKnockdown::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	bEndingAbility = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EndSectionRequestTimerHandle);
	}

	if (MontageTask)
	{
		// Super::EndAbility가 소유 태스크를 종료해야 bStopWhenAbilityEnds가 적용되어
		// 취소·사망 경로에서도 재생 중인 몽타주가 확실히 멈춘다.
		MontageTask = nullptr;
	}

	SourceEnemy = nullptr;

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

bool UGA_BossWeakpointKnockdown::StartKnockdownMontage()
{
	if (!SourceEnemy || !KnockdownData || !KnockdownData->KnockdownMontage)
	{
		return false;
	}

	const float PlayRate = FMath::Max(KnockdownData->MontagePlayRate, 0.01f);
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		KnockdownData->KnockdownMontage,
		PlayRate,
		KnockdownData->StartSectionName);
	if (!MontageTask)
	{
		return false;
	}

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->ReadyForActivation();

	// 몽타주 재생 실패 콜백은 즉시 어빌리티를 끝낼 수 있으므로 이후 참조를 다시 검증한다.
	if (bEndingAbility || !SourceEnemy)
	{
		return false;
	}

	if (USkeletalMeshComponent* Mesh = SourceEnemy->GetMesh())
	{
		if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
		{
			AnimInstance->Montage_SetNextSection(
				KnockdownData->StartSectionName,
				KnockdownData->LoopSectionName,
				KnockdownData->KnockdownMontage);
			AnimInstance->Montage_SetNextSection(
				KnockdownData->LoopSectionName,
				KnockdownData->LoopSectionName,
				KnockdownData->KnockdownMontage);
		}
	}

	const float RequestDelay =
		ResolveStartSectionDuration()
		+ FMath::Max(KnockdownData->LoopDuration, 0.f);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			EndSectionRequestTimerHandle,
			this,
			&ThisClass::RequestEndSection,
			FMath::Max(RequestDelay, 0.01f),
			false);
		return true;
	}
	return false;
}

float UGA_BossWeakpointKnockdown::ResolveStartSectionDuration() const
{
	if (!KnockdownData || !KnockdownData->KnockdownMontage)
	{
		return 0.f;
	}

	const int32 SectionIndex =
		KnockdownData->KnockdownMontage->GetSectionIndex(
			KnockdownData->StartSectionName);
	if (SectionIndex == INDEX_NONE)
	{
		return 0.f;
	}

	float StartTime = 0.f;
	float EndTime = 0.f;
	KnockdownData->KnockdownMontage->GetSectionStartAndEndTime(
		SectionIndex,
		StartTime,
		EndTime);
	return FMath::Max(EndTime - StartTime, 0.f)
		/ FMath::Max(KnockdownData->MontagePlayRate, 0.01f);
}

void UGA_BossWeakpointKnockdown::RequestEndSection()
{
	if (bEndingAbility || !SourceEnemy || !SourceEnemy->IsAlive()
		|| !KnockdownData || !KnockdownData->KnockdownMontage)
	{
		FinishCurrentAbility(true);
		return;
	}

	USkeletalMeshComponent* Mesh = SourceEnemy->GetMesh();
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!AnimInstance
		|| !AnimInstance->Montage_IsPlaying(KnockdownData->KnockdownMontage))
	{
		FinishCurrentAbility(true);
		return;
	}

	AnimInstance->Montage_SetNextSection(
		KnockdownData->LoopSectionName,
		KnockdownData->EndSectionName,
		KnockdownData->KnockdownMontage);
}

void UGA_BossWeakpointKnockdown::FinishCurrentAbility(const bool bWasCancelled)
{
	if (bEndingAbility)
	{
		return;
	}

	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		bWasCancelled);
}

void UGA_BossWeakpointKnockdown::OnMontageCompleted()
{
	FinishCurrentAbility(false);
}

void UGA_BossWeakpointKnockdown::OnMontageCancelled()
{
	FinishCurrentAbility(true);
}

void UGA_BossWeakpointKnockdown::OnMontageInterrupted()
{
	FinishCurrentAbility(true);
}
