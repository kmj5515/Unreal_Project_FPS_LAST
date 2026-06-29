#include "Game/LastFPSGameInstance.h"

#include "Data/Definitions/LastFPSCharacterRoster.h"
#include "UI/Framework/LastFPSUIManagerSubsystem.h"

#include "CommonLocalPlayer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LastFPSGameInstance)

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSTravel, Log, All);

namespace LastFPSTravelInternal
{
	static FString NormalizeMapURL(const FString& MapURL)
	{
		if (MapURL.IsEmpty())
		{
			return MapURL;
		}

		FString Normalized = MapURL;
		Normalized.TrimStartAndEndInline();
		Normalized.ReplaceInline(TEXT("\\"), TEXT("/"));

		if (!Normalized.StartsWith(TEXT("/")))
		{
			Normalized = TEXT("/") + Normalized;
		}

		if (!Normalized.Contains(TEXT(".")))
		{
			const FString AssetName = FPaths::GetBaseFilename(Normalized);
			if (!AssetName.IsEmpty())
			{
				Normalized += TEXT(".") + AssetName;
			}
		}

		return Normalized;
	}

	static FString GetPackagePathFromMapURL(const FString& NormalizedMapURL)
	{
		FString PackagePath = NormalizedMapURL;
		int32 DotIndex = INDEX_NONE;
		if (PackagePath.FindChar(TEXT('.'), DotIndex))
		{
			PackagePath = PackagePath.Left(DotIndex);
		}
		return PackagePath;
	}
}

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
			UE_LOG(LogLastFPSTravel, Warning, TEXT("LastFPS.Travel: GameInstance is not ULastFPSGameInstance"));
			return false;
		}

		const FString Arg = Args.Num() > 0 ? Args[0] : FString();
		if (Arg.IsEmpty())
		{
			UE_LOG(LogLastFPSTravel, Warning,
				TEXT("Usage: LastFPS.Travel MainMenu|CharacterSelect|Hub"));
			return false;
		}

		ELastFPSTravelDestination Destination = ELastFPSTravelDestination::MainMenu;
		if (Arg.Equals(TEXT("MainMenu"), ESearchCase::IgnoreCase))
		{
			Destination = ELastFPSTravelDestination::MainMenu;
		}
		else if (Arg.Equals(TEXT("CharacterSelect"), ESearchCase::IgnoreCase)
			|| Arg.Equals(TEXT("CharSelect"), ESearchCase::IgnoreCase))
		{
			Destination = ELastFPSTravelDestination::CharacterSelect;
		}
		else if (Arg.Equals(TEXT("Hub"), ESearchCase::IgnoreCase))
		{
			Destination = ELastFPSTravelDestination::Hub;
		}
		else
		{
			UE_LOG(LogLastFPSTravel, Warning, TEXT("Unknown destination: %s"), *Arg);
			return false;
		}

		LastGI->RequestTravelToDestination(Destination);
		return true;
	}

	static FAutoConsoleCommandWithWorldAndArgs CCmdTravel(
		TEXT("LastFPS.Travel"),
		TEXT("Travel to a configured map. Args: MainMenu | CharacterSelect | Hub"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& Args, UWorld* World)
			{
				HandleTravelCommand(World, Args);
			}));
}

FText ULastFPSGameInstance::GetDefaultStatusTextForDestination(const ELastFPSTravelDestination Destination)
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
		return NSLOCTEXT("LastFPS", "TravelStatus_Generic", "맵 로딩 중...");
	}
}

FText ULastFPSGameInstance::GetDefaultMapNameTextForDestination(const ELastFPSTravelDestination Destination)
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

	PostLoadMapDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &ULastFPSGameInstance::HandlePostLoadMap);
}

void ULastFPSGameInstance::Shutdown()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapDelegateHandle);
	Super::Shutdown();
}

int32 ULastFPSGameInstance::AddLocalPlayer(ULocalPlayer* NewPlayer, FPlatformUserId UserId)
{
	const int32 Result = Super::AddLocalPlayer(NewPlayer, UserId);
	if (Result != INDEX_NONE)
	{
		if (ULastFPSUIManagerSubsystem* UIManager = GetSubsystem<ULastFPSUIManagerSubsystem>())
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
	if (ULastFPSUIManagerSubsystem* UIManager = GetSubsystem<ULastFPSUIManagerSubsystem>())
	{
		if (UCommonLocalPlayer* CommonPlayer = Cast<UCommonLocalPlayer>(ExistingPlayer))
		{
			UIManager->NotifyPlayerDestroyed(CommonPlayer);
		}
	}
	return Super::RemoveLocalPlayer(ExistingPlayer);
}

void ULastFPSGameInstance::SaveSelectedCharacterIndex(const FString& PlayerKey, const int32 SelectedIndex)
{
	if (PlayerKey.IsEmpty())
	{
		return;
	}
	SelectedCharacterIndexByPlayerKey.Add(PlayerKey, FMath::Max(0, SelectedIndex));
}

bool ULastFPSGameInstance::TryGetSelectedCharacterIndex(const FString& PlayerKey, int32& OutSelectedIndex) const
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
	if (CachedCharacterRoster)
	{
		return CachedCharacterRoster;
	}

	CachedCharacterRoster = CharacterRosterAsset.LoadSynchronous();
	if (!CachedCharacterRoster)
	{
		UE_LOG(LogLastFPSTravel, Error,
			TEXT("CharacterRoster 미지정 — DefaultGame.ini의 [/Script/LastFPS.LastFPSGameInstance] CharacterRosterAsset 경로를 설정하세요."));
	}
	return CachedCharacterRoster;
}

