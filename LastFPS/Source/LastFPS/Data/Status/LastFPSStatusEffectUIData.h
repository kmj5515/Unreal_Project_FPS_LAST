#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "LastFPSStatusEffectUIData.generated.h"

class UTexture2D;
class UMaterialInterface;

UENUM(BlueprintType)
enum class ELastFPSStatusEffectCategory : uint8
{
	Buff,
	Debuff,
	Neutral
};

UENUM(BlueprintType)
enum class ELastFPSStatusAnimationPolicy : uint8
{
	None,
	PauseCurrentPose
};

/** 상태 효과가 활성화됐을 때 캐릭터 메시 위에 표시할 공통 연출 설정이다. */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSStatusOverlayData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect|Overlay")
	bool bEnabled = false;

	/** 완성 상태 태그가 생기기 전까지 누적 강도를 계산할 때 사용하는 태그다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect|Overlay", meta=(Categories="Status"))
	FGameplayTag StackTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect|Overlay")
	TSoftObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect|Overlay")
	FName StackMixParameterName = TEXT("VfxMix");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect|Overlay")
	float StackMixStartValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect|Overlay")
	float StackMixEndValue = 1.f;

	/** 이 스택 수에 도달하면 최종 혼합값을 사용한다. Gameplay Effect 구현과 표시 데이터를 분리하기 위한 명시적 값이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect|Overlay", meta=(ClampMin="1"))
	int32 StackCountForFullMix = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect|Overlay")
	bool bInterpolateStackMix = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect|Overlay", meta=(ClampMin="0.0"))
	float StackMixInterpSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect|Overlay")
	int32 Priority = 0;
};

/** 버프·디버프의 불변 표시 정보만 보관한다. 남은 시간과 스택은 활성 Gameplay Effect에서 조회한다. */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSStatusEffectUIData : public FTableRowBase
{
	GENERATED_BODY()

	/** 런타임 조회에 사용하는 고유 식별자이며 활성 Gameplay Effect가 대상에게 실제로 부여하는 태그다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect", meta=(Categories="Status"))
	FGameplayTag StatusTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect")
	ELastFPSStatusEffectCategory Category = ELastFPSStatusEffectCategory::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect", meta=(MultiLine=true))
	FText Description;

	/** 테이블 로드 시 아이콘까지 강제로 메모리에 올리지 않도록 소프트 참조를 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect")
	FLinearColor IconTint = FLinearColor::White;

	/** 값이 높을수록 HUD 목록의 앞쪽에 배치한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect")
	int32 DisplayPriority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect")
	bool bShowDuration = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect")
	bool bShowStackCount = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect")
	bool bShowOnHUD = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect")
	FLastFPSStatusOverlayData Overlay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Status Effect|Animation")
	ELastFPSStatusAnimationPolicy AnimationPolicy = ELastFPSStatusAnimationPolicy::None;
};
