#include "Game/LastFPSGameInstance.h"

#include "CommonLocalPlayer.h"
#include "Data/Definitions/LastFPSCharacterRoster.h"
#include "Data/Tables/LastFPSLoadingTipData.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Game/Travel/LastFPSLevelTravelSettings.h"
#include "Game/Travel/LastFPSLevelTravelSubsystem.h"
#include "UI/Framework/LastFPSUIManagerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LastFPSGameInstance)

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSTravel, Log, All);

namespace LastFPSTravelConsole
{
	static bool HandleTravelCommand(UWorld* World, const TArray<FString>& Args)
	{
		if (!World)
		{
			return false;
		}

		ULastFPSGameInstance* LastGI = Cast<ULastFPSGameInstance>(World->GetGameInstance());
		if (!LastGI)
		{
			UE_LOG(LogLastFPSTravel, Warning,
				TEXT("LastFPS.Travel: ULastFPSGameInstance를 찾을 수 없습니다."));
			return false;
		}

		const FString Argument = Args.IsEmpty() ? FString() : Args[0];
		ELastFPSTravelDestination Destination;
		if (Argument.Equals(TEXT("MainMenu"), ESearchCase::IgnoreCase))
		{
			Destination = ELastFPSTravelDestination::MainMenu;
		}
		else if (Argument.Equals(TEXT("CharacterSelect"), ESearchCase::IgnoreCase)
			|| Argument.Equals(TEXT("CharSelect"), ESearchCase::IgnoreCase))
		{
			Destination = ELastFPSTravelDestination::CharacterSelect;
		}
		else if (Argument.Equals(TEXT("Hub"), ESearchCase::IgnoreCase))
		{
			Destination = ELastFPSTravelDestination::Hub;
		}
		else
		{
			UE_LOG(LogLastFPSTravel, Warning,
				TEXT("사용법: LastFPS.Travel MainMenu|CharacterSelect|Hub"));
			return false;
		}

		LastGI->RequestTravelToDestination(Destination);
		return true;
	}

	static FAutoConsoleCommandWithWorldAndArgs TravelCommand(
		TEXT("LastFPS.Travel"),
		TEXT("설정된 로컬 맵으로 이동합니다. MainMenu | CharacterSelect | Hub"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& Args, UWorld* World)
			{
				HandleTravelCommand(World, Args);
			}));
}

FText ULastFPSGameInstance::GetDefaultStatusTextForDestination(
	const ELastFPSTravelDestination Destination)
{
	switch (Destination)
	{
	case ELastFPSTravelDestination::MainMenu:
		return NSLOCTEXT("LastFPS", "TravelStatus_MainMenu", "메인 메뉴로 이동 중...");
	case ELastFPSTravelDestination::CharacterSelect:
		return NSLOCTEXT("LastFPS", "TravelStatus_CharacterSelect", "캐릭터 선택으로 이동 중...");
	case ELastFPSTravelDestination::Hub:
		return NSLOCTEXT("LastFPS", "TravelStatus_Hub", "허브로 이동 중...");
	default:
		return NSLOCTEXT("LastFPS", "TravelStatus_Generic", "맵 이동 중...");
	}
}

FText ULastFPSGameInstance::GetDefaultMapNameTextForDestination(
	const ELastFPSTravelDestination Destination)
{
	switch (Destination)
	{
	case ELastFPSTravelDestination::MainMenu:
		return NSLOCTEXT("LastFPS", "TravelMap_MainMenu", "Main Menu");
	case ELastFPSTravelDestination::CharacterSelect:
		return NSLOCTEXT("LastFPS", "TravelMap_CharacterSelect", "Character Select");
	case ELastFPSTravelDestination::Hub:
		return NSLOCTEXT("LastFPS", "TravelMap_Hub", "Hub");
	default:
		return FText::GetEmpty();
	}
}

void ULastFPSGameInstance::Init()
{
	Super::Init();
	PreloadLoadingTipTextures();
}

void ULastFPSGameInstance::Shutdown()
{
	Super::Shutdown();
}

int32 ULastFPSGameInstance::AddLocalPlayer(
	ULocalPlayer* NewPlayer,
	const FPlatformUserId UserId)
{
	const int32 Result = Super::AddLocalPlayer(NewPlayer, UserId);
	if (Result != INDEX_NONE)
	{
		if (ULastFPSUIManagerSubsystem* UIManager =
			GetSubsystem<ULastFPSUIManagerSubsystem>())
		{
			if (UCommonLocalPlayer* CommonPlayer = Cast<UCommonLocalPlayer>(NewPlayer))
			{
				UIManager->NotifyPlayerAdded(CommonPlayer);
			}
		}
	}
	return Result;
}

bool ULastFPSGameInstance::RemoveLocalPlayer(ULocalPlayer* ExistingPlayer)
{
	if (ULastFPSUIManagerSubsystem* UIManager =
		GetSubsystem<ULastFPSUIManagerSubsystem>())
	{
		if (UCommonLocalPlayer* CommonPlayer = Cast<UCommonLocalPlayer>(ExistingPlayer))
		{
			UIManager->NotifyPlayerDestroyed(CommonPlayer);
		}
	}
	return Super::RemoveLocalPlayer(ExistingPlayer);
}

