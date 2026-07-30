#include "Game/Travel/LastFPSLevelTravelSubsystem.h"

#include "CommonSessionSubsystem.h"
#include "CommonUserTypes.h"
#include "Data/Definitions/LastFPSBattleDefinition.h"
#include "Data/Definitions/LastFPSDestinationContentSet.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/Loading/LastFPSLoadingProcessSubsystem.h"
#include "Game/Travel/LastFPSLevelTravelSettings.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Online/OnlineSessionNames.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LastFPSLevelTravelSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSLevelTravel, Log, All);

namespace LastFPSLevelTravelPresentation
{
	FText GetStatusText(const ELastFPSTravelDestination Destination)
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

	FText GetMapNameText(const ELastFPSTravelDestination Destination)
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
}

void ULastFPSLevelTravelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UCommonSessionSubsystem>();

	if (UCommonSessionSubsystem* Sessions = GetCommonSessionSubsystem())
	{
		CreateSessionCompleteDelegateHandle = Sessions->OnCreateSessionCompleteEvent.AddUObject(
			this, &ULastFPSLevelTravelSubsystem::HandleCreateSessionComplete);
		JoinSessionCompleteDelegateHandle = Sessions->OnJoinSessionCompleteEvent.AddUObject(
			this, &ULastFPSLevelTravelSubsystem::HandleJoinSessionComplete);
	}

	PostLoadMapDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &ULastFPSLevelTravelSubsystem::HandlePostLoadMap);
	if (GEngine)
	{
		TravelFailureDelegateHandle = GEngine->OnTravelFailure().AddUObject(
			this, &ULastFPSLevelTravelSubsystem::HandleTravelFailure);
	}

	if (ULastFPSLoadingProcessSubsystem* Loading = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULastFPSLoadingProcessSubsystem>()
		: nullptr)
	{
		LoadingFinishedDelegateHandle = Loading->OnLoadingFinished.AddUObject(
			this, &ULastFPSLevelTravelSubsystem::HandleLoadingFinished);
	}
}

void ULastFPSLevelTravelSubsystem::Deinitialize()
{
	if (UCommonSessionSubsystem* Sessions = GetCommonSessionSubsystem())
	{
		Sessions->OnCreateSessionCompleteEvent.Remove(CreateSessionCompleteDelegateHandle);
		Sessions->OnJoinSessionCompleteEvent.Remove(JoinSessionCompleteDelegateHandle);
	}

	if (ULastFPSLoadingProcessSubsystem* Loading = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULastFPSLoadingProcessSubsystem>()
		: nullptr)
	{
		Loading->OnLoadingFinished.Remove(LoadingFinishedDelegateHandle);
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapDelegateHandle);
	if (GEngine)
	{
		GEngine->OnTravelFailure().Remove(TravelFailureDelegateHandle);
	}
	ResetPendingBattleRequest();
	Super::Deinitialize();
}

ELastFPSTravelRequestResult ULastFPSLevelTravelSubsystem::RequestTravel(
	APlayerController* LocalPlayerController,
	const FLastFPSTravelEntryRequest& Request)
{
	switch (Request.Type)
	{
	case ELastFPSTravelEntryType::LocalDestination:
		return TravelToDestination(Request.Destination);

	case ELastFPSTravelEntryType::QuickPlayBattle:
		return QuickPlayBattle(
			LocalPlayerController,
			Request.BattleDefinitionId);

	default:
		FailRequest(
			ELastFPSTravelRequestResult::InvalidDefinition,
			NSLOCTEXT(
				"LastFPS",
				"TravelUnsupportedEntryType",
				"지원하지 않는 이동 요청입니다."));
		return ELastFPSTravelRequestResult::InvalidDefinition;
	}
}

