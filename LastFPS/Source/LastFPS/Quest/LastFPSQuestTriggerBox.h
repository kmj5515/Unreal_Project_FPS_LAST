#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "GameplayTagContainer.h"
#include "LastFPSQuestTriggerBox.generated.h"

/**
 * 퀘스트 트리거 구역 액터이다.
 * 레벨 내 BP_Quest_Trigger_Depot 의 C++ 부모 클래스로 활용 가능하며, 
 * 플레이어 진입 시 퀘스트 진입/목표 완료 판정 통지 및 지정된 무전 자막 팝업을 트리거한다.
 */
UCLASS()
class LASTFPS_API ALastFPSQuestTriggerBox : public ATriggerBox
{
	GENERATED_BODY()

public:
	ALastFPSQuestTriggerBox();

	/**
	 * 스폰 직후 코드에서 위치 태그와 무전 대사를 주입한다.
	 * 에디터에 직접 배치할 때는 디테일 패널로 설정하므로 호출할 필요가 없다.
	 */
	void ConfigureQuestTrigger(const FGameplayTag& InLocationTag, const TArray<FName>& InRadioIds);

protected:
	virtual void BeginPlay() override;

	/** 플레이어가 트리거 박스 영역에 들어왔을 때 */
	UFUNCTION()
	virtual void OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor);

	/** 퀘스트 목표 위치 판정용 플레이어 태그 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Trigger")
	FGameplayTag LocationTag;

	/** 진입 시 트리거할 무전 대사 행 참조 (DT_RadioTransmission) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Radio")
	TArray<FName> RadioTransmissionIds;

	/** 한 번만 작동할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Trigger")
	bool bTriggerOnce = true;

private:
	bool bAlreadyTriggered = false;
};
