#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LastFPSDataTableImportRegistry.generated.h"

USTRUCT(BlueprintType)
struct EDITORUTILITY_API FLastFPSDataTableImportMapping
{
	GENERATED_BODY()
	// 워크북과 시트의 조합을 안정적인 키로 삼아 파일명이나 대상 에셋의 변경이 임포트 로직으로 번지지 않게 한다.

	UPROPERTY(EditAnywhere, Category="Import")
	FName WorkbookName;

	UPROPERTY(EditAnywhere, Category="Import")
	FName SheetName;

	UPROPERTY(EditAnywhere, Category="Import")
	FString CsvFileName;

	UPROPERTY(EditAnywhere, Category="Import", meta=(AllowedClasses="/Script/Engine.DataTable,/Script/Engine.StringTable"))
	TSoftObjectPtr<UObject> TargetAsset;

	static bool IsValidCsvFileName(const FString& Candidate);
};

UCLASS(BlueprintType)
class EDITORUTILITY_API ULastFPSDataTableImportRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	// 대상 에셋은 편집 도구를 열 때 모두 로드하지 않도록 소프트 참조로 유지한다.
	UPROPERTY(EditAnywhere, Category="Import")
	TArray<FLastFPSDataTableImportMapping> Mappings;

	const FLastFPSDataTableImportMapping* FindMapping(FName WorkbookName, FName SheetName) const;
	bool AddMapping(
		FName WorkbookName,
		FName SheetName,
		const FString& CsvFileName,
		const FSoftObjectPath& TargetAssetPath);
};
