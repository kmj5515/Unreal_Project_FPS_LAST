#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LastFPSDataTableImportSettings.generated.h"

UCLASS(Config=LastFPS, DefaultConfig, meta=(DisplayName="LastFPS Excel Data Import"))
class EDITORUTILITY_API ULastFPSDataTableImportSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	ULastFPSDataTableImportSettings();

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
	virtual bool SupportsAutoRegistration() const override { return false; }
#endif

	// 절대 경로는 사용자와 작업 공간마다 달라 공유 설정을 깨뜨리므로 반드시 프로젝트와 함께 이동 가능한 상대 경로로 저장한다.
	UPROPERTY(Config, EditAnywhere, Category="Excel", meta=(RelativeToGameDir))
	FDirectoryPath ExcelDirectory;

	FString ResolveExcelDirectory() const;
	bool SetExcelDirectoryFromAbsolutePath(const FString& AbsolutePath, FText& OutError);

	static const ULastFPSDataTableImportSettings* Get()
	{
		return GetDefault<ULastFPSDataTableImportSettings>();
	}
};
