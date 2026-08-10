#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Data/AssetManagement/LastFPSGameDataSubsystem.h"
#include "Data/AssetManagement/LastFPSGameDataTags.h"
#include "Data/Tables/LastFPSItemData.h"
#include "Game/LastFPSPlayerController.h"
#include "UI/Result/LastFPSMissionResultTypes.h"

#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSPopupDebug, Log, All);

/**
 * 개발용 팝업 치트.
 *
 * 팝업을 직접 생성하지 않고 ALastFPSPlayerController 의 표시 API 를 그대로 호출한다.
 * 치트가 위젯 클래스나 팝업 태그를 알지 않으므로, 실제 게임 경로와 같은 코드를 지나며
 * 카탈로그 등록 누락 같은 문제도 그대로 재현된다.
 *
 * 팝업이 늘어나면 아래 레지스트리에 항목 하나만 추가한다(분기문을 늘리지 않는다).
 */
namespace LastFPSPopupDebugCommands
{
	/** 치트 전용 표본 문구. 실제 UI 문구가 아니므로 문자열 테이블로 옮기지 않는다. */
	namespace SampleText
	{
		const TCHAR* NoticeTitle = TEXT("[치트] 공지 팝업");
		const TCHAR* NoticeBody = TEXT("공지 팝업 표시 테스트입니다.");
		const TCHAR* ConfirmTitle = TEXT("[치트] 확인 팝업");
		const TCHAR* ConfirmBody = TEXT("확인/취소 결과는 로그로 출력됩니다.");
		const TCHAR* QuantityTitle = TEXT("[치트] 수량 선택");
		const TCHAR* QuantityItemName = TEXT("표본 아이템");
		const TCHAR* DialogueSpeaker = TEXT("[치트] 작전 통제관");
		const TCHAR* DialogueLine1 = TEXT("대화 팝업 첫 번째 줄입니다.");
		const TCHAR* DialogueLine2 = TEXT("다음 줄로 넘어가는지 확인하십시오.");
		const TCHAR* MissionName = TEXT("[치트] 잊혀진 전당");
	}

	/** 인자가 없거나 숫자가 아니면 기본값을 쓴다. 치트라 입력 오류로 중단시키지 않는다. */
	int32 ParseIntArg(const TArray<FString>& Args, const int32 Index, const int32 DefaultValue)
	{
		if (!Args.IsValidIndex(Index))
		{
			return DefaultValue;
		}
		return FCString::IsNumeric(*Args[Index]) ? FCString::Atoi(*Args[Index]) : DefaultValue;
	}

