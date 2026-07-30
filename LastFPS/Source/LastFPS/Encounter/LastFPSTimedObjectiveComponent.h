#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Tables/LastFPSQuestData.h"
#include "Encounter/LastFPSObjectiveHudMode.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "LastFPSTimedObjectiveComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class ULastFPSQuestSubsystem;
class UShapeComponent;

/** 목표의 최종 결과. 미해결 상태와 구분하기 위해 None 을 둔다. */
UENUM(BlueprintType)
enum class ELastFPSObjectiveResult : uint8
{
	None		UMETA(DisplayName="진행중"),
	Succeeded	UMETA(DisplayName="성공"),
	Failed		UMETA(DisplayName="실패")
};

/** 목표가 성공·실패로 확정된 순간. 서버와 클라이언트 양쪽에서 1회씩 발생한다. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnLastFPSObjectiveResolved,
	UActorComponent*, Objective,
	ELastFPSObjectiveResult, Result);

/** 진행률(0~1) 변경 — HUD 게이지·화면 마커용. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLastFPSObjectiveProgress, float, Progress01);

/**
 * 정의 에셋이 런타임 목표에 넘기는 불변 설정 묶음이다.
 * 클라이언트도 같은 값으로 진행률과 라벨을 계산해야 하므로 통째로 복제한다.
 */
USTRUCT()
struct FLastFPSTimedObjectiveSettings
{
	GENERATED_BODY()

	UPROPERTY()
	float Duration = 60.f;

	UPROPERTY()
	float UpdateInterval = 0.25f;

	UPROPERTY()
	ELastFPSObjectiveType QuestObjectiveType = ELastFPSObjectiveType::DefendZone;

	UPROPERTY()
	FGameplayTag ZoneTag;

	UPROPERTY()
	FText DisplayLabel;

	/**
	 * 어떤 HUD 표시를 켤지. 컴포넌트는 이 값을 해석하지 않고 라우터에 넘기기만 한다.
	 * None 이면 화면 표시 없이 판정만 한다.
	 */
	UPROPERTY()
	ELastFPSObjectiveHudMode HudMode = ELastFPSObjectiveHudMode::None;
};

/**
 * 시간으로 진행되는 인카운터 목표의 진행·복제·판정을 소유하는 컴포넌트다.
 *
 * 목표의 "종류"는 아래 두 참조의 유무로만 결정되며, 유형을 나타내는 플래그나 분기를 두지 않는다.
 *   - RequiredVolume    : 지정하면 볼륨 안에 플레이어가 있을 때만 진행한다. 비우면 항상 진행.
 *   - FailureWatchTarget: 지정하면 그 대상이 파괴될 때 실패한다. 비우면 실패 개념이 없다.
 * 따라서 (볼륨만) = 점령, (감시 대상만) = 방어, (둘 다) = 구역 안에서 대상 지키기가 된다.
 *
 * 서버 권한으로 시간을 누적하고 결과를 확정하며, 진행·결과를 복제해 클라이언트가 같은 시점에
 * 게이지와 퀘스트 진행을 갱신하도록 한다(퀘스트 진행은 각 머신의 로컬 서브시스템이 소유).
 *
 * 수명은 인카운터 런타임이 소유한다 — 스스로 시작하지 않는다.
 */
