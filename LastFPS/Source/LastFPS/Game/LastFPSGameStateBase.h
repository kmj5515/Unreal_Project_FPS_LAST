#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "LastFPSGameStateBase.generated.h"

class ULastFPSDestinationContentComponent;
class ULastFPSDestinationContentSet;

DECLARE_MULTICAST_DELEGATE_OneParam(
    FOnLastFPSDestinationContextChanged,
    const FGameplayTagContainer&);

/** 파티 공용 진행이 갱신됐다 — 서버 기록 직후와 클라 복제 수신 직후 모두 발사된다. */
DECLARE_MULTICAST_DELEGATE(FOnLastFPSSharedQuestProgressChanged);

/**
 * 파티 공용 퀘스트 1건의 목표별 진행.
 * 인덱스는 DT_QuestData 행의 Objectives 순서와 1:1 이며, 정적 정의는 각 머신이 테이블에서 읽으므로
 * 여기에는 진행 숫자만 담는다.
 */
USTRUCT()
struct FLastFPSSharedQuestProgress
{
    GENERATED_BODY()

    UPROPERTY()
    FName QuestId;

    UPROPERTY()
    TArray<int32> Progress;

    /**
     * 파티 진행 병합 규칙 — 이 한 줄이 파티 공용 진행의 전부라 별도 함수로 두고 테스트한다.
     * bAbsolute=false 는 파티원 기여를 누적하고, true 는 관측값으로 승격만 한다.
     * 어느 쪽이든 이전 값 아래로 내려가지 않고 RequiredCount 를 넘지 않는다.
     */
    static int32 MergeProgress(
        int32 PreviousProgress,
        int32 Value,
        bool bAbsolute,
        int32 RequiredCount)
    {
        // 인카운터 완료 통지가 MAX_int32 를 절대값으로 보내므로 누적은 64비트로 계산해 넘침을 피한다.
        const int64 Candidate = bAbsolute
            ? static_cast<int64>(Value)
            : static_cast<int64>(PreviousProgress) + Value;
        return static_cast<int32>(FMath::Clamp<int64>(
            FMath::Max<int64>(Candidate, PreviousProgress),
            0,
            FMath::Max(RequiredCount, 0)));
    }
};

/** 파티 공용 무전이 갱신됐다 — 서버 기록 직후와 클라 복제 수신 직후 모두 발사된다. */
DECLARE_MULTICAST_DELEGATE(FOnLastFPSSharedRadioChanged);

/**
 * 파티 공용으로 송출된 무전 1묶음.
 * 지나간 무전을 전부 쌓지 않고 최신 묶음만 남긴다 — 늦게 합류한 파티원에게 밀린 자막을 한꺼번에
 * 쏟아내지 않기 위한 것으로, 서브시스템이 HUD 생성 전 대기열에 쓰는 정책과 같다.
 */
USTRUCT()
struct FLastFPSSharedRadioBroadcast
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FName> RadioIds;

    /**
     * 송출 일련번호. 같은 무전이 다시 송출돼도 복제 갱신이 감지되도록 매번 올리며,
     * 수신 측은 이 값으로 같은 송출을 두 번 재생하지 않는다. 0 은 아직 송출 없음.
     */
    UPROPERTY()
    int32 Serial = 0;
};

/** 맵 UI 규칙이 갱신됐다 — 서버 기록 직후와 클라 복제 수신 직후 모두 발사된다. */
DECLARE_MULTICAST_DELEGATE(FOnLastFPSMapUIRulesChanged);

/**
 * 이 맵의 UI 진입 규칙.
 * 원본은 GameMode 가 소유하지만 GameMode 는 서버에만 존재해 클라이언트에서 GetAuthGameMode() 가
 * 항상 null 이다. 그대로 두면 퀘스트 HUD 와 초기/ESC 화면이 클라이언트에서 동작하지 않으므로,
 * 규칙 소유는 GameMode 에 두고 값만 GameState 로 복제해 모든 머신이 같은 값을 읽게 한다.
 */
USTRUCT()
struct FLastFPSMapUIRules
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag InitialScreenTag;

    UPROPERTY()
    FGameplayTag EscMenuScreenTag;

    UPROPERTY()
    bool bShowQuestTracker = false;

    UPROPERTY()
    bool bShowCombatHUD = false;

    /**
     * 서버가 아직 규칙을 기록하지 않은 상태와 "전부 끔"으로 기록한 상태를 구분한다.
     * 이 구분이 없으면 복제 도착 전 기본값을 실제 규칙으로 오인해 HUD 표시 정책을 잘못 고른다.
     */
    UPROPERTY()
    bool bInitialized = false;
};

/**
 * 목적지 콘텐츠 게이트의 부착 지점.
 * ULoadingScreenManager 가 GameState 의 컴포넌트를 순회하므로 게이트는 여기 있어야 한다.
 */
