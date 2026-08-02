#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LastFPSPreviewStageSubsystem.generated.h"

class ALastFPSPreviewStageActor;

/**
 * 아웃게임 3D 프리뷰 무대의 수명을 레벨에 맞춰 관리한다.
 *
 * 무대를 화면이 소유하면 화면을 열 때마다 스폰·메시 로드 비용을 내고 닫을 때 버린다.
 * 레벨 시작에 한 번 만들어 두고 이후에는 켜고 끄기만 하면 화면을 여는 프레임에 남는 비용이 없다.
 *
 * 무대는 하나이고 화면들이 번갈아 빌려 쓴다. 화면은 "무대를 빌린다"는 계약만 소비하고,
 * 누가 언제 스폰하는지는 알지 않는다.
 */
UCLASS()
class LASTFPS_API ULastFPSPreviewStageSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static ULastFPSPreviewStageSubsystem* Get(const UObject* WorldContext);

	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** 레벨 수명 동안 유지되는 무대. 아직 준비되지 않았으면 이 시점에 만든다. */
	ALastFPSPreviewStageActor* GetStage();

	/**
	 * 무대를 쓰는 화면이 열리고 닫힐 때 호출한다. 무대를 파괴하지 않고 비용만 끊는다.
	 * 중첩을 견디도록 참조 수로 센다(껍데기와 상세 프리뷰가 동시에 요구할 수 있다).
	 */
	void AddStageUser();
	void RemoveStageUser();

private:
	void ApplyActiveState();

	UPROPERTY(Transient)
	TObjectPtr<ALastFPSPreviewStageActor> Stage;

	/** 무대를 요구하는 화면 수. 0 이 되면 무대를 끈다. */
	int32 StageUserCount = 0;
};
