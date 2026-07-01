// NPC preset blueprint generator.

#include "NPCPresetGenLibrary.h"

#include "Engine/DataTable.h"
#include "Engine/Blueprint.h"
#include "Factories/BlueprintFactory.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "EditorAssetLibrary.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogNPCPresetGen, Log, All);

FNPCPresetGenSummary UNPCPresetGenLibrary::GenerateFromTable(
	UDataTable* NPCTable,
	const FString& SavePath,
	const FString& AssetPrefix,
	bool bOverwriteExisting)
{
	FNPCPresetGenSummary Summary;

	if (!NPCTable)
	{
		Summary.Message = TEXT("NPCTable 이 null 입니다.");
		return Summary;
	}

	// 게임 모듈에 링크하지 않고 클래스 경로로 부모를 로드 (에디터 플러그인 독립성 유지).
	UClass* ParentClass = LoadObject<UClass>(nullptr, TEXT("/Script/LastFPS.LastFPSNPC"));
	if (!ParentClass)
	{
		Summary.Message = TEXT("ALastFPSNPC 클래스를 찾지 못했습니다 (/Script/LastFPS.LastFPSNPC).");
		return Summary;
	}

	const FString Prefix = AssetPrefix.IsEmpty() ? TEXT("BP_") : AssetPrefix;
	const FString Path = SavePath.IsEmpty() ? TEXT("/Game/NPC") : SavePath;

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

	for (const FName RowName : NPCTable->GetRowNames())
	{
		const FString AssetName = Prefix + RowName.ToString();
		const FString ObjectPath = FString::Printf(TEXT("%s/%s"), *Path, *AssetName);

		if (UEditorAssetLibrary::DoesAssetExist(ObjectPath))
		{
			if (!bOverwriteExisting)
			{
				Summary.NumSkipped++;
				continue;
			}
			UEditorAssetLibrary::DeleteAsset(ObjectPath);
		}

		UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
		Factory->ParentClass = ParentClass;

		UObject* NewAsset = AssetToolsModule.Get().CreateAsset(AssetName, Path, UBlueprint::StaticClass(), Factory);
		UBlueprint* BP = Cast<UBlueprint>(NewAsset);
		if (!BP)
		{
			Summary.Message += FString::Printf(TEXT("생성 실패: %s\n"), *AssetName);
			continue;
		}

		// NPCRowName 기본값을 CDO에 세팅 (상속 프로퍼티라 CDO에 기록되어 인스턴스가 상속).
		if (UClass* GenClass = BP->GeneratedClass)
		{
			if (UObject* CDO = GenClass->GetDefaultObject())
			{
				if (FNameProperty* RowProp = FindFProperty<FNameProperty>(GenClass, TEXT("NPCRowName")))
				{
					RowProp->SetPropertyValue_InContainer(CDO, RowName);
				}
			}
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		FKismetEditorUtilities::CompileBlueprint(BP);
		UEditorAssetLibrary::SaveLoadedAsset(BP, /*bOnlyIfIsDirty=*/false);

		Summary.NumCreated++;
	}

	Summary.Message += FString::Printf(TEXT("생성 %d개, 건너뜀 %d개."), Summary.NumCreated, Summary.NumSkipped);
	UE_LOG(LogNPCPresetGen, Log, TEXT("%s"), *Summary.Message);
	return Summary;
}
