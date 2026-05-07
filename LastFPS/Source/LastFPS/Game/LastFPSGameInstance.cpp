#include "Game/LastFPSGameInstance.h"

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
