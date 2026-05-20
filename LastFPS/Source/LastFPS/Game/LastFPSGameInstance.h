#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LastFPSGameInstance.generated.h"

class ULastFPSLoadingScreenWidget;

UENUM(BlueprintType)
enum class ELastFPSTravelDirection : uint8
{
    None,
    LobbyToMatch,
    MatchToLobby
};

UCLASS(Config=Game)
class LASTFPS_API ULastFPSGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void Init() override;
    virtual void Shutdown() override;

    void SaveSelectedCharacterIndex(const FString& PlayerKey, int32 SelectedIndex);
    bool TryGetSelectedCharacterIndex(const FString& PlayerKey, int32& OutSelectedIndex) const;

    /** 로비 → 매치 ServerTravel (로딩 UI 표시 후 이동) */
    UFUNCTION(BlueprintCallable, Category="LastFPS|Travel")
    void RequestTravelToMatch(const FString& MatchMapURL);

    /** 매치 → 로비 ServerTravel */
    UFUNCTION(BlueprintCallable, Category="LastFPS|Travel")
    void RequestTravelToLobby(const FString& LobbyMapURL);

    UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
    bool IsLoadingScreenActive() const { return bLoadingScreenActive; }

protected:
    /** WBP_Loading (Parent: LastFPSLoadingScreenWidget) — 에디터에서 할당하거나 DefaultGame.ini에 경로 지정 */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="LastFPS|Loading")
    TSubclassOf<ULastFPSLoadingScreenWidget> LoadingScreenWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category="LastFPS|Loading", meta=(ClampMin="0.0"))
    float MinLoadingScreenDisplaySeconds = 1.0f;

    UPROPERTY(Config, EditAnywhere, Category="LastFPS|Loading")
    int32 LoadingScreenZOrder = 10000;

    UPROPERTY(Config, EditAnywhere, Category="LastFPS|Loading", meta=(ClampMin="1"))
    int32 LoadingScreenHideMaxRetries = 50;

private:
    void HandlePreLoadMap(const FString& MapName);
    void HandlePostLoadMap(UWorld* LoadedWorld);
    void BeginServerTravel(const FString& MapURL, ELastFPSTravelDirection Direction);
    void ShowLoadingScreenWidget();
    void HideLoadingScreenWidget();
    void RefreshLoadingScreenContent();
    void ScheduleFinalizeLoadingScreen(float DelaySeconds);
    void TryFinalizeLoadingScreen();

    static FString ExtractShortMapName(const FString& MapURL);
    static FText BuildStatusText(ELastFPSTravelDirection Direction, bool bMapLoadingPhase);

    UPROPERTY()
    TMap<FString, int32> SelectedCharacterIndexByPlayerKey;

    UPROPERTY()
    TObjectPtr<ULastFPSLoadingScreenWidget> LoadingScreenWidget;

    FDelegateHandle PreLoadMapHandle;
    FDelegateHandle PostLoadMapHandle;
    FTimerHandle FinalizeLoadingTimerHandle;

    ELastFPSTravelDirection ActiveTravelDirection = ELastFPSTravelDirection::None;
    FString PendingMapDisplayName;
    bool bLoadingScreenActive = false;
    bool bMapLoadingPhase = false;
    double LoadingScreenShowStartSeconds = 0.0;
    int32 LoadingScreenHideRetryCount = 0;
};
