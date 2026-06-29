#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSDamageNumberWidget.generated.h"

class UTextBlock;
class AActor;

UCLASS()
class LASTFPS_API ULastFPSDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	ULastFPSDamageNumberWidget(const FObjectInitializer& ObjectInitializer);

	void InitializeDamageNumber(
		float DamageAmount,
		float TotalDamageDealt,
		AActor* DamageTargetActor,
		const FVector& FallbackWorldLocation,
		const FVector& TargetWorldOffset,
		const FVector2D& ScreenOffset,
		const FVector2D& RandomScreenOffset,
		bool bCritical = false);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void PlayDamageNumberAnimation(bool bCritical);

private:
	void EnsureNativeWidgets();
	void ApplyDamageText(float DamageAmount);
	void UpdateScreenPosition();
	void UpdateLifetime(float DeltaTime);
	
	UPROPERTY(Transient, meta=(BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> GeneralAnimation;

	UPROPERTY(Transient, meta=(BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> CriticalAnimation;
	
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, AllowPrivateAccess="true"))
	TObjectPtr<UTextBlock> DamageTextBlock;

	UPROPERTY(EditDefaultsOnly, Category="Damage Number", meta=(ClampMin="0.01"))
	float LifeTime = 0.75f;

	TWeakObjectPtr<AActor> TrackedTargetActor;
	FVector FallbackDamageWorldLocation = FVector::ZeroVector;
	FVector TrackedWorldOffset = FVector::ZeroVector;
	FVector2D DamageScreenOffset = FVector2D::ZeroVector;
	FVector2D DamageRandomScreenOffset = FVector2D::ZeroVector;
	float ElapsedTime = 0.f;
	bool bIsCritical = false;
};