ELastFPSTravelRequestResult ULastFPSLevelTravelSubsystem::TravelToDestination(
	const ELastFPSTravelDestination Destination)
{
	const ULastFPSLevelTravelSettings* Settings = GetDefault<ULastFPSLevelTravelSettings>();
	if (!Settings)
	{
		return ELastFPSTravelRequestResult::InvalidAssetId;
	}

	const ELastFPSTravelReadiness Readiness =
		Destination == ELastFPSTravelDestination::Hub
		? ELastFPSTravelReadiness::LevelControllerAndPawn
		: ELastFPSTravelReadiness::LevelAndController;

	const ELastFPSTravelRequestResult Result = TravelToMap(
		Settings->GetMapId(Destination),
		Readiness,
		LastFPSLevelTravelPresentation::GetStatusText(Destination),
		LastFPSLevelTravelPresentation::GetMapNameText(Destination));
	if (Result == ELastFPSTravelRequestResult::Accepted)
	{
		PendingLocalDestination = Destination;
	}
	return Result;
}

ELastFPSTravelRequestResult ULastFPSLevelTravelSubsystem::TravelToMap(
	const FPrimaryAssetId MapId,
	const ELastFPSTravelReadiness Readiness,
	FText StatusText,
	FText MapNameText)
{
	if (IsBusy())
	{
		return ELastFPSTravelRequestResult::Busy;
	}
	if (!MapId.IsValid() || MapId.PrimaryAssetType != FPrimaryAssetType(TEXT("Map")))
	{
		FailRequest(
			ELastFPSTravelRequestResult::InvalidAssetId,
			FText::Format(
				NSLOCTEXT("LastFPS", "TravelInvalidMapId", "맵 Primary Asset ID가 올바르지 않습니다: {0}"),
				FText::FromString(MapId.ToString())));
		return ELastFPSTravelRequestResult::InvalidAssetId;
	}

	FString PackageName;
	if (!ResolveMapPackageName(MapId, PackageName))
	{
		const FText Error = FText::Format(
			NSLOCTEXT("LastFPS", "TravelInvalidMap", "등록된 맵을 찾을 수 없습니다: {0}"),
			FText::FromString(MapId.ToString()));
		FailRequest(ELastFPSTravelRequestResult::AssetNotFound, Error);
		return ELastFPSTravelRequestResult::AssetNotFound;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		FailRequest(
			ELastFPSTravelRequestResult::InvalidWorld,
			NSLOCTEXT("LastFPS", "TravelInvalidWorld", "이동할 월드를 찾을 수 없습니다."));
		return ELastFPSTravelRequestResult::InvalidWorld;
	}

	if (StatusText.IsEmpty())
	{
		StatusText = NSLOCTEXT("LastFPS", "TravelStatusGeneric", "맵 이동 중...");
	}
	if (MapNameText.IsEmpty())
	{
		MapNameText = FText::FromName(MapId.PrimaryAssetName);
	}

	SetPresentation(StatusText, MapNameText);
	SetTravelState(ELastFPSTravelState::Travelling);

	if (ULastFPSLoadingProcessSubsystem* Loading =
		GetGameInstance()->GetSubsystem<ULastFPSLoadingProcessSubsystem>())
	{
		Loading->BeginTravelLoading(Readiness);
	}

	const ENetMode NetMode = World->GetNetMode();
	if ((NetMode == NM_Client || NetMode == NM_ListenServer)
		&& GetCommonSessionSubsystem())
	{
		// 전투 세션을 떠나 로컬 화면으로 복귀할 때 온라인 상태가 남지 않게 정리한다.
		GetCommonSessionSubsystem()->CleanUpSessions();
	}

	World->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(this, [this, PackageName]()
		{
			UWorld* TravelWorld = GetWorld();
			if (!TravelWorld)
			{
				FailRequest(
					ELastFPSTravelRequestResult::InvalidWorld,
					NSLOCTEXT("LastFPS", "TravelWorldExpired", "이동 전에 월드가 종료되었습니다."));
				return;
			}

			if (TravelWorld->GetNetMode() == NM_DedicatedServer)
			{
				TravelWorld->ServerTravel(PackageName);
				return;
			}

			// 로컬 화면은 각 클라이언트가 독립적으로 열며, 전투 서버 권한과 분리한다.
			UGameplayStatics::OpenLevel(TravelWorld, FName(*PackageName), true);
		}));

	UE_LOG(LogLastFPSLevelTravel, Log, TEXT("로컬 맵 이동 예약: %s"), *PackageName);
	return ELastFPSTravelRequestResult::Accepted;
}

