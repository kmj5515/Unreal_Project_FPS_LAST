#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"

class UImage;
class UTexture2D;
class ULastFPSWeaponDefinition;
class UUserWidget;
struct FStreamableHandle;

/**
 * UI 아이콘을 게임 스레드를 막지 않고 채워 넣는다.
 *
 * 화면을 여는 프레임에 아이콘을 동기 로드하면 슬롯 수만큼 블로킹 IO 가 쌓인다.
 * 특히 무기 아이콘은 ULastFPSWeaponDefinition 을 거쳐야 하는데, 이 정의는 SkeletalMesh·ProjectileClass·
 * AnimLayer 를 하드 참조해서 아이콘 한 장 때문에 무기 에셋 전체가 딸려 온다.
 *
 * 요청은 호출자가 핸들로 들고 있다가 다음 요청 때 취소한다. 그래야 슬롯이 재사용될 때
 * 늦게 도착한 이전 아이콘이 새 아이템 위에 덮이지 않는다.
 */
namespace LastFPSIconLoader
{
	/**
	 * 소프트 아이콘을 비동기로 받아 이미지에 적용한다.
	 *
	 * @param Owner       콜백 수명의 기준. 위젯이 먼저 파괴되면 콜백은 실행되지 않는다.
	 * @param TargetImage 결과를 받을 이미지. 로드 전에는 건드리지 않는다.
	 * @param Icon        비어 있으면 아무 것도 하지 않고 빈 핸들을 돌려준다.
	 * @return 진행 중인 핸들. 호출자가 보관해 두었다가 새 요청 전에 취소한다.
	 */
	LASTFPS_API TSharedPtr<FStreamableHandle> RequestIcon(
		UUserWidget& Owner,
		UImage& TargetImage,
		const TSoftObjectPtr<UTexture2D>& Icon);

	/**
	 * 무기 정의를 먼저 받아 그 안의 아이콘을 이어서 로드한다.
	 * 정의에 아이콘이 없거나 로드에 실패하면 FallbackIcon 으로 넘어간다.
	 */
	LASTFPS_API TSharedPtr<FStreamableHandle> RequestWeaponIcon(
		UUserWidget& Owner,
		UImage& TargetImage,
		const TSoftObjectPtr<ULastFPSWeaponDefinition>& WeaponDefinition,
		const TSoftObjectPtr<UTexture2D>& FallbackIcon);

	/** 진행 중인 요청을 취소하고 핸들을 비운다. 슬롯이 다른 아이템으로 바뀔 때 먼저 호출한다. */
	LASTFPS_API void CancelRequest(TSharedPtr<FStreamableHandle>& Handle);
}
