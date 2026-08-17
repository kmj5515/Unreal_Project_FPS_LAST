#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Pooling/LastFPSPoolableActor.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "LastFPSAreaEffectActor.generated.h"

class UAbilitySystemComponent;
class UAudioComponent;
class UGameplayEffect;
class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class USoundBase;
class USphereComponent;

UENUM(BlueprintType)
enum class ELastFPSAreaEffectShape : uint8
{
	Sphere UMETA(DisplayName="Sphere"),
	Cone UMETA(DisplayName="Cone"),
};

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSAreaEffectConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Area")
	ELastFPSAreaEffectShape Shape = ELastFPSAreaEffectShape::Sphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Area", meta=(ClampMin="0.0"))
	float Radius = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Area", meta=(ClampMin="0.0", ClampMax="360.0", EditCondition="Shape==ELastFPSAreaEffectShape::Cone"))
	float ConeAngleDegrees = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Area", meta=(ClampMin="0.0"))
	float Duration = 5.f;

	/**
	 * 아래 판정용 필드는 ApplyAreaEffects / DoesTargetPass* 경로에서만 쓰이고 그 경로는 전부
	 * HasAuthority 로 막혀 있다. 클라이언트가 읽지 않는 값이라 복제에서 제외해, 이 구조체가
	 * 변경될 때마다 GE 클래스 목록과 태그 컨테이너까지 통째로 재전송되지 않게 한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Area", meta=(ClampMin="0.01"), NotReplicated)
	float DamageInterval = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Damage", NotReplicated)
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Damage", NotReplicated)
	TSubclassOf<UGameplayEffect> DamageCooldownEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Damage", NotReplicated)
	FLastFPSDamageRange DamageRange;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Target Effects", NotReplicated)
	TArray<TSubclassOf<UGameplayEffect>> TargetEffects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Condition", NotReplicated)
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Condition", NotReplicated)
	FGameplayTagContainer BlockedTargetTags;

	/**
	 * 시전자와 같은 진영은 대상에서 제외한다.
	 * 이 검사가 없으면 오버랩에 잡힌 모든 폰이 대상이 되어, 멀티에서 아군 플레이어가 스킬에 맞는다.
	 * 적 스킬도 같은 액터를 쓰므로 진영 계약만으로 양쪽에 동일하게 적용된다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Condition", NotReplicated)
	bool bIgnoreFriendlyTargets = true;

	/**
	 * 시전마다 증가하는 카운터다. 이 액터는 풀에서 재사용되므로 해제(기본값 대입)와 재획득(원래 값
	 * 대입)이 같은 복제 윈도우 안에서 끝나면, 서버 shadow 와 최종값이 같아 변경 없음으로 판정되고
	 * 아무것도 전송되지 않는다. 그러면 클라이언트에서 OnRep 이 뜨지 않아 VFX 가 통째로 누락된다.
	 * 값이 동일해도 이 카운터가 달라져 dirty 판정을 보장한다.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Area Effect")
	uint8 ActivationSequence = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|VFX")
	TObjectPtr<UNiagaraSystem> EffectNiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|VFX")
	FName RadiusNiagaraParameterName = TEXT("User.Radius");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|VFX", meta=(ClampMin="0.0"))
	float VisualRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|VFX")
	FName VisualRadiusNiagaraParameterName = TEXT("User.VisualRadius");
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|VFX")
	FName DurationNiagaraParameterName = TEXT("User.Duration");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|VFX")
	FName SurfaceNormalNiagaraParameterName = TEXT("User.SurfaceNormal");

	/** 영역 액터에 부착해 재생할 사운드다. 반복과 거리 감쇠는 Sound Cue에서 구성한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Audio")
	TObjectPtr<USoundBase> AttachedSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Audio", meta=(ClampMin="0.0", Units="s"))
	float SoundStartTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Audio", meta=(ClampMin="0.0"))
	float SoundVolumeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Audio", meta=(ClampMin="0.01"))
	float SoundPitchMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Debug")
	bool bDrawDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Debug", meta=(ClampMin="0.0", EditCondition="bDrawDebug"))
	float DebugDrawTime = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area Effect|Debug", meta=(EditCondition="bDrawDebug"))
	FLinearColor DebugColor = FLinearColor(0.f, 0.75f, 1.f, 1.f);
};

UCLASS()
class LASTFPS_API ALastFPSAreaEffectActor
	: public AActor
	, public ILastFPSPoolableActor
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

	virtual void OnAcquiredFromPool_Implementation() override;
	virtual void OnReleasedToPool_Implementation() override;
	virtual void OnPrepareForPoolRenderWarmup_Implementation() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Area Effect")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Area Effect")
	TObjectPtr<USphereComponent> AreaSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Area Effect")
	TObjectPtr<UNiagaraComponent> EffectNiagaraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Area Effect|Audio")
	TObjectPtr<UAudioComponent> AttachedAudioComponent;

private:
	UFUNCTION()
	void OnRep_AreaConfig();

	void ConfigureArea();
	void ConfigureAttachedAudio();
	void StopAttachedAudio();
	void StartAreaEffect();
	void ApplyAreaEffects();
	bool ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> EffectClass, bool bApplyDamage);
	void FinishArea();
	void CollectTargets(TArray<AActor*>& OutTargets) const;
	bool DoesTargetPassShape(AActor* TargetActor) const;
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

	/** 풀 재사용을 넘어 유지되어야 하므로 AreaConfig 가 아니라 액터가 소유한다. */
	uint8 AreaActivationSequence = 0;

	FTimerHandle DamageTimerHandle;
	FTimerHandle DurationTimerHandle;
};
