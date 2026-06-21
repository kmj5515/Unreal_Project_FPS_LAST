#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSCharacterCardWidget.generated.h"

class ULastFPSCharacterDefinition;
class UTextBlock;
class UBorder;

DECLARE_DELEGATE_OneParam(FOnCardClicked, int32 /*CardIndex*/);

UCLASS()
class LASTFPS_API ULastFPSCharacterCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 이 카드에 표시할 캐릭터 정의 설정 — SelectWidget이 NativeConstruct에서 호출 */
	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|UI")
	void SetupCard(const ULastFPSCharacterDefinition* Def);
	virtual void SetupCard_Implementation(const ULastFPSCharacterDefinition* Def);

	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|UI")
	void SetSelected(bool bSelected);
	virtual void SetSelected_Implementation(bool bSelected);

	/** 부모 SelectWidget이 NativeConstruct에서 지정 */
	int32 CardIndex = 0;

	FOnCardClicked OnCardClicked;

protected:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_CardName;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_CardRole;

	/** 카드 외곽 프레임 — 선택/호버 하이라이트는 이 보더 색으로 표현 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UBorder> OuterFrame;

	/** 선택됨: 밝은 시안 풀 강조 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|CharacterCard")
	FLinearColor SelectedFrameColor = FLinearColor(0.15f, 0.85f, 1.0f, 1.0f);

	/** 호버: 중간 밝기 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|CharacterCard")
	FLinearColor HoverFrameColor = FLinearColor(0.15f, 0.78f, 1.0f, 0.85f);

	/** 평상시: 어둡게 가라앉힘 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|CharacterCard")
	FLinearColor NormalFrameColor = FLinearColor(0.15f, 0.78f, 1.0f, 0.25f);

	/** 선택 시 살짝 커지는 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|CharacterCard")
	float SelectedScale = 1.06f;

	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
	/** 현재 선택 상태 — 호버 해제 시 어떤 색으로 돌아갈지 판단용 */
	bool bIsSelected = false;

	/** 선택/호버 상태를 보더 색에 반영 */
	void RefreshFrameVisual(bool bHovered);
};