UCLASS(ClassGroup=(LastFPS), meta=(BlueprintSpawnableComponent))
class LASTFPS_API ULastFPSTimedObjectiveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULastFPSTimedObjectiveComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ── 구성 (정의 에셋이 생성 시 1회 호출) ──────────────────────────

	/** 불변 설정을 적용한다. 시작 이후에는 호출하지 않는다. */
	void ApplySettings(const FLastFPSTimedObjectiveSettings& InSettings);

	/** 진행 조건이 될 볼륨. null 이면 조건 없이 항상 진행한다. */
	void SetRequiredVolume(UShapeComponent* InVolume);

	/** 실패 감시 대상. null 이면 실패하지 않는다. */
	void SetFailureWatchTarget(AActor* InTarget);

	/** 감시 대상의 체력을 초기화/복구할 효과(선택). 지정하면 최대 체력 값보다 우선한다. */
	void SetTargetInitEffect(TSubclassOf<UGameplayEffect> InEffect);

	/** 초기화 효과가 없을 때 직접 세팅할 최대 체력. */
	void SetTargetMaxHealth(float InMaxHealth);

	/** 볼륨 안에 적이 있으면 진행을 멈출지. 진행 조건을 강화할 뿐이다. */
	void SetBlockedByEnemies(bool bInBlocked);

	// ── 수명 (인카운터 런타임이 호출) ────────────────────────────────

	/** 진행을 초기화하고 판정을 시작한다. 서버 권한. */
	void StartObjective();

	/** 판정을 중단한다. 결과를 통지하지 않는다(인카운터 종료·언로드 공용). */
	void StopObjective();

#if !UE_BUILD_SHIPPING
	/**
	 * 개발용 즉시 성공 — 치트로 인카운터를 건너뛸 때 목표까지 함께 해결한다.
	 * 이게 없으면 방이 클리어돼도 퀘스트 목표가 미해결로 남아 치트의 목적이 깨진다.
	 */
	void DebugForceSucceed();
