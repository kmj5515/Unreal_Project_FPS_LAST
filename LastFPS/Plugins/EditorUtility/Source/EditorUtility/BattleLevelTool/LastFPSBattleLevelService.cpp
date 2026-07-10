#include "BattleLevelTool/LastFPSBattleLevelService.h"

#include "AssetToolsModule.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Character/LastFPSCharacterBase.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Editor.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "IAssetViewport.h"
#include "LevelEditor.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ObjectTools.h"
#include "PlayInEditorDataTypes.h"
#include "Settings/LastFPSBattleLevelSettings.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "EngineUtils.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "FLastFPSBattleLevelService"

namespace
{
	struct FLastFPSPendingBattleScenario
	{
		FSoftObjectPath BattleMapPath;
		FSoftObjectPath PlayerCharacterPath;
		TArray<FLastFPSBattleScenarioMonsterEntry> Monsters;
		bool bPending = false;
	};

	FLastFPSPendingBattleScenario GPendingBattleScenario;
	FDelegateHandle GPostPIEStartedHandle;

	void CollectTaggedSpawnTransforms(UWorld& World, FName SpawnTag, TArray<FTransform>& OutTransforms)
	{
		for (TActorIterator<AActor> ActorIt(&World); ActorIt; ++ActorIt)
		{
			AActor* Actor = *ActorIt;
			if (Actor && Actor->ActorHasTag(SpawnTag))
			{
				OutTransforms.Add(Actor->GetActorTransform());
			}
		}
	}

	bool SpawnScenarioMonster(UWorld& World, const FLastFPSBattleScenarioMonsterEntry& Monster, const FTransform& SpawnTransform)
	{
		ULastFPSCharacterDefinition* Definition = Monster.MonsterDefinition.LoadSynchronous();
		if (!Definition || !Definition->PawnClass)
		{
			return false;
		}

		APawn* Pawn = World.SpawnActorDeferred<APawn>(
			Definition->PawnClass,
			SpawnTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
		);

		if (!Pawn)
		{
			return false;
		}

		if (ALastFPSCharacterBase* Character = Cast<ALastFPSCharacterBase>(Pawn))
		{
			Character->SetCharacterDefinitionForSpawn(Definition);
		}

		Pawn->FinishSpawning(SpawnTransform);
		Pawn->SpawnDefaultController();
		return true;
	}

	bool SpawnScenarioPlayer(UWorld& World)
	{
		ULastFPSCharacterDefinition* Definition = Cast<ULastFPSCharacterDefinition>(GPendingBattleScenario.PlayerCharacterPath.TryLoad());
		if (!Definition || !Definition->PawnClass)
		{
			return false;
		}

		APlayerController* PlayerController = World.GetFirstPlayerController();
		if (!PlayerController)
		{
			return false;
		}

		APawn* PreviousPawn = PlayerController->GetPawn();
		FTransform SpawnTransform = PreviousPawn ? PreviousPawn->GetActorTransform() : FTransform::Identity;

		if (!PreviousPawn)
		{
			for (TActorIterator<APlayerStart> ActorIt(&World); ActorIt; ++ActorIt)
			{
				SpawnTransform = ActorIt->GetActorTransform();
				break;
			}
		}

		APawn* NewPawn = World.SpawnActorDeferred<APawn>(
			Definition->PawnClass,
			SpawnTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
		);

		if (!NewPawn)
		{
			return false;
		}

		if (ALastFPSCharacterBase* Character = Cast<ALastFPSCharacterBase>(NewPawn))
		{
			Character->SetCharacterDefinitionForSpawn(Definition);
		}

		NewPawn->FinishSpawning(SpawnTransform);
		PlayerController->Possess(NewPawn);

		if (PreviousPawn && PreviousPawn != NewPawn)
		{
			PreviousPawn->Destroy();
		}

		return true;
	}

