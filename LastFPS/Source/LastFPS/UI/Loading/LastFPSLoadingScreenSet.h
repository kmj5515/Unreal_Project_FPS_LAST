#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LastFPSLoadingScreenSet.generated.h"

class UTexture2D;

/**
 * 로딩 화면에 표시할 "이미지 + 팁 텍스트" 한 세트.
 *
 * 이미지는 TSoftObjectPtr로 둬서 로딩 화면이 뜰 때 선택된 1장만 메모리에 올린다.
 * (배열 전체를 하드 레퍼런스로 들면 로딩 화면을 띄우려고 모든 텍스처가 상주하는 본말전도가 된다.)
 */
USTRUCT(BlueprintType)
struct FLastFPSLoadingTip
{
	GENERATED_BODY()

	/** 배경/일러스트 이미지 (선택된 항목만 로드됨) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loading")
	TSoftObjectPtr<UTexture2D> Image;

	/** 팁 제목 (예: "TIP", "조작") — 비워도 됨 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loading")
	FText TipTitle;

	/** 팁 본문 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loading", meta=(MultiLine=true))
	FText TipBody;
};

/**
 * 로딩 화면용 이미지+팁 세트 모음 DataAsset.
 *
 * WBP_LoadingScreen(클래스 디폴트)이 이 에셋 하나를 가리키고,
 * 위젯이 로딩마다 PickRandomEntry()로 1세트를 랜덤 선택해 표시한다.
 * 8월 리소스 작업 때 BP 컴파일 없이 에셋에서 항목만 추가/교체하면 된다.
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSLoadingScreenSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 로딩 화면 후보 세트들 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loading", meta=(TitleProperty="TipTitle"))
	TArray<FLastFPSLoadingTip> Entries;

	/**
	 * 직전에 보여준 항목을 제외하고 랜덤으로 1세트 선택.
	 * 비어 있으면 nullptr. 항목이 1개뿐이면 그 항목을 반환(중복 제외 생략).
	 */
	const FLastFPSLoadingTip* PickRandomEntry() const;

private:
	/** 연속 중복 노출 방지용 — 직전 선택 인덱스. 에셋에 저장하지 않음(런타임 전용). */
	UPROPERTY(Transient)
	mutable int32 LastPickedIndex = INDEX_NONE;
};
