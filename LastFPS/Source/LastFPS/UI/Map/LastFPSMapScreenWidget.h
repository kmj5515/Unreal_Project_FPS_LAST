#pragma once

#include "UI/Framework/LastFPSContentScreenWidget.h"
#include "LastFPSMapScreenWidget.generated.h"

class UPanelWidget;
class UWidget;
class ULastFPSTravelEntryButton;

/**
 * WBP_MapScreen 의 Parent — 목적지를 골라 이동하는 지도 화면.
 *
 * ULastFPSTravelEntryButton 은 목적지만 보관하고 이동 수행은 소유 화면에 맡긴다는 계약이므로,
 * 실제 RequestTravel 호출은 여기 한 곳에 모은다. 버튼은 어떤 서브시스템이 이동을 처리하는지 모른다.
 *
 * 지도 이동은 ScrollBox 대신 RenderTransform 으로 직접 옮긴다. ScrollBox 는 방향이 하나뿐이라
 * 가로·세로를 함께 훑을 수 없고, 지도에는 드래그 팬과 휠 줌이 더 맞는 조작이다.
 */
UCLASS()
class LASTFPS_API ULastFPSMapScreenWidget : public ULastFPSContentScreenWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(
		const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(
		const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(
		const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	/** 목적지 버튼이 배치된 패널. 이 안의 TravelEntryButton 자식이 목적지가 된다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UPanelWidget> Box_Destinations;

	/**
	 * 지도를 들여다보는 창. 이 사각형이 이동 가능 범위의 기준이 되며 스스로는 움직이지 않는다.
	 * 캔버스에서 원하는 만큼 잡아 두고, Clipping 을 Clip to Bounds 로 두면 밖으로 새지 않는다.
	 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> Panel_MapView;

	/**
	 * 실제로 끌려 다니는 알맹이. 지도 이미지와 함께 움직여야 하는 것들을 이 안에 넣는다.
	 *
	 * 비워 두면 Panel_MapView 자신이 움직이고 그 부모가 창이 된다. 다만 창을 크게 잡아 두면
	 * 창과 알맹이가 같은 크기가 되어 이동 여유가 사라지므로, 둘을 나눠 두는 편이 예측 가능하다.
	 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> Panel_MapContent;

	UPROPERTY(EditDefaultsOnly, Category="Map", meta=(ClampMin="0.05"))
	float MinZoom = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category="Map", meta=(ClampMin="0.05"))
	float MaxZoom = 4.0f;

	/** 휠 한 칸당 배율. 1.15 면 한 칸에 15% 확대된다. */
	UPROPERTY(EditDefaultsOnly, Category="Map", meta=(ClampMin="1.01"))
	float ZoomStep = 1.15f;

	/** 지도가 화면 밖으로 완전히 빠져나가지 않도록 이동 범위를 제한한다. */
	UPROPERTY(EditDefaultsOnly, Category="Map")
	bool bClampPanToView = true;

private:
	/** @param DestinationButton 클릭된 버튼. 화면이 파괴된 뒤 호출될 수 있어 약참조로 넘긴다. */
	void HandleDestinationClicked(TWeakObjectPtr<ULastFPSTravelEntryButton> DestinationButton);

	/** 실제로 변환을 받는 위젯. Panel_MapContent 가 있으면 그것, 없으면 Panel_MapView. */
	UWidget* GetTransformTarget() const;

	/** 이동 범위의 기준이 되는 창의 크기. 창 위젯이 없으면 변환 대상의 부모를 쓴다. */
	FVector2D GetViewSize(const FGeometry& FallbackGeometry) const;

	/** 현재 팬·줌 값을 대상 패널에 반영한다. */
	void ApplyMapTransform();

	/** 알맹이가 창 밖으로 벗어나지 않게 PanOffset 을 접는다. */
	void ClampPanOffset(const FGeometry& FallbackGeometry);

	/** 등록 해제를 위해 실제로 바인딩한 버튼만 들고 있는다. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULastFPSTravelEntryButton>> BoundDestinations;

	/** 뷰 좌상단 기준 이동량(픽셀). 회전 피벗을 (0,0)으로 두어 스케일과 독립적으로 다룬다. */
	FVector2D PanOffset = FVector2D::ZeroVector;

	float Zoom = 1.f;

	bool bIsPanning = false;

	/** 직전 프레임의 커서 위치(스크린 공간). 이동량을 차분으로 구한다. */
	FVector2D LastScreenPosition = FVector2D::ZeroVector;
};
