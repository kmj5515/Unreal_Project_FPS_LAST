#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LastFPSCharacterVisualData.generated.h"

class UAnimationAsset;
class UAnimInstance;
class USkeletalMesh;
class UTexture2D;

UCLASS(BlueprintType)
class LASTFPS_API ULastFPSCharacterVisualData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TSubclassOf<UAnimInstance> AnimClass;

	/**
	 * 아웃게임 프리뷰 무대에서 쓸 애님 블루프린트.
	 *
	 * 위 AnimClass 를 그대로 쓰지 않는다. 게임플레이 ABP 는 대개 이동 컴포넌트의 속도·가속도를 읽는데,
	 * 무대의 캐릭터는 빙의돼 있지 않고 이동도 꺼져 있어 그 값들이 의미를 잃는다.
	 *
	 * ABP 는 스켈레톤에 묶이므로 영웅마다 따로 지정해야 한다. 위 SkeletalMesh 와 같은 곳에 두어야
	 * 메시만 갈아끼우고 자세를 안 바꾸는 실수를 막을 수 있다.
	 *
	 * 비우면 아래 PreviewIdleAnimation 을, 그것도 비우면 캐릭터 BP 에 저작된 ABP 를 그대로 쓴다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual|Preview")
	TSubclassOf<UAnimInstance> PreviewAnimClass;

	/** 자세 하나면 ABP 를 만들지 않아도 된다. PreviewAnimClass 가 있으면 그쪽이 우선한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual|Preview")
	TObjectPtr<UAnimationAsset> PreviewIdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<UTexture2D> Portrait;
};
