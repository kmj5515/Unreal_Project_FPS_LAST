#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LastFPSPythonEditorLibrary.generated.h"

class UMovieSceneSequence;

/**
 * 파이썬(MCP) 스크립트에서 시퀀스 마커를 자동화하기 위한 에디터 전용 유틸리티 라이브러리입니다.
 */
UCLASS()
class LASTFPS_API ULastFPSPythonEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LastFPS|Editor")
	static bool AddSequenceMark(UMovieSceneSequence* Sequence, int32 FrameNumber, const FString& Label);
};
