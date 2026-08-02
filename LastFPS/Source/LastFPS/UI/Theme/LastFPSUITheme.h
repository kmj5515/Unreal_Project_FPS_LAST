#pragma once

#include "CoreMinimal.h"

class ULastFPSUIThemeAsset;
class UImage;
class UWidget;

/**
 * 활성 테마를 찾아 위젯 트리에 흘려보내는 진입점.
 *
 * 각 위젯이 스스로 설정을 읽고 로드하면 같은 코드가 화면 수만큼 반복되고 로드 시점도 제각각이 된다.
 * 조회와 전파는 여기 한 곳에 두고, 위젯은 ILastFPSUIThemeReceiver::ApplyUITheme 만 구현하면 된다.
 */
namespace LastFPSUITheme
{
	/**
	 * 프로젝트 설정이 가리키는 테마를 반환한다. 아직 로드되지 않았다면 동기 로드한다.
	 * 화면을 여는 시점에만 호출되므로 프레임 비용이 아니라 지정 누락 여부가 중요하다.
	 * @return 지정되지 않았거나 로드에 실패하면 nullptr
	 */
	LASTFPS_API const ULastFPSUIThemeAsset* GetActiveTheme();

	/**
	 * Root 와 그 하위 위젯 중 ILastFPSUIThemeReceiver 를 구현한 것에 테마를 적용한다.
	 * 이미 UserWidget 경계를 넘어가며 순회하므로 화면 최상위에서 한 번만 부르면 된다.
	 * @return 적용된 위젯 수. 0 이면 테마 지정이나 인터페이스 구현을 확인해야 한다.
	 */
	LASTFPS_API int32 ApplyToWidgetTree(UWidget* Root);

	/**
	 * 등급 테두리 이미지에 색과 발광 세기를 적용한다.
	 *
	 * 슬롯·후보 카드·인벤토리 칸이 모두 같은 표현을 쓰므로 규칙을 한 곳에 둔다.
	 * 브러시가 머티리얼이면 동적 인스턴스의 파라미터로, 아니면 단순 틴트로 처리한다.
	 * (틴트 경로는 발광 세기를 색에 곱하는 근사라 머티리얼 경로보다 표현이 약하다.)
	 */
	LASTFPS_API void ApplyRarityGlow(UImage& BorderImage, const FLinearColor& RarityColor, float GlowIntensity);
}