	void SpawnPendingScenarioMonsters(UWorld& PlayWorld)
	{
		int32 SpawnedCount = 0;

		for (const FLastFPSBattleScenarioMonsterEntry& Monster : GPendingBattleScenario.Monsters)
		{
			const FName SpawnTag = Monster.SpawnTag.IsNone() ? FName(TEXT("EnemySpawn")) : Monster.SpawnTag;

			TArray<FTransform> SpawnTransforms;
			CollectTaggedSpawnTransforms(PlayWorld, SpawnTag, SpawnTransforms);
			if (SpawnTransforms.IsEmpty())
			{
				CollectTaggedSpawnTransforms(PlayWorld, TEXT("EnemySpawn"), SpawnTransforms);
			}

			if (SpawnTransforms.IsEmpty())
			{
				continue;
			}

			const int32 Count = FMath::Max(Monster.Count, 1);
			for (int32 SpawnIndex = 0; SpawnIndex < Count; ++SpawnIndex)
			{
				FTransform SpawnTransform = SpawnTransforms[SpawnIndex % SpawnTransforms.Num()];
				SpawnTransform.AddToTranslation(FVector(0.f, SpawnIndex * 120.f, 0.f));

				if (SpawnScenarioMonster(PlayWorld, Monster, SpawnTransform))
				{
					++SpawnedCount;
				}
			}
		}

		UE_LOG(LogTemp, Log, TEXT("LastFPS Battle Scenario spawned %d monsters."), SpawnedCount);
	}

	void HandlePostPIEStarted(const bool bIsSimulatingInEditor)
	{
		if (!GPendingBattleScenario.bPending)
		{
			return;
		}

		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (PlayWorld)
		{
			SpawnScenarioPlayer(*PlayWorld);
			SpawnPendingScenarioMonsters(*PlayWorld);
		}

		GPendingBattleScenario = FLastFPSPendingBattleScenario();
	}

	void EnsureScenarioPIEDelegate()
	{
		if (!GPostPIEStartedHandle.IsValid())
		{
			GPostPIEStartedHandle = FEditorDelegates::PostPIEStarted.AddStatic(&HandlePostPIEStarted);
		}
	}

	FString MakeScenarioAssetName(const FString& ScenarioName)
	{
		FString AssetName = ObjectTools::SanitizeObjectName(ScenarioName.TrimStartAndEnd());
		if (!AssetName.IsEmpty() && !AssetName.StartsWith(TEXT("BS_")))
		{
			AssetName = TEXT("BS_") + AssetName;
		}

		return AssetName;
	}

	bool ValidateScenarioDraft(
		const FString& ScenarioName,
		const FSoftObjectPath& BattleMapPath,
		const FSoftObjectPath& PlayerCharacterPath,
		const TArray<FLastFPSBattleScenarioMonsterEntry>& Monsters,
		FString& OutScenarioId,
		FText& OutErrorMessage)
	{
		OutScenarioId = MakeScenarioAssetName(ScenarioName);
		if (OutScenarioId.IsEmpty())
		{
			OutErrorMessage = LOCTEXT("ScenarioNameMissing", "시나리오 이름이 필요합니다.");
			return false;
		}

		if (!BattleMapPath.IsValid())
		{
			OutErrorMessage = LOCTEXT("ScenarioMapMissing", "배틀 맵이 필요합니다.");
			return false;
		}

		if (!PlayerCharacterPath.IsValid())
		{
			OutErrorMessage = LOCTEXT("ScenarioPlayerMissing", "플레이어 캐릭터가 필요합니다.");
			return false;
		}

		if (Monsters.IsEmpty())
		{
			OutErrorMessage = LOCTEXT("ScenarioMonsterMissing", "몬스터 항목이 하나 이상 필요합니다.");
			return false;
		}

		return true;
	}

	void ApplyScenarioDraft(
		ULastFPSBattleScenarioDefinition& Scenario,
		const FString& ScenarioId,
		const FSoftObjectPath& BattleMapPath,
		const FSoftObjectPath& PlayerCharacterPath,
		const TArray<FLastFPSBattleScenarioMonsterEntry>& Monsters)
	{
		Scenario.ScenarioId = *ScenarioId;
		Scenario.BattleMap = TSoftObjectPtr<UWorld>(BattleMapPath);
		Scenario.PlayerCharacter = TSoftObjectPtr<ULastFPSCharacterDefinition>(PlayerCharacterPath);
		Scenario.Monsters = Monsters;
	}

