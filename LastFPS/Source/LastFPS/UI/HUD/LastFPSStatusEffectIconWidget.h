#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Status/LastFPSStatusEffectUIData.h"
#include "LastFPSStatusEffectIconWidget.generated.h"

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
