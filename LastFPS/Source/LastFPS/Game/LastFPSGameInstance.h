#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Utility/LastFPSTravelTypes.h"
#include "LastFPSGameInstance.generated.h"

UCLASS(Config=Game)
class LASTFPS_API ULastFPSGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void Init() override;
    virtual void Shutdown() override;

    virtual int32 AddLocalPlayer(ULocalPlayer* NewPlayer, FPlatformUserId UserId) override;
    virtual bool RemoveLocalPlayer(ULocalPlayer* ExistingPlayer) override;

    void SaveSelectedCharacterIndex(const FString& PlayerKey, int32 SelectedIndex);
    bool TryGetSelectedCharacterIndex(const FString& PlayerKey, int32& OutSelectedIndex) const;

    UFUNCTION(BlueprintCallable, Category="LastFPS|Travel")
    void RequestTravelToDestination(ELastFPSTravelDestination Destination);

    UFUNCTION(BlueprintCallable, Category="LastFPS|Travel")
    void RequestTravelToMainMenu();

    UFUNCTION(BlueprintCallable, Category="LastFPS|Travel")
    void RequestTravelToCharacterSelect();

    UFUNCTION(BlueprintCallable, Category="LastFPS|Travel")
    void RequestTravelToHub();

    UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
    bool ResolveMapURL(ELastFPSTravelDestination Destination, FString& OutMapURL) const;

    UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
    ELastFPSTravelDestination GetPendingTravelDestination() const { return PendingTravelDestination; }

    UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
    FText GetPendingTravelStatusText() const { return PendingTravelStatusText; }

    UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
    FText GetPendingTravelMapNameText() const { return PendingTravelMapNameText; }

    UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
    static FText GetDefaultStatusTextForDestination(ELastFPSTravelDestination Destination);

    UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
    static FText GetDefaultMapNameTextForDestination(ELastFPSTravelDestination Destination);

protected:
    void ExecuteServerTravel(const FString& MapURL, ELastFPSTravelDestination DestinationForUI);
    void SetPendingTravelPresentation(ELastFPSTravelDestination Destination, const FText& StatusText, const FText& MapNameText);
    void ClearPendingTravelPresentation();

    void HandlePostLoadMap(UWorld* LoadedWorld);

    UPROPERTY(Config, EditAnywhere, Category="LastFPS|Travel")
    FString MainMenuMap = TEXT("/Game/Maps/Test/MainMenuMap");

    UPROPERTY(Config, EditAnywhere, Category="LastFPS|Travel")
    FString CharacterSelectMap = TEXT("/Game/Maps/Test/CharacterSelectMap");

    UPROPERTY(Config, EditAnywhere, Category="LastFPS|Travel")
    FString HubMap = TEXT("/Game/Maps/Test/HubMap");

private:
    UPROPERTY()
    TMap<FString, int32> SelectedCharacterIndexByPlayerKey;

    ELastFPSTravelDestination PendingTravelDestination = ELastFPSTravelDestination::MainMenu;
    FText PendingTravelStatusText;
    FText PendingTravelMapNameText;

    FDelegateHandle PostLoadMapDelegateHandle;
};