	bool SaveScenarioPackage(ULastFPSBattleScenarioDefinition& Scenario, FText& OutErrorMessage)
	{
		UPackage* Package = Scenario.GetOutermost();
		if (!Package)
		{
			OutErrorMessage = LOCTEXT("ScenarioPackageMissing", "시나리오 패키지를 사용할 수 없습니다.");
			return false;
		}

		Package->MarkPackageDirty();

		const FString PackageFileName = FPackageName::LongPackageNameToFilename(
			Package->GetName(),
			FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(PackageFileName), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_None;

		if (!UPackage::SavePackage(Package, &Scenario, *PackageFileName, SaveArgs))
		{
			OutErrorMessage = LOCTEXT("ScenarioSaveFailed", "시나리오 에셋 저장에 실패했습니다.");
			return false;
		}

		return true;
	}
}

const FString& FLastFPSBattleLevelService::GetScenarioRootPath()
{
	static const FString ScenarioRootPath(TEXT("/Game/Editor/BattleScenarios"));
	return ScenarioRootPath;
}

void FLastFPSBattleLevelService::CollectBattleLevels(TArray<FLastFPSBattleLevelInfo>& OutBattleLevels)
{
	OutBattleLevels.Reset();

	const FString RootPath = GetBattleLevelRootPath();
	if (RootPath.IsEmpty() || !FPackageName::IsValidPath(RootPath))
	{
		return;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(*RootPath);
	Filter.ClassPaths.Add(UWorld::StaticClass()->GetClassPathName());

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

	for (const FAssetData& AssetData : AssetDataList)
	{
		FLastFPSBattleLevelInfo LevelInfo;
		LevelInfo.DisplayName = AssetData.AssetName.ToString();
		LevelInfo.PackageName = AssetData.PackageName.ToString();
		LevelInfo.AssetPath = AssetData.GetSoftObjectPath();
		OutBattleLevels.Add(LevelInfo);
	}

	OutBattleLevels.Sort([](const FLastFPSBattleLevelInfo& Left, const FLastFPSBattleLevelInfo& Right)
	{
		return Left.DisplayName < Right.DisplayName;
	});
}

bool FLastFPSBattleLevelService::OpenBattleLevel(const FLastFPSBattleLevelInfo& BattleLevel)
{
	if (!IsPackageUsable(BattleLevel.PackageName))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("InvalidOpenMap", "배틀 레벨 패키지가 유효하지 않거나 없습니다."));
		return false;
	}

	const FString MapFilename = FPackageName::LongPackageNameToFilename(BattleLevel.PackageName, FPackageName::GetMapPackageExtension());
	return FEditorFileUtils::LoadMap(MapFilename, false, true);
}

bool FLastFPSBattleLevelService::PlayBattleLevel(const FLastFPSBattleLevelInfo& BattleLevel)
{
	if (!GUnrealEd)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("EditorMissing", "언리얼 에디터가 준비되지 않았습니다."));
		return false;
	}

	if (GUnrealEd->IsPlaySessionInProgress())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("PlayInProgress", "이미 실행 중이거나 대기 중인 플레이 세션이 있습니다."));
		return false;
	}

	if (!IsPackageUsable(BattleLevel.PackageName))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("InvalidPlayMap", "배틀 레벨 패키지가 유효하지 않거나 없습니다."));
		return false;
	}

	FRequestPlaySessionParams SessionParams;
	SessionParams.GlobalMapOverride = BattleLevel.PackageName;

	if (FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor")))
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		if (TSharedPtr<IAssetViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport())
		{
			SessionParams.DestinationSlateViewport = ActiveLevelViewport;
		}
	}

	GUnrealEd->RequestPlaySession(SessionParams);
	GUnrealEd->StartQueuedPlaySessionRequest();
	return true;
}

