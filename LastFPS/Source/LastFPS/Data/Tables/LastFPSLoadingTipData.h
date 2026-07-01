#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LastFPSLoadingTipData.generated.h"

class UTexture2D;

/** 로딩 팁 분류 (선택적 필터/아이콘용) */
UENUM(BlueprintType)
enum class ELastFPSLoadingTipCategory : uint8
{
	Control		UMETA(DisplayName="조작"),
	Combat		UMETA(DisplayName="전투"),
	Growth		UMETA(DisplayName="성장"),
	World		UMETA(DisplayName="월드"),
	Lore		UMETA(DisplayName="로어")
};

/**
 * 로딩 화면 팁 1건 — DataTable 행 (로딩 팁의 단일 소스).
 * 제목 + 본문 + (선택)이미지 + 분류. 로딩창이 무작위로 하나 뽑아 표시한다.
 * 이미지는 TSoftObjectPtr라 선택된 1장만 로드된다.
 */
USTRUCT(BlueprintType)
struct FLastFPSLoadingTipData : public FTableRowBase
{
	GENERATED_BODY()

	/** 팁 제목 (예: "조작", "TIP") — 비워도 됨 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LoadingTip")
	FText TipTitle;

	/** 팁 본문 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LoadingTip", meta=(MultiLine=true))
	FText Tip;

	/** 배경/일러스트 이미지 (선택 — 지정 시 선택된 것만 로드) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LoadingTip")
	TSoftObjectPtr<UTexture2D> Image;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LoadingTip")
	ELastFPSLoadingTipCategory Category = ELastFPSLoadingTipCategory::Control;
};

/**
 * 로딩 팁 조회 헬퍼. 로딩 위젯이 C++에서 호출해 무작위 팁을 가져온다.
 * 직전에 뽑은 팁은 연속 제외한다(항목이 2개 이상일 때).
 */
UCLASS()
class LASTFPS_API ULastFPSLoadingTipLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 무작위 팁 1개(행 전체)를 OutTip에 채운다. 비었으면 false. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
	static bool GetRandomLoadingTip(const UDataTable* TipTable, FLastFPSLoadingTipData& OutTip);

	/** 무작위 팁 본문만 반환(편의). 비었으면 빈 텍스트. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
	static FText GetRandomLoadingTipText(const UDataTable* TipTable);
};
