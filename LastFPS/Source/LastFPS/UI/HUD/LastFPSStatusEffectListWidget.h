#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "LastFPSStatusEffectListWidget.generated.h"

class UAbilitySystemComponent;
class UPanelWidget;
class ULastFPSStatusEffectIconWidget;

/** 소유 캐릭터의 활성 버프·디버프를 감시하고 아이콘 목록을 동기화한다. */
UCLASS(BlueprintType, Blueprintable)
class LASTFPS_API ULastFPSStatusEffectListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeDestruct() override;

	void InitializeWithAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent);
	void UpdateRuntimeStates();
	void UninitializeFromAbilitySystem();

protected:
	/** Horizontal Box, Wrap Box 등 원하는 Panel Widget을 사용할 수 있다. */
	UPROPERTY(BlueprintReadOnly, Category="HUD|Status Effect", meta=(BindWidget))
	TObjectPtr<UPanelWidget> StatusEffectContainer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Status Effect")
	TSubclassOf<ULastFPSStatusEffectIconWidget> StatusEffectIconWidgetClass;

private:
	void RefreshEntries();
	void HandleStatusTagChanged(FGameplayTag StatusTag, int32 NewCount);
	void QueryRuntimeState(FGameplayTag StatusTag, float& OutRemaining, float& OutDuration, int32& OutStacks) const;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<ULastFPSStatusEffectIconWidget>> ActiveIconWidgets;

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	TMap<FGameplayTag, FDelegateHandle> StatusTagDelegateHandles;
	bool bConfigurationWarningLogged = false;
};
