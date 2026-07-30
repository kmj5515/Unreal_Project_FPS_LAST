#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSScopeOverlayWidget.generated.h"

class UImage;
class UTextBlock;
class UMaterialInstanceDynamic;

UCLASS(Abstract)
class LASTFPS_API ULastFPSScopeOverlayWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void ApplyAspectRatio(const FVector2D& LocalSize);
    void UpdateRange();

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UImage> ScopeVignette;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> RangeText;

    UPROPERTY(EditDefaultsOnly, Category="Scope", meta=(ClampMin="1.0"))
    float MaxRangeMeters = 500.f;

    UPROPERTY(EditDefaultsOnly, Category="Scope")
    FName AspectParameterName = TEXT("AspectRatio");

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> VignetteMaterial;

    float AppliedAspectRatio = 0.f;
    int32 DisplayedRangeMeters = -1;
};