bool FLastFPSBattleLevelService::PlayScenario(
	const FSoftObjectPath& BattleMapPath,
	const FSoftObjectPath& PlayerCharacterPath,
	const TArray<FLastFPSBattleScenarioMonsterEntry>& Monsters,
	FText& OutErrorMessage)
{
	if (!GUnrealEd)
	{
		OutErrorMessage = LOCTEXT("ScenarioEditorMissing", "언리얼 에디터가 준비되지 않았습니다.");
		return false;
	}

	if (GUnrealEd->IsPlaySessionInProgress())
	{
		OutErrorMessage = LOCTEXT("ScenarioPlayInProgress", "이미 실행 중이거나 대기 중인 플레이 세션이 있습니다.");
		return false;
	}

	if (!BattleMapPath.IsValid())
	{
		OutErrorMessage = LOCTEXT("ScenarioPlayMapMissing", "배틀 맵이 필요합니다.");
		return false;
	}

	if (!PlayerCharacterPath.IsValid())
	{
		OutErrorMessage = LOCTEXT("ScenarioPlayPlayerMissing", "플레이어 캐릭터가 필요합니다.");
		return false;
	}

	if (Monsters.IsEmpty())
	{
		OutErrorMessage = LOCTEXT("ScenarioPlayMonsterMissing", "몬스터 항목이 하나 이상 필요합니다.");
		return false;
	}

	const FString MapPackageName = BattleMapPath.GetLongPackageName();
	if (!IsPackageUsable(MapPackageName))
	{
		OutErrorMessage = LOCTEXT("ScenarioPlayMapInvalid", "배틀 맵 패키지가 유효하지 않거나 없습니다.");
		return false;
	}

	GPendingBattleScenario.BattleMapPath = BattleMapPath;
	GPendingBattleScenario.PlayerCharacterPath = PlayerCharacterPath;
	GPendingBattleScenario.Monsters = Monsters;
	GPendingBattleScenario.bPending = true;
	EnsureScenarioPIEDelegate();

	FRequestPlaySessionParams SessionParams;
	SessionParams.GlobalMapOverride = MapPackageName;

	if (FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor")))
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		if (TSharedPtr<IAssetViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport())
		{
			SessionParams.DestinationSlateViewport = ActiveLevelViewport;
		}
	}

	GUnrealEd->RequestPlaySession(SessionParams);
	GUnrealEd->StartQueuedPlaySessionRequest();
	return true;
}

bool FLastFPSBattleLevelService::PlayScenarioAsset(
	const FSoftObjectPath& ScenarioPath,
	FText& OutErrorMessage)
{
	FString LoadedScenarioName;
	FString LoadedBattleMapObjectPath;
	FString LoadedPlayerCharacterObjectPath;
	TArray<FLastFPSBattleScenarioMonsterEntry> LoadedMonsters;

	if (!LoadScenarioAsset(
		ScenarioPath,
		LoadedScenarioName,
		LoadedBattleMapObjectPath,
		LoadedPlayerCharacterObjectPath,
		LoadedMonsters,
		OutErrorMessage))
	{
		return false;
	}

	return PlayScenario(
		FSoftObjectPath(LoadedBattleMapObjectPath),
		FSoftObjectPath(LoadedPlayerCharacterObjectPath),
		LoadedMonsters,
		OutErrorMessage);
}

bool FLastFPSBattleLevelService::LoadScenarioAsset(
	const FSoftObjectPath& ScenarioPath,
	FString& OutScenarioName,
	FString& OutBattleMapObjectPath,
	FString& OutPlayerCharacterObjectPath,
	TArray<FLastFPSBattleScenarioMonsterEntry>& OutMonsters,
	FText& OutErrorMessage)
{
	OutScenarioName.Reset();
	OutBattleMapObjectPath.Reset();
	OutPlayerCharacterObjectPath.Reset();
	OutMonsters.Reset();

	if (!ScenarioPath.IsValid())
	{
		OutErrorMessage = LOCTEXT("ScenarioAssetMissing", "시나리오 에셋이 필요합니다.");
		return false;
	}

	const ULastFPSBattleScenarioDefinition* Scenario = Cast<ULastFPSBattleScenarioDefinition>(ScenarioPath.TryLoad());
	if (!Scenario)
	{
		OutErrorMessage = LOCTEXT("ScenarioAssetLoadFailed", "선택한 시나리오 에셋을 불러오지 못했습니다.");
		return false;
	}

	OutScenarioName = Scenario->ScenarioId.IsNone() ? Scenario->GetName() : Scenario->ScenarioId.ToString();
	OutBattleMapObjectPath = Scenario->BattleMap.ToSoftObjectPath().ToString();
	OutPlayerCharacterObjectPath = Scenario->PlayerCharacter.ToSoftObjectPath().ToString();
	OutMonsters = Scenario->Monsters;
	return true;
}

