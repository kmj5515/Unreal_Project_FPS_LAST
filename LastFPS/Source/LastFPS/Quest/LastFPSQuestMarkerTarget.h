#pragma once

#include "UObject/Interface.h"
#include "LastFPSQuestMarkerTarget.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class ULastFPSQuestMarkerTarget : public UInterface
{
	GENERATED_BODY()
};

/**
 * 퀘스트 마커가 붙을 수 있는 대상이 자기 식별자를 제공하는 계약.
 * 퀘스트 모듈이 NPC 같은 구체 액터 클래스를 알지 않고 키만 질의하도록 두기 위한 인터페이스다.
 * 반환 값은 퀘스트 목표의 대상 행 이름(TalkToNPC 의 TargetId 등)과 같은 이름 공간을 쓴다.
 */
class LASTFPS_API ILastFPSQuestMarkerTarget
{
	GENERATED_BODY()

public:
	/** 이 액터를 가리키는 퀘스트 대상 행 이름. 없으면 None 을 반환한다. */
	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|Quest")
	FName GetQuestMarkerId() const;
};
