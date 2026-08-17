#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/PrimaryAssetId.h"
#include "Utility/LastFPSTravelTypes.h"
#include "LastFPSLevelTravelSubsystem.generated.h"

class APlayerController;
class UCommonSession_HostSessionRequest;
class UCommonSession_SearchResult;
class UCommonSession_SearchSessionRequest;
class UCommonSessionSubsystem;
class ULastFPSBattleDefinition;
class UTexture2D;
struct FOnlineResultInformation;
struct FStreamableHandle;
namespace ETravelFailure { enum Type : int; }

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnLastFPSTravelPresentationChanged,
	const FText&,
	const FText&);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FLastFPSTravelStateChanged,
	ELastFPSTravelState, PreviousState,
	ELastFPSTravelState, NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FLastFPSTravelRequestFailed,
	ELastFPSTravelRequestResult, Result,
	const FText&, ErrorMessage);

/**
 * 로컬 맵 이동과 전투 세션 진입을 조정하는 단일 전역 진입점.
 * 맵 경로 확인은 Asset Manager에, 온라인 동작은 CommonSessionSubsystem에 위임한다.
 */
UCLASS()
class LASTFPS_API ULastFPSLevelTravelSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="LastFPS|Travel")
	ELastFPSTravelRequestResult RequestTravel(
		APlayerController* LocalPlayerController,
		const FLastFPSTravelEntryRequest& Request);

	UFUNCTION(BlueprintCallable, Category="LastFPS|Travel")
	ELastFPSTravelRequestResult TravelToDestination(ELastFPSTravelDestination Destination);

	UFUNCTION(BlueprintCallable, Category="LastFPS|Travel")
	ELastFPSTravelRequestResult TravelToMap(
		FPrimaryAssetId MapId,
		ELastFPSTravelReadiness Readiness,
		FText StatusText,
		FText MapNameText);

	UFUNCTION(BlueprintCallable, Category="LastFPS|Travel")
	ELastFPSTravelRequestResult QuickPlayBattle(
		APlayerController* LocalPlayerController,
		FPrimaryAssetId BattleDefinitionId);

	/**
	 * 고른 전투를 예약하고 마스터 로비로 보낸다. 방 개설과 등록은 로비가 담당한다.
	 * 실제 전투 맵 이동은 대기실의 GameMode가 BattleDef를 해석해 수행하므로
	 * 여기서는 전투 맵을 알 필요가 없다.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Travel")
	ELastFPSTravelRequestResult TravelToPartyRoom(
		APlayerController* LocalPlayerController,
		FPrimaryAssetId BattleDefinitionId);

	UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
	bool ResolveMapPackageName(FPrimaryAssetId MapId, FString& OutPackageName) const;

	UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
	bool IsBusy() const { return TravelState != ELastFPSTravelState::Idle; }

	/** 설정된 퀘스트 조건을 만족해 이 이동 요청을 실행할 수 있는지 조회한다. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
	bool IsTravelRequestUnlocked(const FLastFPSTravelEntryRequest& Request) const;

	/** 잠금 상태를 표시할 목적지별 아이콘을 반환한다. 조건이 없으면 빈 참조다. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
	TSoftObjectPtr<UTexture2D> GetTravelLockedIcon(const FLastFPSTravelEntryRequest& Request) const;

	UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
	ELastFPSTravelState GetTravelState() const { return TravelState; }

	UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
	FText GetPendingTravelStatusText() const { return PendingStatusText; }

	UFUNCTION(BlueprintPure, Category="LastFPS|Travel")
	FText GetPendingTravelMapNameText() const { return PendingMapNameText; }

	ELastFPSTravelDestination GetPendingLocalDestination() const
	{
		return PendingLocalDestination;
	}

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Travel")
	FLastFPSTravelStateChanged OnTravelStateChanged;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Travel")
	FLastFPSTravelRequestFailed OnTravelRequestFailed;

	FOnLastFPSTravelPresentationChanged OnTravelPresentationChanged;

private:
	void HandleBattleDefinitionLoaded();
	void HandleQuickPlaySearchFinished(bool bSucceeded, const FText& ErrorMessage);
	void HandleCreateSessionComplete(const FOnlineResultInformation& Result);
	void HandleJoinSessionComplete(const FOnlineResultInformation& Result);
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void HandleLoadingFinished(bool bSucceeded);
	void HandleTravelFailure(
		UWorld* FailedWorld,
		ETravelFailure::Type FailureType,
		const FString& Reason);

	void BeginSessionTravelLoading(const ULastFPSBattleDefinition& Definition);
	void SetTravelState(ELastFPSTravelState NewState);
	void SetPresentation(const FText& StatusText, const FText& MapNameText);
	void ClearPresentation();
	void FailRequest(ELastFPSTravelRequestResult Result, const FText& ErrorMessage);
	void ResetPendingBattleRequest();

	UCommonSessionSubsystem* GetCommonSessionSubsystem() const;
	UCommonSession_SearchResult* FindBestMatchingSession(
		const ULastFPSBattleDefinition& Definition,
		const UCommonSession_SearchSessionRequest& SearchRequest) const;

	UPROPERTY(Transient)
	TObjectPtr<ULastFPSBattleDefinition> PendingBattleDefinition;

	UPROPERTY(Transient)
	TObjectPtr<UCommonSession_HostSessionRequest> PendingHostRequest;

	UPROPERTY(Transient)
	TObjectPtr<UCommonSession_SearchSessionRequest> PendingSearchRequest;

	TSharedPtr<FStreamableHandle> PendingDefinitionLoadHandle;
	TWeakObjectPtr<APlayerController> PendingPlayerController;
	FPrimaryAssetId PendingBattleDefinitionId;
	FText PendingStatusText;
	FText PendingMapNameText;
	ELastFPSTravelState TravelState = ELastFPSTravelState::Idle;
	ELastFPSTravelDestination PendingLocalDestination =
		ELastFPSTravelDestination::MainMenu;
	FDelegateHandle PostLoadMapDelegateHandle;
	FDelegateHandle LoadingFinishedDelegateHandle;
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle TravelFailureDelegateHandle;
};
