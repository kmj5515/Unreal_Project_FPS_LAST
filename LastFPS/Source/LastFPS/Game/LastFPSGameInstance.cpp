#include "Game/LastFPSGameInstance.h"

#include "UI/LastFPSUIManagerSubsystem.h"

#include "CommonLocalPlayer.h"
#include "Engine/World.h"

void ULastFPSGameInstance::Init()
{
    Super::Init();
}

void ULastFPSGameInstance::Shutdown()
{
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

void ULastFPSGameInstance::SaveSelectedCharacterIndex(const FString& PlayerKey, int32 SelectedIndex)
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

void ULastFPSGameInstance::RequestTravelToMatch(const FString& MatchMapURL)
{
    if (MatchMapURL.IsEmpty()) return;
    if (UWorld* World = GetWorld())
    {
        World->ServerTravel(MatchMapURL);
    }
}

void ULastFPSGameInstance::RequestTravelToLobby(const FString& LobbyMapURL)
{
    if (LobbyMapURL.IsEmpty()) return;
    if (UWorld* World = GetWorld())
    {
        World->ServerTravel(LobbyMapURL);
    }
}