bool FLastFPSBattleLevelService::CreateScenarioAsset(
	const FString& ScenarioName,
	const FSoftObjectPath& BattleMapPath,
	const FSoftObjectPath& PlayerCharacterPath,
	const TArray<FLastFPSBattleScenarioMonsterEntry>& Monsters,
	FString& OutScenarioObjectPath,
	FText& OutErrorMessage)
{
	OutScenarioObjectPath.Reset();

	FString AssetName;
	if (!ValidateScenarioDraft(ScenarioName, BattleMapPath, PlayerCharacterPath, Monsters, AssetName, OutErrorMessage))
	{
		return false;
	}

	FString PackageName;
	FString UniqueAssetName;
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	AssetToolsModule.Get().CreateUniqueAssetName(GetScenarioRootPath() + TEXT("/") + AssetName, TEXT(""), PackageName, UniqueAssetName);

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		OutErrorMessage = LOCTEXT("ScenarioPackageFailed", "시나리오 패키지 생성에 실패했습니다.");
		return false;
	}

	ULastFPSBattleScenarioDefinition* Scenario = NewObject<ULastFPSBattleScenarioDefinition>(
		Package,
		ULastFPSBattleScenarioDefinition::StaticClass(),
		*UniqueAssetName,
		RF_Public | RF_Standalone | RF_Transactional
	);

	if (!Scenario)
	{
		OutErrorMessage = LOCTEXT("ScenarioAssetFailed", "시나리오 에셋 생성에 실패했습니다.");
		return false;
	}

	ApplyScenarioDraft(*Scenario, UniqueAssetName, BattleMapPath, PlayerCharacterPath, Monsters);

	FAssetRegistryModule::AssetCreated(Scenario);

	if (!SaveScenarioPackage(*Scenario, OutErrorMessage))
	{
		return false;
	}

	OutScenarioObjectPath = Scenario->GetPathName();
	return true;
}

bool FLastFPSBattleLevelService::SaveScenarioAsset(
	const FSoftObjectPath& ScenarioPath,
	const FString& ScenarioName,
	const FSoftObjectPath& BattleMapPath,
	const FSoftObjectPath& PlayerCharacterPath,
	const TArray<FLastFPSBattleScenarioMonsterEntry>& Monsters,
	FString& OutScenarioObjectPath,
	FText& OutErrorMessage)
{
	OutScenarioObjectPath.Reset();

	if (!ScenarioPath.IsValid())
	{
		OutErrorMessage = LOCTEXT("ScenarioAssetMissingForSave", "저장하기 전에 시나리오 에셋을 선택하세요.");
		return false;
	}

	FString ScenarioId;
	if (!ValidateScenarioDraft(ScenarioName, BattleMapPath, PlayerCharacterPath, Monsters, ScenarioId, OutErrorMessage))
	{
		return false;
	}

	ULastFPSBattleScenarioDefinition* Scenario = Cast<ULastFPSBattleScenarioDefinition>(ScenarioPath.TryLoad());
	if (!Scenario)
	{
		OutErrorMessage = LOCTEXT("ScenarioAssetSaveLoadFailed", "선택한 시나리오 에셋을 불러오지 못했습니다.");
		return false;
	}

	Scenario->Modify();
	ApplyScenarioDraft(*Scenario, ScenarioId, BattleMapPath, PlayerCharacterPath, Monsters);

	if (!SaveScenarioPackage(*Scenario, OutErrorMessage))
	{
		return false;
	}

	OutScenarioObjectPath = Scenario->GetPathName();
	return true;
}

