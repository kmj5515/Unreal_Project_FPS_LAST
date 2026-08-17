#include "Game/Travel/LastFPSLevelTravelSubsystem.h"

#include "CommonSessionSubsystem.h"
#include "CommonUserTypes.h"
#include "Data/AssetManagement/LastFPSPrimaryAssetTypes.h"
#include "Data/Definitions/LastFPSBattleDefinition.h"
#include "Data/Definitions/LastFPSDestinationContentSet.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Game/Loading/LastFPSLoadingIndicatorSubsystem.h"
#include "Game/Loading/LastFPSLoadingProcessSubsystem.h"
#include "Game/Travel/LastFPSLevelTravelSettings.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/LastFPSEquipmentSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Localization/LastFPSLocalization.h"
#include "Misc/PackageName.h"
#include "Network/LastFPSMasterLobbyClientSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "Quest/LastFPSQuestSubsystem.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LastFPSLevelTravelSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSLevelTravel, Log, All);

namespace LastFPSLevelTravelPresentation
{
	/**
	 * 표시 이름이 없을 때 쓰는 최후 폴백.
	 * PrimaryAssetName 은 맵 등록 규칙에 따라 전체 패키지 경로가 들어올 수 있어
	 * 그대로 쓰면 로딩 화면에 "/Game/Maps/..." 가 노출된다. 경로는 항상 잘라낸다.
	 */
	FText GetMapIdFallbackText(const FPrimaryAssetId& MapId)
	{
		const FString AssetName = MapId.PrimaryAssetName.ToString();
		return FText::FromString(
			AssetName.Contains(TEXT("/")) ? FPackageName::GetShortName(AssetName) : AssetName);
	}

	FText GetBattleMapNameText(const ULastFPSBattleDefinition& Definition)
	{
		if (!Definition.DisplayNameStringTableKey.IsNone())
		{
			return FLastFPSLocalization::GetUIText(Definition.DisplayNameStringTableKey);
		}

		if (!Definition.DisplayName.IsEmpty())
		{
			return Definition.DisplayName;
		}

		// 표시 이름이 비어 있으면 데이터 누락이다. 조용히 에셋 이름을 쓰면 원인을 못 찾는다.
		UE_LOG(LogLastFPSLevelTravel, Warning,
			TEXT("전투 정의 '%s' 에 표시 이름이 없어 맵 에셋 이름으로 대체합니다. DisplayName 또는 DisplayNameStringTableKey를 설정하세요."),
			*Definition.GetPrimaryAssetId().ToString());
		return GetMapIdFallbackText(Definition.MapId);
	}

	FText GetStatusText(const ELastFPSTravelDestination Destination)
	{
		switch (Destination)
		{
		case ELastFPSTravelDestination::MainMenu:
			return FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::TravelStatusMainMenu);
		case ELastFPSTravelDestination::CharacterSelect:
			return FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::TravelStatusCharacterSelect);
		case ELastFPSTravelDestination::Hub:
			return FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::TravelStatusHub);
		default:
			return FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::TravelStatusGeneric);
		}
	}

	FText GetMapNameText(const ELastFPSTravelDestination Destination)
	{
		switch (Destination)
		{
		case ELastFPSTravelDestination::MainMenu:
			return FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::TravelMapMainMenu);
		case ELastFPSTravelDestination::CharacterSelect:
			return FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::TravelMapCharacterSelect);
		case ELastFPSTravelDestination::Hub:
			return FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::TravelMapHub);
		default:
			return FText::GetEmpty();
		}
	}
}

void ULastFPSLevelTravelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UCommonSessionSubsystem>();
	Collection.InitializeDependency<ULastFPSLoadingIndicatorSubsystem>();
	Collection.InitializeDependency<ULastFPSEquipmentSubsystem>();
	Collection.InitializeDependency<ULastFPSQuestSubsystem>();

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
	if (!IsTravelRequestUnlocked(Request))
	{
		UE_LOG(LogLastFPSLevelTravel, Log,
			TEXT("퀘스트 조건이 충족되지 않아 이동 요청을 거부했습니다: Type=%s, BattleDefinition=%s"),
			*StaticEnum<ELastFPSTravelEntryType>()->GetNameStringByValue(static_cast<int64>(Request.Type)),
			*Request.BattleDefinitionId.ToString());
		return ELastFPSTravelRequestResult::QuestLocked;
	}

	switch (Request.Type)
	{
	case ELastFPSTravelEntryType::LocalDestination:
		return TravelToDestination(Request.Destination);

	case ELastFPSTravelEntryType::QuickPlayBattle:
	{
		const UGameInstance* GameInstance = GetGameInstance();
		if (const ULastFPSEquipmentSubsystem* Equipment =
			GameInstance ? GameInstance->GetSubsystem<ULastFPSEquipmentSubsystem>() : nullptr;
			!Equipment || !Equipment->HasEquippedWeapon())
		{
			return ELastFPSTravelRequestResult::MissingRequiredWeapon;
		}
		return QuickPlayBattle(
			LocalPlayerController,
			Request.BattleDefinitionId);
	}

	case ELastFPSTravelEntryType::PartyRoomBattle:
	{
		const UGameInstance* GameInstance = GetGameInstance();
		if (const ULastFPSEquipmentSubsystem* Equipment =
			GameInstance ? GameInstance->GetSubsystem<ULastFPSEquipmentSubsystem>() : nullptr;
			!Equipment || !Equipment->HasEquippedWeapon())
		{
			return ELastFPSTravelRequestResult::MissingRequiredWeapon;
		}
		return TravelToPartyRoom(
			LocalPlayerController,
			Request.BattleDefinitionId);
	}

	default:
		FailRequest(
			ELastFPSTravelRequestResult::InvalidDefinition,
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::TravelUnsupportedEntryType));
		return ELastFPSTravelRequestResult::InvalidDefinition;
	}
}

namespace
{
	/**
	 * BattleDefinition을 소비하는 요청인지 판별한다. 퀘스트 조건과 잠금 아이콘은
	 * 이동 수단이 아니라 전투 정의에 붙는 규칙이므로, 진입 방식이 늘어날 때마다
	 * 조건을 각자 고쳐 쓰지 않도록 한곳에서 판단한다.
	 */
	bool UsesBattleDefinition(const ELastFPSTravelEntryType Type)
	{
		return Type == ELastFPSTravelEntryType::QuickPlayBattle
			|| Type == ELastFPSTravelEntryType::PartyRoomBattle;
	}
}

bool ULastFPSLevelTravelSubsystem::IsTravelRequestUnlocked(
	const FLastFPSTravelEntryRequest& Request) const
{
	if (!UsesBattleDefinition(Request.Type))
	{
		return true;
	}

	const ULastFPSLevelTravelSettings* Settings = GetDefault<ULastFPSLevelTravelSettings>();
	const FLastFPSTravelQuestRequirement* Requirement = Settings
		? Settings->FindQuestRequirement(Request.BattleDefinitionId)
		: nullptr;
	if (!Requirement || Requirement->RequiredQuestId.IsNone())
	{
		return true;
	}

	const ULastFPSQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULastFPSQuestSubsystem>()
		: nullptr;
	if (!QuestSubsystem)
	{
		return false;
	}

	switch (QuestSubsystem->GetStatus(Requirement->RequiredQuestId))
	{
	case ELastFPSQuestStatus::InProgress:
	case ELastFPSQuestStatus::Completed:
		return true;
	case ELastFPSQuestStatus::Claimed:
		return Requirement->bAllowReplayAfterClaimed;
	default:
		return false;
	}
}