ELastFPSTravelRequestResult ULastFPSLevelTravelSubsystem::QuickPlayBattle(
	APlayerController* LocalPlayerController,
	const FPrimaryAssetId BattleDefinitionId)
{
	if (IsBusy())
	{
		return ELastFPSTravelRequestResult::Busy;
	}
	if (!IsValid(LocalPlayerController)
		|| !LocalPlayerController->IsLocalController()
		|| !LocalPlayerController->GetLocalPlayer())
	{
		FailRequest(
			ELastFPSTravelRequestResult::InvalidPlayer,
			NSLOCTEXT("LastFPS", "QuickPlayInvalidPlayer", "퀵플레이를 요청할 로컬 플레이어가 올바르지 않습니다."));
		return ELastFPSTravelRequestResult::InvalidPlayer;
	}
	if (!BattleDefinitionId.IsValid()
		|| BattleDefinitionId.PrimaryAssetType != ULastFPSBattleDefinition::PrimaryAssetType)
	{
		FailRequest(
			ELastFPSTravelRequestResult::InvalidAssetId,
			FText::Format(
				NSLOCTEXT(
					"LastFPS",
					"QuickPlayInvalidDefinitionId",
					"BattleDefinition Primary Asset ID가 올바르지 않습니다: {0}"),
				FText::FromString(BattleDefinitionId.ToString())));
		return ELastFPSTravelRequestResult::InvalidAssetId;
	}
	if (!GetCommonSessionSubsystem())
	{
		FailRequest(
			ELastFPSTravelRequestResult::SessionUnavailable,
			NSLOCTEXT("LastFPS", "QuickPlayNoSessionSubsystem", "CommonSessionSubsystem을 사용할 수 없습니다."));
		return ELastFPSTravelRequestResult::SessionUnavailable;
	}

	PendingPlayerController = LocalPlayerController;
	PendingBattleDefinitionId = BattleDefinitionId;
	SetTravelState(ELastFPSTravelState::LoadingDefinition);

	TArray<FName> BundlesToLoad;
	BundlesToLoad.Add(TEXT("Game"));
	PendingDefinitionLoadHandle = UAssetManager::Get().LoadPrimaryAsset(
		BattleDefinitionId,
		BundlesToLoad,
		FStreamableDelegate::CreateUObject(
			this, &ULastFPSLevelTravelSubsystem::HandleBattleDefinitionLoaded));

	if (!PendingDefinitionLoadHandle.IsValid())
	{
		FailRequest(
			ELastFPSTravelRequestResult::AssetNotFound,
			FText::Format(
				NSLOCTEXT("LastFPS", "BattleDefinitionNotFound", "전투 정의를 불러올 수 없습니다: {0}"),
				FText::FromString(BattleDefinitionId.ToString())));
		return ELastFPSTravelRequestResult::AssetNotFound;
	}

	return ELastFPSTravelRequestResult::Accepted;
}

bool ULastFPSLevelTravelSubsystem::ResolveMapPackageName(
	const FPrimaryAssetId MapId,
	FString& OutPackageName) const
{
	OutPackageName.Reset();
	if (!MapId.IsValid() || MapId.PrimaryAssetType != FPrimaryAssetType(TEXT("Map")))
	{
		return false;
	}

	FAssetData MapAssetData;
	if (!UAssetManager::Get().GetPrimaryAssetData(MapId, MapAssetData))
	{
		return false;
	}

	OutPackageName = MapAssetData.PackageName.ToString();
	return !OutPackageName.IsEmpty() && FPackageName::DoesPackageExist(OutPackageName);
}