void FLastFPSBattleLevelService::ValidateCurrentBattleLevel(TArray<FLastFPSBattleLevelValidationMessage>& OutMessages)
{
	OutMessages.Reset();

	UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!EditorWorld)
	{
		AddMessage(OutMessages, ELastFPSBattleLevelValidationSeverity::Error, LOCTEXT("NoEditorWorld", "에디터 월드를 사용할 수 없습니다."));
		return;
	}

	const FString CurrentPackageName = GetCurrentLevelPackageName();
	if (CurrentPackageName.IsEmpty())
	{
		AddMessage(OutMessages, ELastFPSBattleLevelValidationSeverity::Error, LOCTEXT("NoCurrentLevel", "현재 레벨 패키지를 확인할 수 없습니다."));
		return;
	}

	if (!IsBattleLevelPackage(CurrentPackageName))
	{
		AddMessage(OutMessages, ELastFPSBattleLevelValidationSeverity::Warning, FText::Format(
			LOCTEXT("OutsideBattleRoot", "현재 레벨이 설정된 배틀 레벨 루트 밖에 있습니다: {0}"),
			FText::FromString(GetBattleLevelRootPath())
		));
	}

	const ULastFPSBattleLevelSettings* Settings = ULastFPSBattleLevelSettings::Get();
	if (!Settings)
	{
		AddMessage(OutMessages, ELastFPSBattleLevelValidationSeverity::Error, LOCTEXT("MissingSettings", "배틀 레벨 설정을 사용할 수 없습니다."));
		return;
	}

	if (Settings->bRequirePlayerStart)
	{
		bool bHasPlayerStart = false;
		for (TActorIterator<APlayerStart> ActorIt(EditorWorld); ActorIt; ++ActorIt)
		{
			bHasPlayerStart = true;
			break;
		}

		if (!bHasPlayerStart)
		{
			AddMessage(OutMessages, ELastFPSBattleLevelValidationSeverity::Error, LOCTEXT("MissingPlayerStart", "PlayerStart 액터가 없습니다."));
		}
	}

	for (const FName RequiredTag : Settings->RequiredActorTags)
	{
		if (RequiredTag.IsNone())
		{
			continue;
		}

		if (!HasActorWithTag(*EditorWorld, RequiredTag))
		{
			AddMessage(OutMessages, ELastFPSBattleLevelValidationSeverity::Warning, FText::Format(
				LOCTEXT("MissingRequiredTag", "필수 태그를 가진 액터가 없습니다: {0}"),
				FText::FromName(RequiredTag)
			));
		}
	}

	if (OutMessages.IsEmpty())
	{
		AddMessage(OutMessages, ELastFPSBattleLevelValidationSeverity::Info, LOCTEXT("ValidationPassed", "배틀 레벨 검증을 통과했습니다."));
	}
}

FString FLastFPSBattleLevelService::GetCurrentLevelPackageName()
{
	UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	return EditorWorld && EditorWorld->GetOutermost() ? EditorWorld->GetOutermost()->GetName() : FString();
}

FString FLastFPSBattleLevelService::GetBattleLevelRootPath()
{
	const ULastFPSBattleLevelSettings* Settings = ULastFPSBattleLevelSettings::Get();
	if (!Settings)
	{
		return FString();
	}

	FString RootPath = Settings->BattleLevelRootPath.Path;
	RootPath.RemoveFromEnd(TEXT("/"));
	return RootPath;
}

bool FLastFPSBattleLevelService::IsPackageUsable(const FString& PackageName)
{
	return !PackageName.IsEmpty()
		&& FPackageName::IsValidLongPackageName(PackageName)
		&& FPackageName::DoesPackageExist(PackageName);
}

bool FLastFPSBattleLevelService::IsBattleLevelPackage(const FString& PackageName)
{
	const FString RootPath = GetBattleLevelRootPath();
	return !RootPath.IsEmpty() && (PackageName == RootPath || PackageName.StartsWith(RootPath + TEXT("/")));
}

bool FLastFPSBattleLevelService::HasActorWithTag(UWorld& World, FName TagName)
{
	for (TActorIterator<AActor> ActorIt(&World); ActorIt; ++ActorIt)
	{
		const AActor* Actor = *ActorIt;
		if (Actor && Actor->ActorHasTag(TagName))
		{
			return true;
		}
	}

	return false;
}

void FLastFPSBattleLevelService::AddMessage(TArray<FLastFPSBattleLevelValidationMessage>& OutMessages, ELastFPSBattleLevelValidationSeverity Severity, const FText& Message)
{
	FLastFPSBattleLevelValidationMessage NewMessage;
	NewMessage.Severity = Severity;
	NewMessage.Message = Message;
	OutMessages.Add(NewMessage);
}

#undef LOCTEXT_NAMESPACE
