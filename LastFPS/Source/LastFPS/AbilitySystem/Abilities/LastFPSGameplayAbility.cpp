#include "AbilitySystem/Abilities/LastFPSGameplayAbility.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Character/LastFPSCharacterBase.h"
#include "Character/Components/WeaponComponent.h"
#include "Character/Interfaces/LastFPSWeaponUser.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Data/Tables/LastFPSCharacterSkillData.h"
#include "Data/Tables/LastFPSSkillBalanceData.h"
#include "Engine/GameInstance.h"
#include "Skills/LastFPSSkillDataSubsystem.h"

ULastFPSGameplayAbility::ULastFPSGameplayAbility()
{
}

namespace
{
	/**
	 * 아바타 → 캐릭터 정의 → 스킬 행 조회를 한 곳에 모은다.
	 * 밸런스 조회와 쿨다운 신원 조회가 반드시 같은 행에서 나오도록 강제하기 위한 것이다.
	 */
	struct FLastFPSSkillLookup
	{
		const ULastFPSSkillDataSubsystem* Subsystem = nullptr;
		const FLastFPSCharacterSkillData* SkillRow = nullptr;

		bool IsValid() const { return Subsystem != nullptr && SkillRow != nullptr; }
	};

	FLastFPSSkillLookup FindSkillRow(const AActor* AvatarActor, const FGameplayTagContainer& AbilityTags)
	{
		const ALastFPSCharacterBase* Character = Cast<ALastFPSCharacterBase>(AvatarActor);
		const ULastFPSCharacterDefinition* CharacterDefinition =
			Character ? Character->GetCharacterDefinition() : nullptr;
		const UGameInstance* GameInstance = Character ? Character->GetGameInstance() : nullptr;
		const ULastFPSSkillDataSubsystem* SkillDataSubsystem =
			GameInstance ? GameInstance->GetSubsystem<ULastFPSSkillDataSubsystem>() : nullptr;
		if (!CharacterDefinition || !SkillDataSubsystem)
		{
			return FLastFPSSkillLookup();
		}

		FLastFPSSkillLookup Lookup;
		Lookup.Subsystem = SkillDataSubsystem;
		Lookup.SkillRow = SkillDataSubsystem->FindSkillByAbilityTags(
			CharacterDefinition->CharacterId,
			AbilityTags);
		return Lookup.SkillRow ? Lookup : FLastFPSSkillLookup();
	}
}

const FLastFPSSkillBalanceData* ULastFPSGameplayAbility::GetSkillBalanceData() const
{
	const FLastFPSSkillLookup Lookup = FindSkillRow(GetAvatarActorFromActorInfo(), GetAssetTags());
	return Lookup.IsValid() ? Lookup.Subsystem->FindBalance(Lookup.SkillRow->SkillId) : nullptr;
}

FLastFPSAbilityCooldownIdentity ULastFPSGameplayAbility::ResolveCooldownIdentity() const
{
	FLastFPSAbilityCooldownIdentity Identity;

	const FLastFPSSkillLookup Lookup = FindSkillRow(GetAvatarActorFromActorInfo(), GetAssetTags());
	if (!Lookup.IsValid())
	{
		return Identity;
	}

	const FLastFPSSkillBalanceData* Balance = Lookup.Subsystem->FindBalance(Lookup.SkillRow->SkillId);
	if (!Balance)
	{
		return Identity;
	}

	// 태그와 시간을 같은 조회에서 함께 꺼낸다. 둘이 다른 경로로 오면 어긋날 수 있다.
	Identity.CooldownTag = Lookup.SkillRow->CooldownTag;
	Identity.CooldownSeconds = Balance->Cooldown;
	return Identity;
}

float ULastFPSGameplayAbility::GetEquippedWeaponBaseDamage() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const ILastFPSWeaponUser* WeaponUser = Cast<ILastFPSWeaponUser>(AvatarActor);
	const UWeaponComponent* WeaponComponent = WeaponUser ? WeaponUser->GetWeaponComponent() : nullptr;
	return WeaponComponent ? WeaponComponent->GetWeaponBaseDamage() : 0.f;
}

void ULastFPSGameplayAbility::DrawDebug(
	const FGameplayAbilityActorInfo*,
	const FGameplayEventData*) const
{
}

bool ULastFPSGameplayAbility::ShouldDrawDebug() const
{
	return bDrawDebug;
}

float ULastFPSGameplayAbility::GetDebugDrawTime() const
{
	return DebugDrawTime;
}

FColor ULastFPSGameplayAbility::GetDebugColor() const
{
	return DebugColor.ToFColor(true);
}

float ULastFPSGameplayAbility::GetDebugPointSize() const
{
	return DebugPointSize;
}

float ULastFPSGameplayAbility::GetDebugLineThickness() const
{
	return DebugLineThickness;
}

UWorld* ULastFPSGameplayAbility::GetDebugWorld(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		return ActorInfo->AvatarActor->GetWorld();
	}

	return GetWorld();
}

void ULastFPSGameplayAbility::DrawDebugPoint(
	const FGameplayAbilityActorInfo* ActorInfo,
	const FVector& Location) const
{
#if ENABLE_DRAW_DEBUG
	if (!ShouldDrawDebug())
	{
		return;
	}

	UWorld* World = GetDebugWorld(ActorInfo);
	if (!World)
	{
		return;
	}

	::DrawDebugPoint(
		World,
		Location,
		GetDebugPointSize(),
		GetDebugColor(),
		false,
		GetDebugDrawTime());
#endif
}

void ULastFPSGameplayAbility::DrawDebugSphere(
	const FGameplayAbilityActorInfo* ActorInfo,
	const FVector& Location,
	float Radius) const
{
#if ENABLE_DRAW_DEBUG
	if (!ShouldDrawDebug() || Radius <= 0.f)
	{
		return;
	}

	UWorld* World = GetDebugWorld(ActorInfo);
	if (!World)
	{
		return;
	}

	::DrawDebugSphere(
		World,
		Location,
		Radius,
		24,
		GetDebugColor(),
		false,
		GetDebugDrawTime(),
		0,
		GetDebugLineThickness());
#endif
}

void ULastFPSGameplayAbility::DrawDebugLine(
	const FGameplayAbilityActorInfo* ActorInfo,
	const FVector& Start,
	const FVector& End) const
{
#if ENABLE_DRAW_DEBUG
	if (!ShouldDrawDebug())
	{
		return;
	}

	UWorld* World = GetDebugWorld(ActorInfo);
	if (!World)
	{
		return;
	}

	::DrawDebugLine(
		World,
		Start,
		End,
		GetDebugColor(),
		false,
		GetDebugDrawTime(),
		0,
		GetDebugLineThickness());
#endif
}