TSoftObjectPtr<UTexture2D> ULastFPSLevelTravelSubsystem::GetTravelLockedIcon(
	const FLastFPSTravelEntryRequest& Request) const
{
	if (!UsesBattleDefinition(Request.Type))
	{
		return TSoftObjectPtr<UTexture2D>();
	}

	const ULastFPSLevelTravelSettings* Settings = GetDefault<ULastFPSLevelTravelSettings>();
	const FLastFPSTravelQuestRequirement* Requirement = Settings
		? Settings->FindQuestRequirement(Request.BattleDefinitionId)
		: nullptr;
	return Requirement ? Requirement->LockedIcon : TSoftObjectPtr<UTexture2D>();
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
	if (!MapId.IsValid() || MapId.PrimaryAssetType != LastFPSPrimaryAssetTypes::Map)
	{
		FailRequest(
			ELastFPSTravelRequestResult::InvalidAssetId,
			FText::Format(
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::TravelInvalidMapId),
				FText::FromString(MapId.ToString())));
		return ELastFPSTravelRequestResult::InvalidAssetId;
	}

	FString PackageName;
	if (!ResolveMapPackageName(MapId, PackageName))
	{
		const FText Error = FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::TravelInvalidMap),
			FText::FromString(MapId.ToString()));
		FailRequest(ELastFPSTravelRequestResult::AssetNotFound, Error);
		return ELastFPSTravelRequestResult::AssetNotFound;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		FailRequest(
			ELastFPSTravelRequestResult::InvalidWorld,
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::TravelInvalidWorld));
		return ELastFPSTravelRequestResult::InvalidWorld;
	}

	if (StatusText.IsEmpty())
	{
		StatusText = FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::TravelStatusGeneric);
	}
	if (MapNameText.IsEmpty())
	{
		MapNameText = LastFPSLevelTravelPresentation::GetMapIdFallbackText(MapId);
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
					FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::TravelWorldExpired));
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