#endif

	// ── 조회 ────────────────────────────────────────────────────────

	/** 성공으로 확정됐는가. 미해결이면 인카운터가 웨이브를 순환하며 클리어를 보류한다. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Objective")
	bool IsSucceeded() const { return Result == ELastFPSObjectiveResult::Succeeded; }

	UFUNCTION(BlueprintPure, Category="LastFPS|Objective")
	bool IsResolved() const { return Result != ELastFPSObjectiveResult::None; }

	/** 0~1 진행률. HUD·마커·인카운터 진행 이벤트가 공유하는 단일 값이다. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Objective")
	float GetProgress01() const;

	/** 화면 표시용 라벨. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Objective")
	const FText& GetDisplayLabel() const { return Settings.DisplayLabel; }

	/** 목표 전체 시간(초) — 방어 HUD 가 남은 시간을 계산하는 데 쓴다. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Objective")
	float GetDuration() const { return Settings.Duration; }

	UFUNCTION(BlueprintPure, Category="LastFPS|Objective")
	ELastFPSObjectiveHudMode GetHudMode() const { return Settings.HudMode; }

	/** 지킬 대상의 0~1 체력. 대상이 없거나 GAS 가 없으면 음수. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Objective")
	float GetWatchTargetHealth01() const;

	/** 감시 대상(지킬 장치 등). 없으면 null. HUD 가 체력 게이지를 붙일 때 쓴다. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Objective")
	AActor* GetFailureWatchTarget() const { return FailureWatchTarget; }

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Objective")
	FOnLastFPSObjectiveResolved OnObjectiveResolved;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Objective")
	FOnLastFPSObjectiveProgress OnProgressChanged;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnRep_ElapsedSeconds();

	UFUNCTION()
	void OnRep_Result();

	/** 설정이 복제로 도착한 시점 — 클라이언트는 여기서 처음 HUD 모드를 알게 된다. */
	UFUNCTION()
	void OnRep_Settings();

	/** 활성 여부가 복제된 시점 — 클라이언트가 자기 화면 표시를 켜고 끈다. */
	UFUNCTION()
	void OnRep_Active();

	/** 현재 상태에 맞춰 HUD 슬롯을 점유하거나 반납한다. 서버·클라 공통 진입점이다. */
	void RefreshHudPresentation();

	/** 타이머 콜백(서버) — 실패 검사 후 조건이 맞으면 진행을 누적한다. */
	void TickObjective();

	/** 이번 주기에 진행이 누적되어야 하는가. 조건이 없으면 항상 참이다. */
	bool ShouldAdvance() const;

	/** 결과를 확정하고 복제한다. 서버 전용. 이미 확정됐으면 무시한다. */
	void Resolve(ELastFPSObjectiveResult NewResult);

	/** 결과 확정 시 서버·클라 공통 처리 — 퀘스트 통지 + 델리게이트. */
	void HandleResolved();

	/** 감시 대상의 체력 0 도달을 구독한다. GAS 가 없으면 파괴 이벤트만 감시한다. */
	void BindFailureWatch();
	void UnbindFailureWatch();

	/** 감시 대상 체력이 바뀔 때(서버) — 0 이면 실패 확정. */
	void HandleWatchTargetHealthChanged(const struct FOnAttributeChangeData& Data);

	UFUNCTION()
	void HandleWatchTargetDestroyed(AActor* DestroyedActor);

	/** 감시 대상의 체력을 초기화 효과로 되돌린다(시작·재시작 공용). */
	void ResetWatchTarget();

	bool HasAuthority() const;
	UAbilitySystemComponent* GetWatchTargetAbilitySystem() const;
	ULastFPSQuestSubsystem* GetQuestSubsystem() const;

	/**
	 * HUD 표시 슬롯을 점유/반납한다.
	 * 서버·클라 양쪽에서 각자 로컬 화면을 켜야 하므로 권한을 따지지 않고,
	 * 설정이 복제로 도착한 시점(OnRep)에도 다시 시도한다.
	 */
	void RequestHudPresentation();
	void ReleaseHudPresentation();
	class ULastFPSObjectiveHudSubsystem* GetHudSubsystem() const;

	/** HUD 슬롯을 점유 중인가 — 중복 요청·반납 누락을 막는 로컬 래치다. */
	bool bHudPresentationActive = false;

	/** 불변 설정 — 정의 에셋이 채운다. 런타임 상태와 섞지 않는다. */
	UPROPERTY(ReplicatedUsing=OnRep_Settings)
	FLastFPSTimedObjectiveSettings Settings;

	/**
	 * 지정 시 이 볼륨 안에 플레이어가 있을 때만 진행한다.
	 * 판정은 서버 전용이라 복제하지 않는다 — 컴포넌트 참조는 클라이언트에서 해석되지 않는 경우가 많다.
	 */
	UPROPERTY()
	TObjectPtr<UShapeComponent> RequiredVolume;

	/** 지정 시 이 대상이 파괴되면 실패한다. */
	UPROPERTY(Replicated)
	TObjectPtr<AActor> FailureWatchTarget;

	/**
	 * 감시 대상이 지정되었는가.
	 * GC 가 FailureWatchTarget 을 null 로 만들면 "원래 없었다"와 "사라졌다"를 구분할 수 없으므로
	 * 지정 사실 자체를 따로 기억한다.
	 */
	UPROPERTY(Replicated)
	bool bWatchTargetAssigned = false;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> TargetInitEffect;

	/** 초기화 효과가 없을 때 쓰는 최대 체력. 0 이하면 초기화하지 않는다. */
	UPROPERTY()
	float TargetMaxHealth = 0.f;

	UPROPERTY(ReplicatedUsing=OnRep_ElapsedSeconds)
	float ElapsedSeconds = 0.f;

	UPROPERTY(ReplicatedUsing=OnRep_Result)
	ELastFPSObjectiveResult Result = ELastFPSObjectiveResult::None;

	/** 진행 조건 강화 플래그. RequiredVolume 과 함께 서버에서만 쓰인다. */
	UPROPERTY()
	bool bBlockedByEnemies = false;

	FTimerHandle ObjectiveTimerHandle;
	FDelegateHandle WatchTargetHealthHandle;

	/** 판정이 돌고 있는가. 클라이언트가 자기 화면 표시를 켜려면 알아야 하므로 복제한다. */
	UPROPERTY(ReplicatedUsing=OnRep_Active)
	bool bActive = false;

	/** 결과 통지 1회 래치 — 서버·클라가 각자 소유한다(복제하지 않는다). */
	bool bResolvedBroadcast = false;
};
