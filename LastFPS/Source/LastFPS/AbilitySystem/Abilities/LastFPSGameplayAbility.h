#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "LastFPSGameplayAbility.generated.h"

class UWorld;
struct FLastFPSSkillBalanceData;

UCLASS(Abstract)
class LASTFPS_API ULastFPSGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	ULastFPSGameplayAbility();

protected:
	/** 현재 Ability의 입력 태그에 연결된 캐릭터 스킬 밸런스 행을 반환한다. */
	const FLastFPSSkillBalanceData* GetSkillBalanceData() const;
	float GetEquippedWeaponBaseDamage() const;

	virtual void DrawDebug(
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayEventData* TriggerEventData) const;

	bool ShouldDrawDebug() const;
	float GetDebugDrawTime() const;
	FColor GetDebugColor() const;
	float GetDebugPointSize() const;
	float GetDebugLineThickness() const;
	UWorld* GetDebugWorld(const FGameplayAbilityActorInfo* ActorInfo) const;

	void DrawDebugPoint(const FGameplayAbilityActorInfo* ActorInfo, const FVector& Location) const;
	void DrawDebugSphere(const FGameplayAbilityActorInfo* ActorInfo, const FVector& Location, float Radius) const;
	void DrawDebugLine(const FGameplayAbilityActorInfo* ActorInfo, const FVector& Start, const FVector& End) const;

	UPROPERTY(EditDefaultsOnly, Category="Debug")
	bool bDrawDebug = false;

	UPROPERTY(EditDefaultsOnly, Category="Debug", meta=(ClampMin="0.0", EditCondition="bDrawDebug"))
	float DebugDrawTime = 2.f;

	UPROPERTY(EditDefaultsOnly, Category="Debug", meta=(EditCondition="bDrawDebug"))
	FLinearColor DebugColor = FLinearColor(0.f, 1.f, 1.f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category="Debug", meta=(ClampMin="1.0", EditCondition="bDrawDebug"))
	float DebugPointSize = 12.f;

	UPROPERTY(EditDefaultsOnly, Category="Debug", meta=(ClampMin="0.0", EditCondition="bDrawDebug"))
	float DebugLineThickness = 2.f;
};
