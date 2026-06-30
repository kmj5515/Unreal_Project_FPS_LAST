#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "LastFPSAreaEffectActor.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class USphereComponent;

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSAreaEffectConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Area", meta=(ClampMin="0.0"))
	float Radius = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Area", meta=(ClampMin="0.0"))
	float Duration = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Area", meta=(ClampMin="0.01"))
	float DamageInterval = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Damage")
	FLastFPSDamageRange DamageRange;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Target Effects")
	TArray<TSubclassOf<UGameplayEffect>> TargetEffects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Condition")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Condition")
	FGameplayTagContainer BlockedTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|VFX")
	TObjectPtr<UNiagaraSystem> EffectNiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|VFX")
	FName DurationNiagaraParameterName = TEXT("User.Duration");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Debug")
	bool bDrawDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Debug", meta=(ClampMin="0.0", EditCondition="bDrawDebug"))
	float DebugDrawTime = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Debug", meta=(EditCondition="bDrawDebug"))
	FLinearColor DebugColor = FLinearColor(0.f, 0.75f, 1.f, 1.f);
};

UCLASS()
class LASTFPS_API ALastFPSAreaEffectActor : public AActor
{
	GENERATED_BODY()

public:
	ALastFPSAreaEffectActor();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeAreaEffect(
		AActor* InSourceActor,
		UAbilitySystemComponent* InSourceASC,
		const FLastFPSAreaEffectConfig& InAreaConfig);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Area Effect")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Area Effect")
	TObjectPtr<USphereComponent> AreaSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Area Effect")
	TObjectPtr<UNiagaraComponent> EffectNiagaraComponent;

private:
	UFUNCTION()
	void OnRep_AreaConfig();

	void ConfigureArea();
	void ApplyAreaEffects();
	void ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> EffectClass, bool bApplyDamage);
	void FinishArea();
	void CollectTargets(TArray<AActor*>& OutTargets) const;
	bool DoesTargetPassTags(AActor* TargetActor) const;
	UAbilitySystemComponent* GetAbilitySystemComponentFromActor(AActor* Actor) const;
	void DrawAreaDebug() const;
	void DrawTargetDebug(AActor* TargetActor) const;

	UPROPERTY(ReplicatedUsing=OnRep_AreaConfig)
	FLastFPSAreaEffectConfig AreaConfig;

	UPROPERTY()
	TWeakObjectPtr<AActor> SourceActor;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	FTimerHandle DamageTimerHandle;
	FTimerHandle DurationTimerHandle;
};
