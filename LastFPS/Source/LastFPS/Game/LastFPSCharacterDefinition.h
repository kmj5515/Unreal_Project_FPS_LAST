#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LastFPSCharacterDefinition.generated.h"

class APawn;
class UTexture2D;

/**
 * 계승자(플레이어 캐릭터) 1종의 정의 — DataAsset.
 *
 * 캐릭터 선택/관리 화면이 표시할 메타데이터(이름·역할·아이콘·설명)와
 * 스폰할 PawnClass를 한곳에 모은다. 위젯에 직접 적던 이름/역할 배열을 대체한다.
 *
 * PawnClass는 인게임 팀 폰 준비 후 채운다 — 현재 위젯 표시(이름/역할)에는 불필요.
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSCharacterDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 캐릭터 선택창에 표시할 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	FText DisplayName;

	/** 역할/클래스 한 줄 설명 (예: "돌격", "지원") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	FText Role;

	/** 선택창/관리 화면 아이콘 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	TObjectPtr<UTexture2D> Icon;

	/** 상세 설명 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character", meta=(MultiLine=true))
	FText Description;

	/** 스폰할 Pawn 클래스 (인게임 팀 폰 준비 후 지정 — 현재 스폰 경로 미연결) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	TSubclassOf<APawn> PawnClass;
};