ELastFPSTravelRequestResult ULastFPSLevelTravelSubsystem::TravelToPartyRoom(
	APlayerController* LocalPlayerController,
	const FPrimaryAssetId BattleDefinitionId)
{
	if (!IsValid(LocalPlayerController) || !LocalPlayerController->IsLocalController())
	{
		FailRequest(
			ELastFPSTravelRequestResult::InvalidPlayer,
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuickPlayInvalidPlayer));
		return ELastFPSTravelRequestResult::InvalidPlayer;
	}

	if (!BattleDefinitionId.IsValid()
		|| BattleDefinitionId.PrimaryAssetType != ULastFPSBattleDefinition::PrimaryAssetType)
	{
		FailRequest(
			ELastFPSTravelRequestResult::InvalidAssetId,
			FText::Format(
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuickPlayInvalidDefinitionId),
				FText::FromString(BattleDefinitionId.ToString())));
		return ELastFPSTravelRequestResult::InvalidAssetId;
	}

	// 여기서 대기 맵을 직접 열면 마스터 로비에 방이 등록되지 않아 파티원이 찾을 수 없다.
	// 방 개설과 Beacon 등록은 마스터 로비 경유 흐름이 소유하므로, 고른 전투만 넘기고 로비로 보낸다.
	ULastFPSMasterLobbyClientSubsystem* MasterLobby = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULastFPSMasterLobbyClientSubsystem>()
		: nullptr;
	if (!MasterLobby)
	{
		UE_LOG(LogLastFPSLevelTravel, Error,
			TEXT("파티 대기실로 이동할 수 없습니다: 마스터 로비 서브시스템이 없습니다."));
		FailRequest(
			ELastFPSTravelRequestResult::SessionUnavailable,
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::BattleSessionUnavailable));
		return ELastFPSTravelRequestResult::SessionUnavailable;
	}

	MasterLobby->SetPendingBattleDefinition(BattleDefinitionId);
	if (!MasterLobby->ConnectToMasterLobby())
	{
		// 실패 사유는 서브시스템이 이미 상세히 남긴다.
		MasterLobby->SetPendingBattleDefinition(FPrimaryAssetId());
		FailRequest(
			ELastFPSTravelRequestResult::SessionUnavailable,
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::BattleSessionUnavailable));
		return ELastFPSTravelRequestResult::SessionUnavailable;
	}

	UE_LOG(LogLastFPSLevelTravel, Log,
		TEXT("마스터 로비를 거쳐 파티 대기실로 이동합니다: BattleDef=%s"), *BattleDefinitionId.ToString());
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
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuickPlayInvalidPlayer));
		return ELastFPSTravelRequestResult::InvalidPlayer;
	}
	if (!BattleDefinitionId.IsValid()
		|| BattleDefinitionId.PrimaryAssetType != ULastFPSBattleDefinition::PrimaryAssetType)
	{
		FailRequest(
			ELastFPSTravelRequestResult::InvalidAssetId,
			FText::Format(
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuickPlayInvalidDefinitionId),
				FText::FromString(BattleDefinitionId.ToString())));
		return ELastFPSTravelRequestResult::InvalidAssetId;
	}
	if (!GetCommonSessionSubsystem())
	{
		FailRequest(
			ELastFPSTravelRequestResult::SessionUnavailable,
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuickPlayNoSessionSubsystem));
		return ELastFPSTravelRequestResult::SessionUnavailable;
	}

	PendingPlayerController = LocalPlayerController;
	PendingBattleDefinitionId = BattleDefinitionId;
	SetTravelState(ELastFPSTravelState::LoadingDefinition);

	ULastFPSLoadingIndicatorSubsystem::Show(
		this,
		LocalPlayerController,
		FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::BattleLoadingIndicator));

	TArray<FName> BundlesToLoad;
	BundlesToLoad.Add(LastFPSAssetBundles::Game);
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
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::BattleDefinitionNotFound),
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
	if (!MapId.IsValid() || MapId.PrimaryAssetType != LastFPSPrimaryAssetTypes::Map)
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
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::BattleDefinitionInvalid),
				FText::FromString(PendingBattleDefinitionId.ToString())));
		return;
	}

	FString MapPackageName;
	if (!ResolveMapPackageName(PendingBattleDefinition->MapId, MapPackageName))
	{
		FailRequest(
			ELastFPSTravelRequestResult::AssetNotFound,
			FText::Format(
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::BattleMapInvalid),
				FText::FromString(PendingBattleDefinition->MapId.ToString())));
		return;
	}

	UCommonSessionSubsystem* Sessions = GetCommonSessionSubsystem();
	APlayerController* PlayerController = PendingPlayerController.Get();
	if (!Sessions || !IsValid(PlayerController))
	{
		FailRequest(
			ELastFPSTravelRequestResult::SessionUnavailable,
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::BattleSessionUnavailable));
		return;
	}

	PendingHostRequest = Sessions->CreateOnlineHostSessionRequest();
	PendingSearchRequest = Sessions->CreateOnlineSearchSessionRequest();
	if (!PendingHostRequest || !PendingSearchRequest)
	{
		FailRequest(
			ELastFPSTravelRequestResult::SessionUnavailable,
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::BattleSessionRequestInvalid));
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
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuickPlayStateLost));
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
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuickPlayPlayerLost));
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
	SetPresentation(
		FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::BattleTravelStatus),
		LastFPSLevelTravelPresentation::GetBattleMapNameText(Definition));

	if (ULastFPSLoadingProcessSubsystem* Loading =
		GetGameInstance()->GetSubsystem<ULastFPSLoadingProcessSubsystem>())
	{
		Loading->BeginTravelLoading(ELastFPSTravelReadiness::LevelControllerAndPawn);
		// 배틀 준비 인디케이터의 책임을 전체 로딩 화면으로 넘긴다.
		ULastFPSLoadingIndicatorSubsystem::Hide(this);
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
				? FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::HostSessionFailed)
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
				? FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::JoinSessionFailed)
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
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::TravelFailure),
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

	ULastFPSLoadingIndicatorSubsystem::Hide(this);

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
