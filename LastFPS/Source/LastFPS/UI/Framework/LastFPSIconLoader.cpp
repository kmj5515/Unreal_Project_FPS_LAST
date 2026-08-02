#include "UI/Framework/LastFPSIconLoader.h"

#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSIconLoader, Log, All);

namespace
{
	/** 로드된 텍스처를 이미지에 반영한다. 실패는 이미지를 건드리지 않는 것으로 표현한다. */
	void ApplyTexture(UImage& TargetImage, UTexture2D* Texture)
	{
		if (!Texture)
		{
			return;
		}

		TargetImage.SetBrushFromTexture(Texture);
	}
}

TSharedPtr<FStreamableHandle> LastFPSIconLoader::RequestIcon(
	UUserWidget& Owner,
	UImage& TargetImage,
	const TSoftObjectPtr<UTexture2D>& Icon)
{
	if (Icon.IsNull())
	{
		return nullptr;
	}

	// 이미 메모리에 있으면 한 프레임 늦출 이유가 없다. 비동기 경로는 실제로 디스크를 읽어야 할 때만 탄다.
	if (UTexture2D* LoadedIcon = Icon.Get())
	{
		ApplyTexture(TargetImage, LoadedIcon);
		return nullptr;
	}

	UImage* ImagePtr = &TargetImage;
	const TSoftObjectPtr<UTexture2D> IconCopy = Icon;

	// CreateWeakLambda: 슬롯이 먼저 파괴되면 콜백을 실행하지 않는다. 이미지 포인터는 슬롯이 소유한다.
	return UAssetManager::GetStreamableManager().RequestAsyncLoad(
		Icon.ToSoftObjectPath(),
		FStreamableDelegate::CreateWeakLambda(&Owner, [ImagePtr, IconCopy]()
		{
			ApplyTexture(*ImagePtr, IconCopy.Get());
		}));
}

TSharedPtr<FStreamableHandle> LastFPSIconLoader::RequestWeaponIcon(
	UUserWidget& Owner,
	UImage& TargetImage,
	const TSoftObjectPtr<ULastFPSWeaponDefinition>& WeaponDefinition,
	const TSoftObjectPtr<UTexture2D>& FallbackIcon)
{
	if (WeaponDefinition.IsNull())
	{
		return RequestIcon(Owner, TargetImage, FallbackIcon);
	}

	// 정의가 이미 로드돼 있으면 아이콘 경로를 바로 알 수 있다.
	if (const ULastFPSWeaponDefinition* LoadedDefinition = WeaponDefinition.Get())
	{
		const TSoftObjectPtr<UTexture2D>& WeaponIcon = LoadedDefinition->Icon;
		return RequestIcon(Owner, TargetImage, WeaponIcon.IsNull() ? FallbackIcon : WeaponIcon);
	}

	UUserWidget* OwnerPtr = &Owner;
	UImage* ImagePtr = &TargetImage;
	const TSoftObjectPtr<ULastFPSWeaponDefinition> DefinitionCopy = WeaponDefinition;
	const TSoftObjectPtr<UTexture2D> FallbackCopy = FallbackIcon;

	// 아이콘 경로를 알려면 정의를 먼저 받아야 한다. 정의가 무거운 것은 어쩔 수 없지만
	// 최소한 게임 스레드를 멈추지는 않게 한다.
	return UAssetManager::GetStreamableManager().RequestAsyncLoad(
		WeaponDefinition.ToSoftObjectPath(),
		FStreamableDelegate::CreateWeakLambda(&Owner, [OwnerPtr, ImagePtr, DefinitionCopy, FallbackCopy]()
		{
			const ULastFPSWeaponDefinition* LoadedDefinition = DefinitionCopy.Get();
			if (!LoadedDefinition)
			{
				UE_LOG(LogLastFPSIconLoader, Warning,
					TEXT("무기 정의를 불러오지 못해 기본 아이콘을 사용합니다: %s"), *DefinitionCopy.ToString());
				RequestIcon(*OwnerPtr, *ImagePtr, FallbackCopy);
				return;
			}

			const TSoftObjectPtr<UTexture2D>& WeaponIcon = LoadedDefinition->Icon;
			RequestIcon(*OwnerPtr, *ImagePtr, WeaponIcon.IsNull() ? FallbackCopy : WeaponIcon);
		}));
}

void LastFPSIconLoader::CancelRequest(TSharedPtr<FStreamableHandle>& Handle)
{
	if (Handle.IsValid())
	{
		Handle->CancelHandle();
		Handle.Reset();
	}
}
