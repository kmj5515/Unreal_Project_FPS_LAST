#include "AbilitySystem/Abilities/LastFPSActiveGameplayAbility.h"

#include "Components/SceneComponent.h"
#include "GameplayEffect.h"
#include "Data/Tables/LastFPSSkillBalanceData.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSAbilityCooldown, Log, All);

ULastFPSActiveGameplayAbility::ULastFPSActiveGameplayAbility()
{
}

void ULastFPSActiveGameplayAbility::OnAvatarSet(
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	// 아바타가 정해져야 캐릭터 정의를 거쳐 스킬 행을 찾을 수 있다.
	const FLastFPSAbilityCooldownIdentity Identity = ResolveCooldownIdentity();

	CachedCooldownTags.Reset();
	CachedCooldownSeconds = 0.f;

	if (!Identity.IsValid())
	{
		// 쿨다운 GE 를 지정하지 않은 어빌리티(기본 사격·질주·재장전·점프·그래플링 등)는
		// 쿨다운 자체가 없는 것이 정상이다. 데이터가 없다고 알릴 대상이 아니다.
		// 판정 기준을 ApplyCooldown 의 가드와 같은 것(쿨다운 GE 지정 여부)으로 맞춘다.
		if (GetCooldownGameplayEffect() == nullptr)
		{
			return;
		}

		// 여기부터는 쿨다운을 걸겠다고 선언해 놓고 데이터가 없는 경우다.
		// 조용히 넘어가면 쿨다운 없이 무한 시전이 되므로 원인을 남긴다.
		// 스킬 테이블은 부팅 시 CooldownTag 누락을 이미 검증하므로, 여기 걸리면 밸런스 행 쪽 문제다.
		UE_LOG(LogLastFPSAbilityCooldown, Error,
			TEXT("어빌리티 '%s' 의 쿨다운 신원을 데이터에서 찾지 못했습니다. 스킬/밸런스 행을 확인하십시오."),
			*GetName());
		return;
	}

	CachedCooldownTags.AddTag(Identity.CooldownTag);
	CachedCooldownSeconds = Identity.CooldownSeconds;
}

const FGameplayTagContainer* ULastFPSActiveGameplayAbility::GetCooldownTags() const
{
	// 데이터 해석에 실패한 경우에만 엔진 기본(쿨다운 GE 클래스의 태그) 경로로 물러난다.
	return CachedCooldownTags.IsEmpty() ? Super::GetCooldownTags() : &CachedCooldownTags;
}

void ULastFPSActiveGameplayAbility::PlayActivationSound() const
{
	if (!ActivationSound)
	{
		return;
	}

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	AActor* AvatarActor =
		ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!ActorInfo || !ActorInfo->IsLocallyControlled() || !AvatarActor)
	{
		return;
	}

	USceneComponent* AttachComponent = AvatarActor->GetRootComponent();
	if (!AttachComponent)
	{
		return;
	}

	UGameplayStatics::SpawnSoundAttached(
		ActivationSound,
		AttachComponent,
		NAME_None,
		FVector::ZeroVector,
		EAttachLocation::KeepRelativeOffset,
		true,
		FMath::Max(ActivationSoundVolumeMultiplier, 0.f),
		FMath::Max(ActivationSoundPitchMultiplier, 0.01f));
}

void ULastFPSActiveGameplayAbility::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownEffect = GetCooldownGameplayEffect();
	if (!CooldownEffect)
	{
		return; // 쿨다운을 쓰지 않는 어빌리티.
	}

	// 대체값을 만들지 않는다. 데이터가 없으면 쿨다운을 걸지 않고 원인을 남긴다.
	// (예전에는 GE 클래스에 박힌 리터럴로 조용히 물러나 밸런스 수정이 삼켜졌다.)
	if (CachedCooldownTags.IsEmpty() || CachedCooldownSeconds <= 0.f)
	{
		UE_LOG(LogLastFPSAbilityCooldown, Error,
			TEXT("어빌리티 '%s' 에 적용할 쿨다운 데이터가 없어 쿨다운을 걸지 않았습니다."),
			*GetName());
		return;
	}

	FGameplayEffectSpecHandle CooldownSpec = MakeOutgoingGameplayEffectSpec(
		Handle,
		ActorInfo,
		ActivationInfo,
		CooldownEffect->GetClass(),
		GetAbilityLevel(Handle, ActorInfo));
	if (!CooldownSpec.IsValid())
	{
		UE_LOG(LogLastFPSAbilityCooldown, Error,
			TEXT("어빌리티 '%s' 의 쿨다운 스펙 생성에 실패했습니다."),
			*GetName());
		return;
	}

	// 태그와 시간 모두 스펙에 싣는다. 공유 쿨다운 GE 클래스는 형태만 제공하고 값은 갖지 않는다.
	// DynamicGrantedTags 는 Def 의 부여 태그에 더해지므로, 공유 클래스는 태그를 부여하지 않아야 한다.
	CooldownSpec.Data->DynamicGrantedTags.AppendTags(CachedCooldownTags);
	CooldownSpec.Data->SetDuration(CachedCooldownSeconds, true);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpec);
}
