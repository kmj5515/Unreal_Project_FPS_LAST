#include "Settings/LastFPSDataTableImportSettings.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "ULastFPSDataTableImportSettings"

namespace
{
	FString GetWorkspaceRoot()
	{
		// 설정값의 기본형인 'LastFPS/Excel'을 프로젝트 파일의 상위 작업 공간 기준으로 일관되게 해석한다.
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("..")));
	}

	FString NormalizeAbsoluteDirectory(const FString& Directory)
	{
		FString Result = FPaths::ConvertRelativePathToFull(Directory);
		FPaths::NormalizeDirectoryName(Result);
		return Result;
	}
}

ULastFPSDataTableImportSettings::ULastFPSDataTableImportSettings()
{
	ExcelDirectory.Path = TEXT("LastFPS/Excel");
	CsvDirectory.Path = TEXT("LastFPS/Excel/Csv");
}

namespace
{
	FString ResolveConfiguredDirectory(const FString& ConfiguredPath)
	{
		if (FPaths::IsRelative(ConfiguredPath))
		{
			return FPaths::ConvertRelativePathToFull(FPaths::Combine(GetWorkspaceRoot(), ConfiguredPath));
		}
		return FPaths::ConvertRelativePathToFull(ConfiguredPath);
	}
}

FString ULastFPSDataTableImportSettings::ResolveExcelDirectory() const
{
	return ResolveConfiguredDirectory(ExcelDirectory.Path);
}

FString ULastFPSDataTableImportSettings::ResolveCsvDirectory() const
{
	// 설정이 비어 있으면 워크북 폴더로 되돌려, 잘못 저장된 설정 때문에 산출물이 엉뚱한 곳에 쌓이지 않게 한다.
	if (CsvDirectory.Path.IsEmpty())
	{
		return ResolveExcelDirectory();
	}
	return ResolveConfiguredDirectory(CsvDirectory.Path);
}

bool ULastFPSDataTableImportSettings::SetExcelDirectoryFromAbsolutePath(const FString& AbsolutePath, FText& OutError)
{
	const FString NormalizedPath = NormalizeAbsoluteDirectory(AbsolutePath);

	const FString ProjectDirectory = NormalizeAbsoluteDirectory(FPaths::ProjectDir());
	// 외부 폴더를 허용하면 상대 경로 계약을 지킬 수 없고 다른 개발 환경에서 같은 설정을 재현할 수 없다.
	if (!FPaths::IsUnderDirectory(NormalizedPath, ProjectDirectory) && NormalizedPath != ProjectDirectory)
	{
		OutError = FText::Format(
			LOCTEXT("OutsideProjectDirectory", "프로젝트 밖의 폴더는 저장할 수 없습니다: {0}"),
			FText::FromString(NormalizedPath));
		return false;
	}

	// 공유 설정에는 머신별 드라이브 문자를 남기지 않도록 검증이 끝난 절대 경로를 작업 공간 상대 경로로 축약한다.
	FString RelativePath = NormalizedPath;
	FString WorkspaceRootWithSeparator = NormalizeAbsoluteDirectory(GetWorkspaceRoot());
	WorkspaceRootWithSeparator += TEXT("/");
	if (!FPaths::MakePathRelativeTo(RelativePath, *WorkspaceRootWithSeparator))
	{
		OutError = FText::Format(
			LOCTEXT("RelativePathConversionFailed", "Excel 폴더를 작업 공간 상대 경로로 변환하지 못했습니다: {0}"),
			FText::FromString(NormalizedPath));
		return false;
	}
	FPaths::NormalizeDirectoryName(RelativePath);

	// 저장 전에 실제 설정 API로 왕복해 폴더명 중복이나 잘못된 기준 경로가 공유 설정에 기록되지 않게 한다.
	const FString PreviousPath = ExcelDirectory.Path;
	ExcelDirectory.Path = RelativePath;
	const FString ResolvedPath = NormalizeAbsoluteDirectory(ResolveExcelDirectory());
	if (!FPaths::IsSamePath(ResolvedPath, NormalizedPath)
		|| !IFileManager::Get().DirectoryExists(*ResolvedPath))
	{
		ExcelDirectory.Path = PreviousPath;
		OutError = FText::Format(
			LOCTEXT("ExcelDirectoryRoundTripFailed", "Excel 폴더 설정 왕복 검증에 실패했습니다. 선택={0}, 해석={1}"),
			FText::FromString(NormalizedPath),
			FText::FromString(ResolvedPath));
		return false;
	}

	SaveConfig();
	OutError = FText::GetEmpty();
	return true;
}

#if WITH_EDITOR
FText ULastFPSDataTableImportSettings::GetSectionText() const
{
	return LOCTEXT("SectionText", "Excel Data Import");
}

FText ULastFPSDataTableImportSettings::GetSectionDescription() const
{
	return LOCTEXT("SectionDescription", "Excel 원본에서 DataTable과 StringTable을 갱신하는 에디터 도구 설정입니다.");
}
#endif

#undef LOCTEXT_NAMESPACE
