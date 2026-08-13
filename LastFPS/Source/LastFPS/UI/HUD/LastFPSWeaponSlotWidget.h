#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSWeaponSlotWidget.generated.h"

class UImage;
class UTextBlock;
class ULastFPSWeaponDefinition;
struct FStreamableHandle;

/**
 * WBP_HUDWeaponSlot 의 Parent — HUD 무기 슬롯 1칸.
 *
 * 표시 전용이며 상태를 스스로 조회하지 않는다. Presenter 가 갱신 시점을 정하고 값을 밀어 넣는다.
 * Designer 바인딩(모두 선택): TB_SlotKey / TB_WeaponName / Img_WeaponIcon / Img_ActiveHighlight
 */
UCLASS()
class LASTFPS_API ULastFPSWeaponSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @param SlotIndex   0-based 슬롯 번호. 표시는 1-based 로 바꾼다(1·2 키와 맞추기 위함).
	 * @param Definition  이 슬롯의 무기. null 이면 빈 슬롯으로 표시한다.
	 * @param bIsActive   현재 손에 든 슬롯인지.
	 */
	void SetupSlot(int32 SlotIndex, const ULastFPSWeaponDefinition* Definition, bool bIsActive);

	/** 활성 강조만 갱신한다. 무기 구성이 그대로일 때 전체 재구성을 피하기 위한 경로다. */
	void SetActive(bool bIsActive);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_SlotKey;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Img_WeaponIcon;

	/** 활성 슬롯일 때만 보이는 테두리·발광 등. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Img_ActiveHighlight;

	/** 빈 슬롯에 표시할 문구. 무기가 없을 때 이름 칸을 비워 두지 않기 위한 값이다. */
	UPROPERTY(EditDefaultsOnly, Category="HUD|Weapon Slot")
	FText EmptySlotText;

	/** 비활성 슬롯을 흐리게 보여 활성 슬롯이 눈에 띄게 한다. */
	UPROPERTY(EditDefaultsOnly, Category="HUD|Weapon Slot", meta=(ClampMin="0.0", ClampMax="1.0"))
	float InactiveOpacity = 0.45f;
	
	
	UPROPERTY(EditDefaultsOnly,Category="HUD|Weapon Slot")
	bool bUseHUDIcon;
private:
	/** 슬롯 내용이 바뀌거나 위젯이 제거될 때 이전 비동기 아이콘 요청을 취소하기 위해 보관한다. */
	TSharedPtr<FStreamableHandle> IconLoadHandle;
};
