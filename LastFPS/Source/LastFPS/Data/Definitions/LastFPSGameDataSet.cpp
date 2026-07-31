#include "Data/Definitions/LastFPSGameDataSet.h"

#include "Data/AssetManagement/LastFPSPrimaryAssetTypes.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "LastFPSGameDataSet"

const FPrimaryAssetType ULastFPSGameDataSet::PrimaryAssetType =
	LastFPSPrimaryAssetTypes::GameDataSet;

FPrimaryAssetId ULastFPSGameDataSet::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}

const FLastFPSDataTableReference* ULastFPSGameDataSet::FindTable(
	const FGameplayTag TableId) const
{
	if (!TableId.IsValid())
	{
		return nullptr;
	}

	const auto FindInGroup = [TableId](
		const TArray<FLastFPSDataTableReference>& Group)
	{
		return Group.FindByPredicate(
			[TableId](const FLastFPSDataTableReference& Entry)
			{
				return Entry.TableId == TableId;
			});
	};

	if (const FLastFPSDataTableReference* Found = FindInGroup(StartupTables))
	{
		return Found;
	}
	if (const FLastFPSDataTableReference* Found = FindInGroup(GameTables))
	{
		return Found;
	}
	if (const FLastFPSDataTableReference* Found = FindInGroup(UITables))
	{
		return Found;
	}

	return nullptr;
}

#if WITH_EDITOR
EDataValidationResult ULastFPSGameDataSet::IsDataValid(
	FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	if (ParentResult == EDataValidationResult::Invalid)
	{
		return ParentResult;
	}

	bool bIsValid = true;
	TSet<FGameplayTag> RegisteredTableIds;

	const auto ValidateGroup =
		[&Context, &RegisteredTableIds, &bIsValid](
			const TArray<FLastFPSDataTableReference>& Group)
	{
		for (const FLastFPSDataTableReference& Entry : Group)
		{
			if (!Entry.TableId.IsValid())
			{
				Context.AddError(LOCTEXT(
					"InvalidTableId",
					"GameDataSet에 유효하지 않은 데이터 테이블 태그가 있습니다."));
				bIsValid = false;
				continue;
			}

			if (Entry.Table.IsNull())
			{
				Context.AddError(FText::Format(
					LOCTEXT(
						"MissingTable",
						"'{0}' 데이터 테이블 참조가 비어 있습니다."),
					FText::FromName(Entry.TableId.GetTagName())));
				bIsValid = false;
			}

			if (RegisteredTableIds.Contains(Entry.TableId))
			{
				Context.AddError(FText::Format(
					LOCTEXT(
						"DuplicateTableId",
						"'{0}' 데이터 테이블 태그가 둘 이상의 번들에 중복 등록되었습니다."),
					FText::FromName(Entry.TableId.GetTagName())));
				bIsValid = false;
				continue;
			}

			RegisteredTableIds.Add(Entry.TableId);
		}
	};

	ValidateGroup(StartupTables);
	ValidateGroup(GameTables);
	ValidateGroup(UITables);

	return bIsValid
		? EDataValidationResult::Valid
		: EDataValidationResult::Invalid;
}
#endif

#undef LOCTEXT_NAMESPACE
