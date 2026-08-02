#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameplayTagContainer.h"
#include "LastFPSPreviewSlotComponent.generated.h"

/**
 * 프리뷰 무대에서 대상 하나가 서는 자리.
 *
 * 무대에 자리를 여러 개 두면 캐릭터와 무기를 동시에 올려 둔 채 시점만 옮길 수 있다.
 * 자리가 하나뿐이면 무기를 보려 할 때마다 캐릭터를 치워야 하고, 돌아올 때 다시 조립해야 한다.
 *
 * 이 컴포넌트는 위치와 회전만 잡는다. 실제 메시는 무대가 런타임에 이 아래로 붙인다.
 * 드래그 회전도 이 컴포넌트를 돌리므로, 대상이 제자리에서 도는 것으로 보인다.
 */
UCLASS(ClassGroup=(Preview), meta=(BlueprintSpawnableComponent, DisplayName="LastFPS Preview Slot"))
class LASTFPS_API ULastFPSPreviewSlotComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	ULastFPSPreviewSlotComponent();

	FGameplayTag GetSlotTag() const { return SlotTag; }
	void SetSlotTag(const FGameplayTag& InSlotTag) { SlotTag = InSlotTag; }

protected:
	/** 이 자리를 지목할 때 쓰는 태그. 같은 태그가 둘이면 먼저 찾은 것이 쓰인다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preview", meta=(Categories="UI.Preview.Slot"))
	FGameplayTag SlotTag;
};
