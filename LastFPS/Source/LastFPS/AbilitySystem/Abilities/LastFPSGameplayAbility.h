#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "LastFPSGameplayAbility.generated.h"

class UWorld;
struct FLastFPSCharacterSkillData;
struct FLastFPSSkillBalanceData;

/**
 * 쿨다운 1건의 신원 — 어떤 태그로, 몇 초를 잠글 것인가.
 *
 * 태그는 스킬 정의 행(FLastFPSCharacterSkillData::CooldownTag), 시간은 밸런스 행이 소유한다.
 * 두 값이 항상 같은 조회에서 나와야 어긋나지 않으므로 하나로 묶어 반환한다.
 */
struct FLastFPSAbilityCooldownIdentity
{
	FGameplayTag CooldownTag;
	float CooldownSeconds = 0.f;

	bool IsValid() const { return CooldownTag.IsValid() && CooldownSeconds > 0.f; }
};

UCLASS(Abstract)
class LASTFPS_API ULastFPSGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	ULastFPSGameplayAbility();

protected:
	/** 현재 Ability의 입력 태그에 연결된 캐릭터 스킬 밸런스 행을 반환한다. */
	const FLastFPSSkillBalanceData* GetSkillBalanceData() const;

	/**
	 * 이 Ability의 쿨다운 신원(태그 + 시간)을 데이터에서 해석한다.
	 * 해석에 실패하면 무효 신원을 돌려준다 — 호출부가 대체값을 만들지 않도록 조용히 성공시키지 않는다.
	 */
	FLastFPSAbilityCooldownIdentity ResolveCooldownIdentity() const;
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
