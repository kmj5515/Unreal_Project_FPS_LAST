#include "Debug/GasInspector/LastFPSGasSnapshotBuilder.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"

FLastFPSGasSnapshot FLastFPSGasSnapshotBuilder::BuildFromActor(const AActor* Actor)
{
	if (!Actor)
	{
		return FLastFPSGasSnapshot();
	}

	// IAbilitySystemInterface 또는 전역 헬퍼로 ASC를 해석한다. 구체 캐릭터 타입에 의존하지 않는다.
	UAbilitySystemComponent* AbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);

	return BuildFromAbilitySystem(AbilitySystem, Actor->GetActorNameOrLabel(), Actor->GetClass()->GetName());
}

FLastFPSGasSnapshot FLastFPSGasSnapshotBuilder::BuildFromAbilitySystem(
UAbilitySystemComponent* AbilitySystem,
	const FString& DisplayName,
	const FString& OwnerClassName)
{
	FLastFPSGasSnapshot Snapshot;
	Snapshot.DisplayName = DisplayName;
	Snapshot.OwnerClassName = OwnerClassName;

	if (!AbilitySystem)
	{
		return Snapshot;
	}

	Snapshot.bValid = true;

	// ── 어트리뷰트: 이름/베이스/현재값 ──────────────────────────────
	{
		TArray<FGameplayAttribute> Attributes;
		AbilitySystem->GetAllAttributes(Attributes);
		Snapshot.Attributes.Reserve(Attributes.Num());
		for (const FGameplayAttribute& Attribute : Attributes)
		{
			if (!Attribute.IsValid())
			{
				continue;
			}

			FLastFPSGasAttributeSnapshot Entry;
			Entry.Name = Attribute.GetName();
			Entry.BaseValue = AbilitySystem->GetNumericAttributeBase(Attribute);
			Entry.CurrentValue = AbilitySystem->GetNumericAttribute(Attribute);
			Snapshot.Attributes.Add(MoveTemp(Entry));
		}

		// 이름순 정렬로 프레임마다 순서가 흔들리지 않게 한다.
		Snapshot.Attributes.Sort([](const FLastFPSGasAttributeSnapshot& A, const FLastFPSGasAttributeSnapshot& B)
		{
			return A.Name < B.Name;
		});
	}

	// ── 활성 게임플레이 이펙트: 남은 시간/스택 ───────────────────────
	{
		const UWorld* World = AbilitySystem->GetWorld();
		const float WorldTime = World ? World->GetTimeSeconds() : 0.f;

		const FGameplayEffectQuery Query;
		const TArray<FActiveGameplayEffectHandle> Handles = AbilitySystem->GetActiveEffects(Query);
		Snapshot.Effects.Reserve(Handles.Num());
		for (const FActiveGameplayEffectHandle& Handle : Handles)
		{
			const FActiveGameplayEffect* ActiveEffect = AbilitySystem->GetActiveGameplayEffect(Handle);
			if (!ActiveEffect)
			{
				continue;
			}

			FLastFPSGasEffectSnapshot Entry;
			Entry.Name = GetNameSafe(ActiveEffect->Spec.Def);
			Entry.Duration = ActiveEffect->GetDuration();
			Entry.TimeRemaining = ActiveEffect->GetTimeRemaining(WorldTime);
			Entry.StackCount = ActiveEffect->Spec.GetStackCount();
			Snapshot.Effects.Add(MoveTemp(Entry));
		}
	}

	// ── 보유 게임플레이 태그 ─────────────────────────────────────────
	{
		FGameplayTagContainer OwnedTags;
		AbilitySystem->GetOwnedGameplayTags(OwnedTags);
		Snapshot.OwnedTags.Reserve(OwnedTags.Num());
		for (const FGameplayTag& Tag : OwnedTags)
		{
			Snapshot.OwnedTags.Add(Tag.ToString());
		}
		Snapshot.OwnedTags.Sort();
	}

	// ── 부여된 어빌리티: 이름/활성 여부 ──────────────────────────────
	{
		const TArray<FGameplayAbilitySpec>& Abilities = AbilitySystem->GetActivatableAbilities();
		Snapshot.Abilities.Reserve(Abilities.Num());
		for (const FGameplayAbilitySpec& Spec : Abilities)
		{
			FLastFPSGasAbilitySnapshot Entry;
			Entry.Name = GetNameSafe(Spec.Ability);
			Entry.bActive = Spec.IsActive();
			Snapshot.Abilities.Add(MoveTemp(Entry));
		}

		Snapshot.Abilities.Sort([](const FLastFPSGasAbilitySnapshot& A, const FLastFPSGasAbilitySnapshot& B)
		{
			return A.Name < B.Name;
		});
	}

	return Snapshot;
}
