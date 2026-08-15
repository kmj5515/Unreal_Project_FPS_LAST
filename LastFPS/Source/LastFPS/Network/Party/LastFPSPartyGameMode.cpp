#include "LastFPSPartyGameMode.h"
#include "LastFPSPartyGameState.h"
#include "LastFPSPartyPlayerState.h"
#include "LastFPSPartyPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/AssetManager.h"
#include "Data/Definitions/LastFPSBattleDefinition.h"

ALastFPSPartyGameMode::ALastFPSPartyGameMode()
{
	GameStateClass = ALastFPSPartyGameState::StaticClass();
	PlayerStateClass = ALastFPSPartyPlayerState::StaticClass();
	PlayerControllerClass = ALastFPSPartyPlayerController::StaticClass();
	DefaultPawnClass = nullptr; // UI only lobby
}

void ALastFPSPartyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	FString BattleDefStr = UGameplayStatics::ParseOption(Options, TEXT("BattleDef"));
	if (!BattleDefStr.IsEmpty())
	{
		PendingBattleDefinition = FPrimaryAssetId(BattleDefStr);
		UE_LOG(LogTemp, Log, TEXT("[PartyGameMode] Initialized with PendingBattleDefinition: %s"), *BattleDefStr);
	}
}

void ALastFPSPartyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (ALastFPSPartyPlayerState* PS = NewPlayer->GetPlayerState<ALastFPSPartyPlayerState>())
	{
		UE_LOG(LogTemp, Log, TEXT("[PartyGameMode] Player Connected: %s"), *PS->GetPlayerName());
	}

	// If this is the first player or no host exists, assign host
	AssignNewHost();
}

void ALastFPSPartyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// Reassign host if the exiting player was the host
	if (ALastFPSPartyPlayerState* PS = Cast<ALastFPSPartyPlayerState>(Exiting->PlayerState))
	{
		if (PS->IsPartyHost())
		{
			PS->SetPartyHost(false);
			AssignNewHost();
		}
	}
}

void ALastFPSPartyGameMode::AssignNewHost()
{
	if (GameState)
	{
		bool bHasHost = false;
		ALastFPSPartyPlayerState* FirstValidPS = nullptr;

		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (ALastFPSPartyPlayerState* PartyPS = Cast<ALastFPSPartyPlayerState>(PS))
			{
				if (PartyPS->IsPartyHost())
				{
					bHasHost = true;
					break;
				}
				if (!FirstValidPS)
				{
					FirstValidPS = PartyPS;
				}
			}
		}

		if (!bHasHost && FirstValidPS)
		{
			FirstValidPS->SetPartyHost(true);
		}
	}
}

void ALastFPSPartyGameMode::RequestStartGame(APlayerController* RequestingPlayer)
{
	if (ALastFPSPartyPlayerState* PS = RequestingPlayer->GetPlayerState<ALastFPSPartyPlayerState>())
	{
		if (PS->IsPartyHost())
		{
			if (AreAllPlayersReady())
			{
				const FString TravelUrl = BuildTravelUrl();
				if (TravelUrl.IsEmpty())
				{
					UE_LOG(LogTemp, Error,
						TEXT("ALastFPSPartyGameMode::RequestStartGame - 이동할 맵을 결정하지 못했습니다. ")
						TEXT("DefaultGame.ini의 [/Script/LastFPS.LastFPSPartyGameMode] DefaultBattleMap을 확인할 것."));
					return;
				}

				UE_LOG(LogTemp, Log, TEXT("ALastFPSPartyGameMode::RequestStartGame - 게임 시작. Url=%s"), *TravelUrl);

				// 대기실은 리슨 서버이므로 ServerTravel로 접속한 파티원 전원을 함께 이동시킨다.
				GetWorld()->ServerTravel(TravelUrl);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ALastFPSPartyGameMode::RequestStartGame - Not all players are ready."));
			}
		}
	}
}

FString ALastFPSPartyGameMode::BuildTravelUrl() const
{
	// 우선순위: 대기실 진입 시 넘어온 BattleDef > 설정된 기본 맵.
	if (PendingBattleDefinition.IsValid())
	{
		UAssetManager& AssetManager = UAssetManager::Get();
		const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(PendingBattleDefinition);
		if (const ULastFPSBattleDefinition* BattleDef = Cast<ULastFPSBattleDefinition>(AssetPath.TryLoad()))
		{
			if (BattleDef->MapId.IsValid())
			{
				const FSoftObjectPath MapPath = AssetManager.GetPrimaryAssetPath(BattleDef->MapId);
				const FString PackageName = MapPath.GetLongPackageName();
				if (!PackageName.IsEmpty())
				{
					return PackageName + TEXT("?listen");
				}
			}
		}

		UE_LOG(LogTemp, Warning,
			TEXT("ALastFPSPartyGameMode - BattleDef에서 맵을 해석하지 못해 DefaultBattleMap으로 대체합니다. BattleDef=%s"),
			*PendingBattleDefinition.ToString());
	}

	if (DefaultBattleMap.IsNull())
	{
		return FString();
	}

	const FString DefaultPackageName = DefaultBattleMap.GetLongPackageName();
	if (DefaultPackageName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("ALastFPSPartyGameMode - DefaultBattleMap 경로가 올바르지 않습니다. Value=%s"),
			*DefaultBattleMap.ToString());
		return FString();
	}

	return DefaultPackageName + TEXT("?listen");
}

bool ALastFPSPartyGameMode::AreAllPlayersReady() const
{
	if (!GameState) return false;

	int32 PartyMemberCount = 0;

	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (ALastFPSPartyPlayerState* PartyPS = Cast<ALastFPSPartyPlayerState>(PS))
		{
			++PartyMemberCount;

			// 방장은 시작 버튼을 누르는 주체이므로 준비 검사 대상이 아니다.
			if (!PartyPS->IsPartyReady() && !PartyPS->IsPartyHost())
			{
				return false;
			}
		}
	}

	if (PartyMemberCount < MinPlayersToStart)
	{
		UE_LOG(LogTemp, Warning, TEXT("ALastFPSPartyGameMode - 시작 최소 인원 미달. Current=%d, Min=%d"),
			PartyMemberCount, MinPlayersToStart);
		return false;
	}

	return true;
}
