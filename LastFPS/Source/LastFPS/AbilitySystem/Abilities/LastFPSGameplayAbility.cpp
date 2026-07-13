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

const FLastFPSSkillBalanceData* ULastFPSGameplayAbility::GetSkillBalanceData() const
{
	const ALastFPSCharacterBase* Character = Cast<ALastFPSCharacterBase>(GetAvatarActorFromActorInfo());
	const ULastFPSCharacterDefinition* CharacterDefinition = Character ? Character->GetCharacterDefinition() : nullptr;
	const UGameInstance* GameInstance = Character ? Character->GetGameInstance() : nullptr;
	const ULastFPSSkillDataSubsystem* SkillDataSubsystem =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSSkillDataSubsystem>() : nullptr;
	if (!CharacterDefinition || !SkillDataSubsystem)
	{
		return nullptr;
	}

	const FGameplayTagContainer& CurrentAbilityTags = GetAssetTags();
	const FLastFPSCharacterSkillData* SkillData = SkillDataSubsystem->FindSkillByAbilityTags(
		CharacterDefinition->CharacterId,
		CurrentAbilityTags);
	if (!SkillData)
	{
		return nullptr;
	}

	return SkillDataSubsystem->FindBalance(SkillData->SkillId);
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
