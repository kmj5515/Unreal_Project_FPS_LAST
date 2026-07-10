#pragma once

#include "UI/Framework/LastFPSActivatableWidget.h"
#include "Data/Tables/LastFPSItemData.h"
#include "LastFPSWeaponPreviewWidget.generated.h"

class ULastFPSWeaponDefinition;
class UTextBlock;
class UImage;
class ULastFPSButtonBase;
class ALastFPSWeaponPreviewRig;

/**
 * 무기 상세 3D 프리뷰 오버레이 (Modal 레이어). 인벤토리에서 무기 hover 중 F1 로 열린다.
 * 왼쪽=능력치 패널, 오른쪽=3D 회전(추후 페이즈). ESC 로 이 오버레이만 닫힌다.
 *
 * P1: push/close 뼈대만. Setup 이 무기 정의/아이템을 보관한다.
 * P2: 왼쪽 스탯 패널 채우기. P3: SceneCapture 리그로 오른쪽 3D 표시 + 드래그 회전.
 */
UCLASS()
class LASTFPS_API ULastFPSWeaponPreviewWidget : public ULastFPSActivatableWidget
{
	GENERATED_BODY()

public:
	/** 표시할 무기 정의/아이템 + 인벤토리가 소유·예열한 리그 주입 (F1 핸들러가 push 시 호출). */
	void Setup(ULastFPSWeaponDefinition* InWeaponDef, const FLastFPSItemData& InItem, FName InRowId, ALastFPSWeaponPreviewRig* InRig);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 오른쪽 프리뷰 영역 드래그로 무기 회전.
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 보관된 무기 정의/아이템으로 왼쪽 스탯 패널 채우기. */
	void PopulateStats();

	/** 주입된 리그에 무기 메시 세팅 + 플레이어 뷰타깃을 리그 카메라로 전환. */
	void BindPreviewRig();

	/** 드래그 픽셀당 회전 각도(도). */
	UPROPERTY(EditDefaultsOnly, Category="Preview")
	float RotationSpeed = 0.4f;

	// ── 왼쪽 능력치 패널 (Designer 바인딩) ──
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_WeaponName;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_Rarity;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_Type;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_Description;

	/** 스탯 시트 2열 — 라벨 열 / 값 열 (줄 수를 맞춰 정렬). */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_StatLabels;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_StatValues;

	/** (선택) 프리뷰 영역 테두리/배경용 이미지. 뷰타깃 카메라 방식에선 렌더타깃을 연결하지 않는다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> Image_Preview;

	/** 닫기 버튼 (선택, 공용 버튼) — 클릭 시 이 오버레이를 닫는다. ESC 와 동일 동작. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<ULastFPSButtonBase> Btn_Close;

	/** 닫기 버튼 클릭 핸들러 — Modal 레이어에서 자신을 pop. */
	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(Transient)
	TObjectPtr<ULastFPSWeaponDefinition> WeaponDef;

	UPROPERTY(Transient)
	FLastFPSItemData CachedItem;

	FName CachedRowId;

	UPROPERTY(Transient)
	TObjectPtr<ALastFPSWeaponPreviewRig> Rig;

	/** 프리뷰 진입 전 뷰타깃(닫을 때 복구용). */
	TWeakObjectPtr<AActor> PrevViewTarget;

	bool bDragging = false;
};