void ULastFPSLevelTravelSubsystem::HandleBattleDefinitionLoaded()
{
	PendingDefinitionLoadHandle.Reset();

	PendingBattleDefinition = Cast<ULastFPSBattleDefinition>(
		UAssetManager::Get().GetPrimaryAssetObject(PendingBattleDefinitionId));
	if (!PendingBattleDefinition
		|| !PendingBattleDefinition->MatchmakingPoolTag.IsValid()
		|| PendingBattleDefinition->MaxPlayerCount < 1
		|| !PendingBattleDefinition->ContentSet.Get())
	{
		FailRequest(
			ELastFPSTravelRequestResult::InvalidDefinition,
			FText::Format(
				NSLOCTEXT("LastFPS", "BattleDefinitionInvalid", "전투 정의의 필수 값이 올바르지 않습니다: {0}"),
				FText::FromString(PendingBattleDefinitionId.ToString())));
		return;
	}

	FString MapPackageName;
	if (!ResolveMapPackageName(PendingBattleDefinition->MapId, MapPackageName))
	{
		FailRequest(
			ELastFPSTravelRequestResult::AssetNotFound,
			FText::Format(
				NSLOCTEXT("LastFPS", "BattleMapInvalid", "전투 맵이 Asset Manager에 등록되지 않았습니다: {0}"),
				FText::FromString(PendingBattleDefinition->MapId.ToString())));
		return;
	}

	UCommonSessionSubsystem* Sessions = GetCommonSessionSubsystem();
	APlayerController* PlayerController = PendingPlayerController.Get();
	if (!Sessions || !IsValid(PlayerController))
	{
		FailRequest(
			ELastFPSTravelRequestResult::SessionUnavailable,
			NSLOCTEXT("LastFPS", "BattleSessionUnavailable", "세션 시스템 또는 로컬 플레이어를 사용할 수 없습니다."));
		return;
	}

	PendingHostRequest = Sessions->CreateOnlineHostSessionRequest();
	PendingSearchRequest = Sessions->CreateOnlineSearchSessionRequest();
	if (!PendingHostRequest || !PendingSearchRequest)
	{
		FailRequest(
			ELastFPSTravelRequestResult::SessionUnavailable,
			NSLOCTEXT("LastFPS", "BattleSessionRequestInvalid", "세션 요청을 생성할 수 없습니다."));
		return;
	}

	PendingHostRequest->OnlineMode = ECommonSessionOnlineMode::Online;
	PendingHostRequest->MapID = PendingBattleDefinition->MapId;
	PendingHostRequest->ModeNameForAdvertisement =
		PendingBattleDefinition->MatchmakingPoolTag.ToString();
	PendingHostRequest->MaxPlayerCount = PendingBattleDefinition->MaxPlayerCount;
	PendingHostRequest->ExtraArgs.Add(
		TEXT("BattleDefinition"),
		PendingBattleDefinitionId.PrimaryAssetName.ToString());

	PendingSearchRequest->OnlineMode = ECommonSessionOnlineMode::Online;
	PendingSearchRequest->OnSearchFinished.AddUObject(
		this, &ULastFPSLevelTravelSubsystem::HandleQuickPlaySearchFinished);

	SetTravelState(ELastFPSTravelState::SearchingSession);
	Sessions->FindSessions(PlayerController, PendingSearchRequest);
}

void ULastFPSLevelTravelSubsystem::HandleQuickPlaySearchFinished(
	const bool bSucceeded,
	const FText& ErrorMessage)
{
	if (!PendingBattleDefinition || !PendingSearchRequest)
	{
		FailRequest(
			ELastFPSTravelRequestResult::InvalidDefinition,
			NSLOCTEXT("LastFPS", "QuickPlayStateLost", "퀵플레이 요청 상태가 유실되었습니다."));
		return;
	}

	// 일부 온라인 서비스는 검색 결과가 없는 경우 실패와 빈 오류를 함께 반환한다.
	if (!bSucceeded && !ErrorMessage.IsEmpty())
	{
		FailRequest(ELastFPSTravelRequestResult::SessionUnavailable, ErrorMessage);
		return;
	}

	UCommonSessionSubsystem* Sessions = GetCommonSessionSubsystem();
	APlayerController* PlayerController = PendingPlayerController.Get();
	if (!Sessions || !IsValid(PlayerController))
	{
		FailRequest(
			ELastFPSTravelRequestResult::SessionUnavailable,
			NSLOCTEXT("LastFPS", "QuickPlayPlayerLost", "퀵플레이 중 로컬 플레이어가 종료되었습니다."));
		return;
	}

	if (UCommonSession_SearchResult* Match =
		FindBestMatchingSession(*PendingBattleDefinition, *PendingSearchRequest))
	{
		SetTravelState(ELastFPSTravelState::JoiningSession);
		BeginSessionTravelLoading(*PendingBattleDefinition);
		Sessions->JoinSession(PlayerController, Match);
		return;
	}

	SetTravelState(ELastFPSTravelState::HostingSession);
	BeginSessionTravelLoading(*PendingBattleDefinition);
	Sessions->HostSession(PlayerController, PendingHostRequest);
}

