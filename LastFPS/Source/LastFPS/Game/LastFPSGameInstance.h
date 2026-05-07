#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LastFPSGameInstance.generated.h"

UCLASS()
class LASTFPS_API ULastFPSGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    void SaveSelectedCharacterIndex(const FString& PlayerKey, int32 SelectedIndex);
    bool TryGetSelectedCharacterIndex(const FString& PlayerKey, int32& OutSelectedIndex) const;

private:
    UPROPERTY()
    TMap<FString, int32> SelectedCharacterIndexByPlayerKey;
};
