#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Status/LastFPSStatusEffectUIData.h"
#include "LastFPSStatusEffectIconWidget.generated.h"

class UBorder;
class UImage;
class UProgressBar;
class UTextBlock;
class UTexture2D;
struct FStreamableHandle;

/** 하나의 버프·디버프 아이콘과 해당 효과의 런타임 표시 상태만 담당한다. */
UCLASS(BlueprintType, Blueprintable)
class LASTFPS_API ULastFPSStatusEffectIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeDestruct() override;

	void InitializeStatusEffect(const FLastFPSStatusEffectUIData& InData);
	void UpdateRuntimeState(float TimeRemaining, float Duration, int32 StackCount);

	FGameplayTag GetStatusTag() const { return StatusEffectData.StatusTag; }

protected:
	UPROPERTY(BlueprintReadOnly, Category="HUD|Status Effect", meta=(BindWidgetOptional))
	TObjectPtr<UImage> StatusIcon;

	/** 아이콘 뒤에 깔리는 카테고리 구분 프레임. WBP에 없으면 프레임 교체를 건너뛴다. */
	UPROPERTY(BlueprintReadOnly, Category="HUD|Status Effect", meta=(BindWidgetOptional))
	TObjectPtr<UBorder> CategoryBackground;

	/**
	 * 카테고리별 배경 프레임 텍스처다. 코드에 카테고리를 열거하는 분기나 콘텐츠 경로를 두지 않으려고
	 * 맵으로 소유하며, 카테고리가 늘어나도 이 맵에 항목만 추가하면 된다.
	 * 항목이 없는 카테고리는 WBP에 저작된 브러시를 그대로 쓴다.
	 * 상시 상주할 필요가 없는 에셋이라 소프트 참조로 두고 표시 시점에 스트리밍한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Status Effect")
	TMap<ELastFPSStatusEffectCategory, TSoftObjectPtr<UTexture2D>> CategoryBackgroundTextures;

	UPROPERTY(BlueprintReadOnly, Category="HUD|Status Effect", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> StackText;

	UPROPERTY(BlueprintReadOnly, Category="HUD|Status Effect")
	FLastFPSStatusEffectUIData StatusEffectData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HUD|Skill|Icon")
	FName StatusIconTextureParameterName = TEXT("SkillIcon");

	UPROPERTY(EditDefaultsOnly, Category="HUD|Skill|Material")
	FName CooldownMaterialParameterName = TEXT("CoolDownRemainingPercent");

private:
	void RequestIconTexture();
	void HandleIconTextureLoaded();
	void ApplyIconTexture(UTexture2D* IconTexture) const;

	TSharedPtr<FStreamableHandle> IconLoadHandle;
};
