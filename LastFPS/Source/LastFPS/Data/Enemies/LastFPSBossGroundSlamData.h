#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Actors/LastFPSExpandingMeshAttackActor.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LastFPSBossGroundSlamData.generated.h"

class UAnimMontage;

/** 보스 지면 강타의 타이밍, 지면 투영, 도넛 범위 설정을 보관한다. */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSBossGroundSlamData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULastFPSBossGroundSlamData();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ground Slam|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ground Slam|Animation", meta=(ClampMin="0.01"))
	float MontagePlayRate = 1.f;

	/** 이 태그를 보내는 Anim Notify 시점에 도넛 충돌 액터를 생성한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ground Slam|Animation")
	FGameplayTag ImpactEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ground Slam|Attack Mesh")
	TSubclassOf<ALastFPSExpandingMeshAttackActor> AttackActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ground Slam|Attack Mesh")
	FLastFPSExpandingMeshAttackConfig AttackConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ground Slam|Ground")
	bool bProjectToGround = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ground Slam|Ground")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ground Slam|Ground", meta=(ClampMin="0.0", Units="cm"))
	float GroundTraceStartOffset = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ground Slam|Ground", meta=(ClampMin="0.0", Units="cm"))
	float GroundTraceDistance = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ground Slam|Ground", meta=(ClampMin="0.0", Units="cm"))
	float GroundSurfaceOffset = 3.f;
};