UCLASS()
class LASTFPS_API ALastFPSGameStateBase : public AGameStateBase
{
    GENERATED_BODY()

public:
    ALastFPSGameStateBase();

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void SetDestinationContextTags(
        const FGameplayTagContainer& NewContextTags);

    /** 서버가 선택한 목적지 Content Set을 모든 클라이언트의 로컬 로더에 전달한다. */
    void SetDestinationContentSet(
        const ULastFPSDestinationContentSet* NewContentSet);

    const FGameplayTagContainer& GetDestinationContextTags() const
    {
        return DestinationContextTags;
    }

    FOnLastFPSDestinationContextChanged OnDestinationContextChanged;

    /**
     * 서버: 파티 공용 목표 1건의 진행을 갱신한다.
     * bAbsolute=false 는 Value 만큼 누적한다(처치 수처럼 파티원 기여를 합산하는 목표).
     * bAbsolute=true 는 Value 로 승격만 한다(도달·인카운터처럼 한 명이 달성하면 확정되는 사실형 목표).
     * 어느 쪽이든 RequiredCount 를 넘지 않고 이전 값보다 낮아지지 않는다 — 진행 되돌림은 파티 전체의
     * 완료 판정을 뒤집으므로 허용하지 않는다. 실제로 값이 바뀌었을 때만 true.
     *
     * 권위 머신에서는 변경 통지를 발사하지 않는다. 호출부가 여러 목표를 연달아 기록한 뒤
     * OnSharedQuestProgressChanged 를 한 번만 돌리도록 남겨둔 것이다(원격 클라는 OnRep 이 담당).
     */
    bool Auth_ApplySharedQuestProgress(
        FName QuestId,
        int32 ObjectiveIndex,
        int32 ObjectiveNum,
        int32 Value,
        bool bAbsolute,
        int32 RequiredCount);

    /** 파티 공용 진행 조회. 아직 기록이 없으면 nullptr. */
    const TArray<int32>* FindSharedQuestProgress(FName QuestId) const;

    FOnLastFPSSharedQuestProgressChanged OnSharedQuestProgressChanged;

    /**
     * 서버: 파티 전원이 들을 무전 묶음을 기록한다.
     * 복제 상태로 남기므로 이후 합류한 파티원도 접속 시점에 최신 묶음을 받는다.
     */
    void Auth_BroadcastSharedRadio(const TArray<FName>& RadioIds);

    const TArray<FName>& GetSharedRadioIds() const { return SharedRadio.RadioIds; }

    /** 현재 송출 일련번호. 0 이면 이 맵에서 아직 파티 무전이 송출되지 않았다. */
    int32 GetSharedRadioSerial() const { return SharedRadio.Serial; }

    FOnLastFPSSharedRadioChanged OnSharedRadioChanged;

    /** 서버: 이 맵의 UI 규칙을 기록해 모든 클라이언트에 복제한다. */
    void Auth_SetMapUIRules(const FLastFPSMapUIRules& NewRules);

    /** 아직 서버 규칙이 도착하지 않았으면 false — 호출부가 표시 정책의 기본값을 고를 때 쓴다. */
    bool HasMapUIRules() const { return MapUIRules.bInitialized; }

    const FLastFPSMapUIRules& GetMapUIRules() const { return MapUIRules; }

    FOnLastFPSMapUIRulesChanged OnMapUIRulesChanged;

private:
    UFUNCTION()
    void OnRep_MapUIRules();

    UPROPERTY(ReplicatedUsing=OnRep_MapUIRules)
    FLastFPSMapUIRules MapUIRules;

    UFUNCTION()
    void OnRep_DestinationContextTags();

    UFUNCTION()
    void OnRep_SharedQuestProgress();

    UFUNCTION()
    void OnRep_SharedRadio();

    UFUNCTION()
    void OnRep_DestinationContentSetId();

    UPROPERTY(VisibleAnywhere, Category="LastFPS|Loading")
    TObjectPtr<ULastFPSDestinationContentComponent> DestinationContent;

    UPROPERTY(ReplicatedUsing=OnRep_DestinationContextTags)
    FGameplayTagContainer DestinationContextTags;

    UPROPERTY(ReplicatedUsing=OnRep_DestinationContentSetId)
    FPrimaryAssetId DestinationContentSetId;

    // ponytail: 동시에 걸리는 파티 퀘스트는 1~2건·목표 5개 안쪽이라 통 배열 복제로 충분하다.
    // 퀘스트 수가 늘어 대역폭이 문제되면 FFastArraySerializer 로 전환한다.
    UPROPERTY(ReplicatedUsing=OnRep_SharedQuestProgress)
    TArray<FLastFPSSharedQuestProgress> SharedQuestProgress;

    UPROPERTY(ReplicatedUsing=OnRep_SharedRadio)
    FLastFPSSharedRadioBroadcast SharedRadio;
};
