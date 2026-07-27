#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Pooling/LastFPSPoolableActor.h"
#include "TimerManager.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "LastFPSExpandingMeshAttackActor.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;

/** Niagara 링의 시각 표현과 서버 권한 충돌 판정을 구성한다. */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSExpandingMeshAttackConfig
{
	GENERATED_BODY()

	/** 확장 링을 표시하는 Niagara 시스템이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|VFX")
	TObjectPtr<UNiagaraSystem> EffectNiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|VFX")
	FName StartRadiusOffsetNiagaraParameterName = TEXT("User.StartRadiusOffset");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|VFX")
	FName EndRadiusOffsetNiagaraParameterName = TEXT("User.EndRadiusOffset");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|VFX")
	FName ExpansionDurationNiagaraParameterName = TEXT("User.ExpansionDuration");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|VFX")
	FName ExpansionAlphaNiagaraParameterName = TEXT("User.ExpansionAlpha");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|VFX")
	FName RingThicknessNiagaraParameterName = TEXT("User.RingThickness");

	/** Niagara Mesh Renderer에 지정한 원본 도넛의 바깥 반지름이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|VFX", meta=(ClampMin="0.01", Units="cm"))
	float MeshBaseOuterRadius = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|Expansion", meta=(ClampMin="0.01", Units="cm"))
	float StartOuterRadius = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|Expansion", meta=(ClampMin="0.01", Units="cm"))
	float EndOuterRadius = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|Expansion", meta=(ClampMin="0.01", Units="s"))
	float ExpansionDuration = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|Expansion", meta=(ClampMin="0.0", Units="s"))
	float LifeAfterExpansion = 0.1f;

	/** 확장 중에도 일정하게 유지되는 공격 링의 방사 방향 두께다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|Collision", meta=(ClampMin="0.01", Units="cm"))
	float RingThickness = 100.f;

	/** 지면 위아래로 공격 판정을 허용하는 높이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|Collision", meta=(ClampMin="0.01", Units="cm"))
	float HitHalfHeight = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|Damage")
	FLastFPSDamageRange DamageRange;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|Target Effects")
	TArray<TSubclassOf<UGameplayEffect>> TargetEffects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|Condition")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|Condition")
	FGameplayTagContainer BlockedTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|Condition")
	bool bIgnoreFriendlyTargets = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|Debug")
	bool bDrawDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|Debug", meta=(EditCondition="bDrawDebug"))
	FLinearColor DebugOuterColor = FLinearColor::Red;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|Debug", meta=(EditCondition="bDrawDebug"))
	FLinearColor DebugInnerColor = FLinearColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|Debug", meta=(ClampMin="0.1", EditCondition="bDrawDebug"))
	float DebugLineThickness = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Expanding Mesh|Debug", meta=(ClampMin="12", ClampMax="256", EditCondition="bDrawDebug"))
	int32 DebugCircleSegments = 96;
};

/** 클라이언트가 동일한 Niagara 확장을 재생하는 데 필요한 최소 상태다. */
USTRUCT()
struct LASTFPS_API FLastFPSExpandingMeshVisualState
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> EffectNiagaraSystem;

	UPROPERTY()
	FName StartRadiusOffsetNiagaraParameterName = TEXT("User.StartRadiusOffset");

	UPROPERTY()
	FName EndRadiusOffsetNiagaraParameterName = TEXT("User.EndRadiusOffset");

	UPROPERTY()
	FName ExpansionDurationNiagaraParameterName = TEXT("User.ExpansionDuration");

	UPROPERTY()
	FName ExpansionAlphaNiagaraParameterName = TEXT("User.ExpansionAlpha");

	UPROPERTY()
	FName RingThicknessNiagaraParameterName = TEXT("User.RingThickness");

	UPROPERTY()
	float MeshBaseOuterRadius = 100.f;

	UPROPERTY()
	float StartOuterRadius = 25.f;

	UPROPERTY()
	float EndOuterRadius = 1000.f;

	UPROPERTY()
	float ExpansionDuration = 1.f;

	UPROPERTY()
	float RingThickness = 100.f;

};

UCLASS()
class LASTFPS_API ALastFPSExpandingMeshAttackActor
	: public AActor
	, public ILastFPSPoolableActor
{
	GENERATED_BODY()

public:
	ALastFPSExpandingMeshAttackActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeAttack(
		AActor* InSourceActor,
		UAbilitySystemComponent* InSourceASC,
		const FLastFPSExpandingMeshAttackConfig& InAttackConfig);

	virtual void OnAcquiredFromPool_Implementation() override;
	virtual void OnReleasedToPool_Implementation() override;
	virtual void OnPrepareForPoolRenderWarmup_Implementation() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Expanding Mesh")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Expanding Mesh")
	TObjectPtr<UNiagaraComponent> EffectNiagaraComponent;

private:
	UFUNCTION()
	void OnRep_AttackState();

	void ConfigureAttack();
	void StartAttack();
	void FinishAttack();
	void UpdateExpansion();
	void ProcessRingHits(float PreviousOuterRadius, float CurrentOuterRadius);
	void DrawCollisionDebug(float CurrentOuterRadius) const;
	float GetSynchronizedWorldTime() const;
	bool DoesTargetPassConditions(AActor* TargetActor) const;
	bool ApplyEffectsToTarget(AActor* TargetActor);
	bool ApplyEffectToTarget(
		AActor* TargetActor,
		TSubclassOf<UGameplayEffect> EffectClass,
		bool bApplyDamage);
	UAbilitySystemComponent* GetAbilitySystemComponentFromActor(AActor* Actor) const;

	UPROPERTY()
	FLastFPSExpandingMeshAttackConfig AttackConfig;

	UPROPERTY(ReplicatedUsing=OnRep_AttackState)
	FLastFPSExpandingMeshVisualState VisualState;

	UPROPERTY(ReplicatedUsing=OnRep_AttackState)
	float ExpansionStartServerTime = 0.f;

	UPROPERTY()
	TWeakObjectPtr<AActor> SourceActor;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	TSet<TWeakObjectPtr<AActor>> AffectedActors;
	float PreviousOuterRadius = 0.f;
	FTimerHandle AttackLifetimeTimerHandle;
};