bool ULastFPSGameInstance::ResolveMapURL(const ELastFPSTravelDestination Destination, FString& OutMapURL) const
{
	switch (Destination)
	{
	case ELastFPSTravelDestination::MainMenu:
		OutMapURL = MainMenuMap;
		break;
	case ELastFPSTravelDestination::CharacterSelect:
		OutMapURL = CharacterSelectMap;
		break;
	case ELastFPSTravelDestination::Hub:
		OutMapURL = HubMap;
		break;
	default:
		OutMapURL.Reset();
		return false;
	}

	return !OutMapURL.IsEmpty();
}

void ULastFPSGameInstance::SetPendingTravelPresentation(
	const ELastFPSTravelDestination Destination,
	const FText& StatusText,
	const FText& MapNameText)
{
	PendingTravelDestination = Destination;
	PendingTravelStatusText = StatusText;
	PendingTravelMapNameText = MapNameText;
	OnTravelPresentationChanged.Broadcast(PendingTravelStatusText, PendingTravelMapNameText);
}

void ULastFPSGameInstance::ClearPendingTravelPresentation()
{
	PendingTravelStatusText = FText::GetEmpty();
	PendingTravelMapNameText = FText::GetEmpty();
	OnTravelPresentationChanged.Broadcast(PendingTravelStatusText, PendingTravelMapNameText);
}

void ULastFPSGameInstance::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld->GetGameInstance() != this)
	{
		return;
	}

	ClearPendingTravelPresentation();
	UE_LOG(LogLastFPSTravel, Log, TEXT("PostLoadMap: %s"), *LoadedWorld->GetMapName());
}

void ULastFPSGameInstance::ExecuteServerTravel(const FString& MapURL, const ELastFPSTravelDestination DestinationForUI)
{
	if (MapURL.IsEmpty())
	{
		UE_LOG(LogLastFPSTravel, Error, TEXT("ExecuteServerTravel: empty MapURL"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogLastFPSTravel, Error, TEXT("ExecuteServerTravel: no world"));
		return;
	}

	const FString NormalizedURL = LastFPSTravelInternal::NormalizeMapURL(MapURL);
	const FString PackagePath = LastFPSTravelInternal::GetPackagePathFromMapURL(NormalizedURL);
	if (!FPackageName::DoesPackageExist(PackagePath))
	{
		UE_LOG(LogLastFPSTravel, Error,
			TEXT("Map package does not exist: %s (configured: %s)"),
			*PackagePath,
			*MapURL);
		return;
	}

	SetPendingTravelPresentation(
		DestinationForUI,
		GetDefaultStatusTextForDestination(DestinationForUI),
		GetDefaultMapNameTextForDestination(DestinationForUI));

	const ENetMode NetMode = World->GetNetMode();
	UE_LOG(LogLastFPSTravel, Log, TEXT("Travel scheduled -> %s (%s, NetMode=%d)"),
		*PackagePath,
		*StaticEnum<ELastFPSTravelDestination>()->GetNameStringByValue(static_cast<int64>(DestinationForUI)),
		static_cast<int32>(NetMode));

	// UI 버튼 콜백 안에서 즉시 Travel 하면 PIE/에디터가 죽는 경우가 있어 1틱 지연
	World->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(this, [this, PackagePath, NetMode]()
		{
			UWorld* TravelWorld = GetWorld();
			if (!TravelWorld)
			{
				return;
			}

			if (NetMode == NM_Client)
			{
				UE_LOG(LogLastFPSTravel, Error,
					TEXT("Client cannot travel. Use Standalone PIE or Play -> Standalone Game."));
				return;
			}

			// PIE(Standalone/ListenServer): OpenLevel. ServerTravel URL에는 '.MapName' 접미사 불가(FURL 거부).
			if (NetMode == NM_Standalone || NetMode == NM_ListenServer)
			{
				UE_LOG(LogLastFPSTravel, Log, TEXT("OpenLevel: %s"), *PackagePath);
				UGameplayStatics::OpenLevel(TravelWorld, FName(*PackagePath), true);
				return;
			}

			UE_LOG(LogLastFPSTravel, Log, TEXT("ServerTravel: %s"), *PackagePath);
			TravelWorld->ServerTravel(PackagePath);
		}));
}

void ULastFPSGameInstance::RequestTravelToDestination(const ELastFPSTravelDestination Destination)
{
	FString MapURL;
	if (!ResolveMapURL(Destination, MapURL))
	{
		UE_LOG(LogLastFPSTravel, Error, TEXT("RequestTravelToDestination: no URL for %s"),
			*StaticEnum<ELastFPSTravelDestination>()->GetNameStringByValue(static_cast<int64>(Destination)));
		return;
	}

	ExecuteServerTravel(MapURL, Destination);
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
