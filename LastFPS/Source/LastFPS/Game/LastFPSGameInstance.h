#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Utility/LastFPSTravelTypes.h"
#include "LastFPSGameInstance.generated.h"

class ULastFPSCharacterRoster;
class UDataTable;
class UTexture2D;

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

    /** 선택 가능한 캐릭터 로스터(단일 소스). 미지정/로드 실패 시 nullptr. 첫 호출에서 lazy 로드 후 캐시. */
    const ULastFPSCharacterRoster* GetCharacterRoster();

    /** 로딩 팁 테이블(단일 소스). 프리로드와 표시가 같은 DT를 쓰도록 위젯이 이걸 우선 참조. */
    UDataTable* GetLoadingTipTable();

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
    ELastFPSTravelDestination GetPendingTravelDestination() const;

    UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
    FText GetPendingTravelStatusText() const;

    UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
    FText GetPendingTravelMapNameText() const;

    UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
    static FText GetDefaultStatusTextForDestination(ELastFPSTravelDestination Destination);

    UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
    static FText GetDefaultMapNameTextForDestination(ELastFPSTravelDestination Destination);

protected:
    /** 캐릭터 로스터 에셋 경로 — DefaultGame.ini의 [/Script/LastFPS.LastFPSGameInstance] 에서 지정 */
    UPROPERTY(Config, EditAnywhere, Category="LastFPS|Character")
    TSoftObjectPtr<ULastFPSCharacterRoster> CharacterRosterAsset;

    /** 로딩 팁 테이블 (FLastFPSLoadingTipData) — 시작 시 이미지 프리로드용 */
    UPROPERTY(Config, EditAnywhere, Category="LastFPS|Loading")
    TSoftObjectPtr<UDataTable> LoadingTipTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/Loading/DT_LoadingTips.DT_LoadingTips")));

private:
    /** 시작 시 로딩 팁 이미지들을 로드·상주시켜 캐싱 (첫 로딩 화면부터 즉시 표시) */
    void PreloadLoadingTipTextures();

    /** 프리로드한 텍스처의 하드 레퍼런스 — GC 방지 + 상주 유지 */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UTexture2D>> PreloadedLoadingTipTextures;

    /** GetCharacterRoster의 lazy 로드 캐시 */
    UPROPERTY(Transient)
    TObjectPtr<ULastFPSCharacterRoster> CachedCharacterRoster;

    UPROPERTY()
    TMap<FString, int32> SelectedCharacterIndexByPlayerKey;

};
