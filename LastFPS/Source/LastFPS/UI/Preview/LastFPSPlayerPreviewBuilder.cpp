#include "UI/Preview/LastFPSPlayerPreviewBuilder.h"

#include "Character/LastFPSCharacterBase.h"
#include "Data/Characters/LastFPSCharacterVisualData.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/LastFPSEquipmentSubsystem.h"
#include "UI/Framework/LastFPSUITags.h"
#include "UI/Preview/LastFPSPreviewStageActor.h"
#include "UI/Theme/LastFPSUISettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSPlayerPreview, Log, All);

namespace
{
	/** 무대에 세울 무기 슬롯. 아웃게임에서는 주무기 한 자루만 보여 준다. */
	constexpr int32 PrimaryWeaponSlotIndex = 0;
}

bool LastFPSPlayerPreview::BuildSubject(const UObject* WorldContext, FLastFPSPreviewSubject& OutSubject)
{
	if (!GEngine || !WorldContext)
	{
		return false;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return false;
	}

	// "내 캐릭터와 같은 모습"이 요구사항이므로 설정 애셋이 아니라 실제 폰에서 가져온다.
	// 캐릭터가 바뀌어도 무대가 따라오고, 지정을 두 곳에서 관리할 일이 없다.
	const APlayerController* PC = World->GetFirstPlayerController();
	const APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		// 레벨 전환 직후처럼 폰이 아직 없을 수 있다. 정상 흐름이라 경고로 남기지 않는다.
		return false;
	}

	// 메시·파츠·머티리얼을 하나씩 옮겨 담지 않는다. 모듈러 캐릭터는 구성이 늘 때마다 여기도
	// 고쳐야 하고, 실제로 회전 보정과 의상 파츠를 연달아 놓쳤다.
	// 클래스를 그대로 세우면 조립 결과가 통째로 따라온다.
	OutSubject = FLastFPSPreviewSubject();
	OutSubject.ActorClass = PlayerPawn->GetClass();

	// 프리뷰 자세는 스켈레톤에 묶이므로 영웅마다 다르다. 전역 설정 하나로는 표현할 수 없어
	// 메시를 들고 있는 곳(캐릭터 시각 데이터)에서 함께 읽는다.
	// 둘 다 비면 캐릭터 BP 에 저작된 ABP 가 그대로 돈다.
	if (const ALastFPSCharacterBase* PlayerCharacter = Cast<ALastFPSCharacterBase>(PlayerPawn))
	{
		if (const ULastFPSCharacterDefinition* Definition = PlayerCharacter->ResolveCharacterDefinition())
		{
			if (const ULastFPSCharacterVisualData* VisualData = Definition->VisualData.LoadSynchronous())
			{
				OutSubject.AnimInstanceClass = VisualData->PreviewAnimClass;
				if (!OutSubject.AnimInstanceClass)
				{
					OutSubject.IdleAnimation = VisualData->PreviewIdleAnimation;
				}
			}
		}
	}

	// 장착 무기는 장비 서브시스템이 단독으로 안다. 무대도 화면도 장착 규칙을 알지 않는다.
	const UGameInstance* GameInstance = World->GetGameInstance();
	const ULastFPSEquipmentSubsystem* Equipment =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSEquipmentSubsystem>() : nullptr;
	if (!Equipment)
	{
		return true;
	}

	if (const ULastFPSWeaponDefinition* WeaponDefinition =
		Equipment->GetWeaponDefinitionForSlot(PrimaryWeaponSlotIndex))
	{
		if (!WeaponDefinition->SkeletalMesh.IsNull())
		{
			FLastFPSPreviewAttachment& Attachment = OutSubject.Attachments.AddDefaulted_GetRef();
			Attachment.Mesh = WeaponDefinition->SkeletalMesh.LoadSynchronous();
			// 부착 소켓은 무기 정의가 이미 들고 있다. UI 가 소켓 이름을 따로 저작하지 않는다.
			Attachment.SocketName = WeaponDefinition->AttachSocketName;
		}
	}

	return true;
}