UCommonSession_SearchResult* ULastFPSLevelTravelSubsystem::FindBestMatchingSession(
	const ULastFPSBattleDefinition& Definition,
	const UCommonSession_SearchSessionRequest& SearchRequest) const
{
	FString ExpectedMap;
	if (!ResolveMapPackageName(Definition.MapId, ExpectedMap))
	{
		return nullptr;
	}

	const FString ExpectedPool = Definition.MatchmakingPoolTag.ToString();
	UCommonSession_SearchResult* BestResult = nullptr;
	int32 BestPing = MAX_int32;

	for (UCommonSession_SearchResult* Result : SearchRequest.Results)
	{
		if (!Result || Result->GetNumOpenPublicConnections() <= 0)
		{
			continue;
		}

		FString Pool;
		FString Map;
		bool bFoundPool = false;
		bool bFoundMap = false;
		Result->GetStringSetting(SETTING_GAMEMODE, Pool, bFoundPool);
		Result->GetStringSetting(SETTING_MAPNAME, Map, bFoundMap);

		if (!bFoundPool || !bFoundMap || Pool != ExpectedPool || Map != ExpectedMap)
		{
			continue;
		}

		const int32 Ping = Result->GetPingInMs();
		if (!BestResult || Ping < BestPing)
		{
			BestResult = Result;
			BestPing = Ping;
		}
	}

	return BestResult;
}

void ULastFPSLevelTravelSubsystem::BeginSessionTravelLoading(
	const ULastFPSBattleDefinition& Definition)
{
	const FText MapName = Definition.DisplayName.IsEmpty()
		? FText::FromName(Definition.MapId.PrimaryAssetName)
		: Definition.DisplayName;
	SetPresentation(
		NSLOCTEXT("LastFPS", "BattleTravelStatus", "전투에 참가하는 중..."),
		MapName);

	if (ULastFPSLoadingProcessSubsystem* Loading =
		GetGameInstance()->GetSubsystem<ULastFPSLoadingProcessSubsystem>())
	{
		Loading->BeginTravelLoading(ELastFPSTravelReadiness::LevelControllerAndPawn);
	}
}

void ULastFPSLevelTravelSubsystem::HandleCreateSessionComplete(
	const FOnlineResultInformation& Result)
{
	if (TravelState != ELastFPSTravelState::HostingSession)
	{
		return;
	}
	if (!Result.bWasSuccessful)
	{
		FailRequest(
			ELastFPSTravelRequestResult::SessionUnavailable,
			Result.ErrorText.IsEmpty()
				? NSLOCTEXT("LastFPS", "HostSessionFailed", "전투 세션 생성에 실패했습니다.")
				: Result.ErrorText);
		return;
	}

	SetTravelState(ELastFPSTravelState::Travelling);
}

void ULastFPSLevelTravelSubsystem::HandleJoinSessionComplete(
	const FOnlineResultInformation& Result)
{
	if (TravelState != ELastFPSTravelState::JoiningSession)
	{
		return;
	}
	if (!Result.bWasSuccessful)
	{
		FailRequest(
			ELastFPSTravelRequestResult::SessionUnavailable,
			Result.ErrorText.IsEmpty()
				? NSLOCTEXT("LastFPS", "JoinSessionFailed", "전투 세션 참가에 실패했습니다.")
				: Result.ErrorText);
		return;
	}

	SetTravelState(ELastFPSTravelState::Travelling);
}

void ULastFPSLevelTravelSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	if (ULastFPSLoadingProcessSubsystem* Loading =
		GetGameInstance()->GetSubsystem<ULastFPSLoadingProcessSubsystem>())
	{
		Loading->NotifyLevelLoadComplete(LoadedWorld);
	}

	if (TravelState != ELastFPSTravelState::Idle)
	{
		SetTravelState(ELastFPSTravelState::Idle);
		ResetPendingBattleRequest();
	}
}

void ULastFPSLevelTravelSubsystem::HandleLoadingFinished(const bool bSucceeded)
{
	ClearPresentation();
	if (!bSucceeded && TravelState != ELastFPSTravelState::Idle)
	{
		SetTravelState(ELastFPSTravelState::Idle);
		ResetPendingBattleRequest();
	}
}

void ULastFPSLevelTravelSubsystem::HandleTravelFailure(
	UWorld* FailedWorld,
	const ETravelFailure::Type FailureType,
	const FString& Reason)
{
	if (!IsBusy()
		|| !FailedWorld
		|| FailedWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	FailRequest(
		ELastFPSTravelRequestResult::InvalidWorld,
		FText::Format(
			NSLOCTEXT("LastFPS", "TravelFailure", "맵 이동에 실패했습니다. ({0}: {1})"),
			FText::AsNumber(static_cast<int32>(FailureType)),
			FText::FromString(Reason)));
}

void ULastFPSLevelTravelSubsystem::SetTravelState(const ELastFPSTravelState NewState)
{
	if (TravelState == NewState)
	{
		return;
	}

	const ELastFPSTravelState PreviousState = TravelState;
	TravelState = NewState;
	OnTravelStateChanged.Broadcast(PreviousState, TravelState);
}

void ULastFPSLevelTravelSubsystem::SetPresentation(
	const FText& StatusText,
	const FText& MapNameText)
{
	PendingStatusText = StatusText;
	PendingMapNameText = MapNameText;
	OnTravelPresentationChanged.Broadcast(PendingStatusText, PendingMapNameText);
}

void ULastFPSLevelTravelSubsystem::ClearPresentation()
{
	PendingStatusText = FText::GetEmpty();
	PendingMapNameText = FText::GetEmpty();
	OnTravelPresentationChanged.Broadcast(PendingStatusText, PendingMapNameText);
}

void ULastFPSLevelTravelSubsystem::FailRequest(
	const ELastFPSTravelRequestResult Result,
	const FText& ErrorMessage)
{
	UE_LOG(LogLastFPSLevelTravel, Error, TEXT("이동 요청 실패: Result=%s, Reason=%s"),
		*StaticEnum<ELastFPSTravelRequestResult>()->GetNameStringByValue(static_cast<int64>(Result)),
		*ErrorMessage.ToString());

	if (ULastFPSLoadingProcessSubsystem* Loading = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULastFPSLoadingProcessSubsystem>()
		: nullptr;
		Loading && Loading->IsLoadingActive())
	{
		Loading->CancelLoading(ErrorMessage.ToString());
	}

	ClearPresentation();
	SetTravelState(ELastFPSTravelState::Idle);
	ResetPendingBattleRequest();
	OnTravelRequestFailed.Broadcast(Result, ErrorMessage);
}

void ULastFPSLevelTravelSubsystem::ResetPendingBattleRequest()
{
	if (PendingSearchRequest)
	{
		PendingSearchRequest->OnSearchFinished.RemoveAll(this);
	}

	PendingDefinitionLoadHandle.Reset();
	PendingBattleDefinition = nullptr;
	PendingHostRequest = nullptr;
	PendingSearchRequest = nullptr;
	PendingPlayerController.Reset();
	PendingBattleDefinitionId = FPrimaryAssetId();
}

UCommonSessionSubsystem* ULastFPSLevelTravelSubsystem::GetCommonSessionSubsystem() const
{
	return GetGameInstance()
		? GetGameInstance()->GetSubsystem<UCommonSessionSubsystem>()
		: nullptr;
}