	/** 보상 아이콘 표시를 확인하려면 실제 아이템 행이 필요하다. 앞쪽 몇 개를 표본으로 쓴다. */
	TArray<FLastFPSItemGrant> BuildSampleItemGrants(const UWorld& World, const int32 DesiredCount)
	{
		TArray<FLastFPSItemGrant> Grants;

		const UGameInstance* GameInstance = World.GetGameInstance();
		ULastFPSGameDataSubsystem* GameData =
			GameInstance ? GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>() : nullptr;
		const UDataTable* ItemTable =
			GameData ? GameData->FindTable(LastFPSGameDataTags::Data_Table_Economy_Item) : nullptr;
		if (!ItemTable)
		{
			UE_LOG(LogLastFPSPopupDebug, Warning,
				TEXT("[치트] 아이템 테이블을 찾지 못해 보상 아이콘 없이 표시합니다."));
			return Grants;
		}

		const TArray<FName> RowNames = ItemTable->GetRowNames();
		const int32 Count = FMath::Min(DesiredCount, RowNames.Num());
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FLastFPSItemGrant& Grant = Grants.AddDefaulted_GetRef();
			Grant.RowId = RowNames[Index];
			Grant.Count = Index + 1; // 수량 표기(×N)까지 확인하려고 1개짜리를 섞지 않는다.
		}
		return Grants;
	}

	void ShowNotice(ALastFPSPlayerController& PC, const TArray<FString>&)
	{
		PC.ShowNotice(
			FText::FromString(SampleText::NoticeTitle),
			FText::FromString(SampleText::NoticeBody));
	}

	void ShowConfirm(ALastFPSPlayerController& PC, const TArray<FString>&)
	{
		// 결과 델리게이트는 동적(UFUNCTION 전용)이라 치트에서 람다로 받을 수 없다.
		// 표시 확인이 목적이므로 비워 둔다.
		PC.ShowConfirm(
			FText::FromString(SampleText::ConfirmTitle),
			FText::FromString(SampleText::ConfirmBody),
			FLastFPSConfirmResultDelegate());
	}

	void ShowQuantity(ALastFPSPlayerController& PC, const TArray<FString>& Args)
	{
		const int32 UnitPrice = ParseIntArg(Args, 0, 250);
		const int32 MaxQuantity = ParseIntArg(Args, 1, 9);

		PC.ShowQuantityPrompt(
			FText::FromString(SampleText::QuantityTitle),
			FText::FromString(SampleText::QuantityItemName),
			UnitPrice,
			MaxQuantity,
			FLastFPSQuantityResultDelegate());
	}

	void ShowDialogue(ALastFPSPlayerController& PC, const TArray<FString>&)
	{
		const TArray<FText> Lines = {
			FText::FromString(SampleText::DialogueLine1),
			FText::FromString(SampleText::DialogueLine2),
		};
		PC.ShowDialogue(FText::FromString(SampleText::DialogueSpeaker), Lines);
	}

	void ShowMissionResult(ALastFPSPlayerController& PC, const TArray<FString>& Args)
	{
		const UWorld* World = PC.GetWorld();
		if (!World)
		{
			return;
		}

		FLastFPSMissionResult Result;
		Result.MissionName = FText::FromString(SampleText::MissionName);
		Result.ElapsedSeconds = static_cast<float>(ParseIntArg(Args, 0, 604));
		Result.Credits = ParseIntArg(Args, 1, 59427);
		Result.Items = BuildSampleItemGrants(*World, ParseIntArg(Args, 2, 3));

		// 전투 통계와 캐릭터 초상은 컨트롤러가 PlayerState/로스터에서 채운다.
		// 점수는 산출 시스템이 없어 비워 두며, 결과 화면이 점수 구획을 접는지 함께 확인된다.
		PC.ShowMissionResult(Result);
	}

	using FPopupCommandHandler = void (*)(ALastFPSPlayerController&, const TArray<FString>&);

	struct FPopupCommandEntry
	{
		const TCHAR* Usage = nullptr;
		FPopupCommandHandler Handler = nullptr;
	};

	const TMap<FName, FPopupCommandEntry>& GetRegistry()
	{
		static const TMap<FName, FPopupCommandEntry> Registry = {
			{ TEXT("Notice"),        { TEXT("Notice — 공지 팝업"), &ShowNotice } },
			{ TEXT("Confirm"),       { TEXT("Confirm — 확인/취소 팝업"), &ShowConfirm } },
			{ TEXT("Quantity"),      { TEXT("Quantity [단가] [최대수량] — 수량 선택 팝업"), &ShowQuantity } },
			{ TEXT("Dialogue"),      { TEXT("Dialogue — NPC 대화 팝업"), &ShowDialogue } },
			{ TEXT("MissionResult"), { TEXT("MissionResult [초] [크레딧] [아이템수] — 임무 결과 화면"), &ShowMissionResult } },
		};
		return Registry;
	}

	void LogUsage()
	{
		UE_LOG(LogLastFPSPopupDebug, Display, TEXT("사용법: LastFPS.UI.Popup <이름> [인자...]"));
		for (const TPair<FName, FPopupCommandEntry>& Pair : GetRegistry())
		{
			UE_LOG(LogLastFPSPopupDebug, Display, TEXT("  %s"), Pair.Value.Usage);
		}
	}

	void HandlePopupCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || Args.Num() < 1)
		{
			LogUsage();
			return;
		}

		const FName CommandName(*Args[0]);
		const FPopupCommandEntry* Entry = GetRegistry().Find(CommandName);
		if (!Entry || !Entry->Handler)
		{
			UE_LOG(LogLastFPSPopupDebug, Warning, TEXT("알 수 없는 팝업 이름: %s"), *Args[0]);
			LogUsage();
			return;
		}

		ALastFPSPlayerController* PC =
			Cast<ALastFPSPlayerController>(World->GetFirstPlayerController());
		if (!PC)
		{
			UE_LOG(LogLastFPSPopupDebug, Warning,
				TEXT("LastFPSPlayerController 를 찾지 못했습니다. 팝업은 로컬 플레이어가 있어야 표시됩니다."));
			return;
		}

		// 첫 인자는 팝업 이름이므로 핸들러에는 나머지만 넘긴다.
		TArray<FString> HandlerArgs(Args);
		HandlerArgs.RemoveAt(0);
		Entry->Handler(*PC, HandlerArgs);
	}

	FAutoConsoleCommandWithWorldAndArgs PopupCommand(
		TEXT("LastFPS.UI.Popup"),
		TEXT("개발용 팝업 표시. 인자 없이 실행하면 사용 가능한 이름을 출력한다."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandlePopupCommand));
}

#endif // !UE_BUILD_SHIPPING
