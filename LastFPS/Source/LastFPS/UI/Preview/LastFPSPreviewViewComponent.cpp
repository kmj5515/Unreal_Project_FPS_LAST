#include "UI/Preview/LastFPSPreviewViewComponent.h"

ULastFPSPreviewViewComponent::ULastFPSPreviewViewComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 시점은 무대가 태그로 지목해 켠다. 기본이 켜져 있으면 어떤 시점이 잡힐지 배치 순서에 좌우된다.
	bAutoActivate = false;

	// 아웃게임 무대라 컨트롤러 회전을 따라갈 이유가 없다. 배치한 각도가 그대로 시점이다.
	bUsePawnControlRotation = false;
}