void ULastFPSGameInstance::SaveSelectedCharacterIndex(
	const FString& PlayerKey,
	const int32 SelectedIndex)
{
	if (!PlayerKey.IsEmpty())
	{
		SelectedCharacterIndexByPlayerKey.Add(PlayerKey, FMath::Max(0, SelectedIndex));
	}
}

bool ULastFPSGameInstance::TryGetSelectedCharacterIndex(
	const FString& PlayerKey,
	int32& OutSelectedIndex) const
{
	if (const int32* FoundIndex = SelectedCharacterIndexByPlayerKey.Find(PlayerKey))
	{
		OutSelectedIndex = *FoundIndex;
		return true;
	}
	return false;
}

const ULastFPSCharacterRoster* ULastFPSGameInstance::GetCharacterRoster()
{
	if (!CachedCharacterRoster)
	{
		CachedCharacterRoster = CharacterRosterAsset.LoadSynchronous();
		if (!CachedCharacterRoster)
		{
			UE_LOG(LogLastFPSTravel, Error,
				TEXT("CharacterRosterAsset을 불러오지 못했습니다. DefaultGame.ini 설정을 확인하세요."));
		}
	}
	return CachedCharacterRoster;
}

UDataTable* ULastFPSGameInstance::GetLoadingTipTable()
{
	return LoadingTipTable.LoadSynchronous();
}

void ULastFPSGameInstance::PreloadLoadingTipTextures()
{
	UDataTable* Table = LoadingTipTable.LoadSynchronous();
	if (!Table)
	{
		UE_LOG(LogLastFPSTravel, Warning,
			TEXT("LoadingTipTable을 불러오지 못했습니다. DefaultGame.ini 설정을 확인하세요."));
		return;
	}

	TArray<FLastFPSLoadingTipData*> Rows;
	Table->GetAllRows<FLastFPSLoadingTipData>(TEXT("PreloadLoadingTips"), Rows);

	PreloadedLoadingTipTextures.Reset();
	for (const FLastFPSLoadingTipData* Row : Rows)
	{
		if (!Row || Row->Image.IsNull())
		{
			continue;
		}

		if (UTexture2D* Texture = Row->Image.LoadSynchronous())
		{
			// 첫 로딩 화면에서 이미지 스트리밍 지연이 발생하지 않도록 게임 인스턴스가 수명을 유지한다.
			Texture->SetForceMipLevelsToBeResident(3600.0f);
			Texture->WaitForStreaming();
			PreloadedLoadingTipTextures.Add(Texture);
		}
	}

	UE_LOG(LogLastFPSTravel, Log,
		TEXT("로딩 팁 이미지 사전 로드 완료: %d개"),
		PreloadedLoadingTipTextures.Num());
}

bool ULastFPSGameInstance::ResolveMapURL(
	const ELastFPSTravelDestination Destination,
	FString& OutMapURL) const
{
	const ULastFPSLevelTravelSettings* Settings =
		GetDefault<ULastFPSLevelTravelSettings>();
	const ULastFPSLevelTravelSubsystem* Travel =
		GetSubsystem<ULastFPSLevelTravelSubsystem>();
	return Settings
		&& Travel
		&& Travel->ResolveMapPackageName(Settings->GetMapId(Destination), OutMapURL);
}

void ULastFPSGameInstance::RequestTravelToDestination(
	const ELastFPSTravelDestination Destination)
{
	if (ULastFPSLevelTravelSubsystem* Travel =
		GetSubsystem<ULastFPSLevelTravelSubsystem>())
	{
		Travel->TravelToDestination(Destination);
	}
}

void ULastFPSGameInstance::RequestTravelToMainMenu()
{
	RequestTravelToDestination(ELastFPSTravelDestination::MainMenu);
}

void ULastFPSGameInstance::RequestTravelToCharacterSelect()
{
	RequestTravelToDestination(ELastFPSTravelDestination::CharacterSelect);
}

void ULastFPSGameInstance::RequestTravelToHub()
{
	RequestTravelToDestination(ELastFPSTravelDestination::Hub);
}

ELastFPSTravelDestination ULastFPSGameInstance::GetPendingTravelDestination() const
{
	const ULastFPSLevelTravelSubsystem* Travel =
		GetSubsystem<ULastFPSLevelTravelSubsystem>();
	return Travel
		? Travel->GetPendingLocalDestination()
		: ELastFPSTravelDestination::MainMenu;
}

FText ULastFPSGameInstance::GetPendingTravelStatusText() const
{
	const ULastFPSLevelTravelSubsystem* Travel =
		GetSubsystem<ULastFPSLevelTravelSubsystem>();
	return Travel ? Travel->GetPendingTravelStatusText() : FText::GetEmpty();
}

FText ULastFPSGameInstance::GetPendingTravelMapNameText() const
{
	const ULastFPSLevelTravelSubsystem* Travel =
		GetSubsystem<ULastFPSLevelTravelSubsystem>();
	return Travel ? Travel->GetPendingTravelMapNameText() : FText::GetEmpty();
}
