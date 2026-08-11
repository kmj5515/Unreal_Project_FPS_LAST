#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Data/Tables/LastFPSQuestData.h"
#include "Quest/LastFPSQuestSubsystem.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSQuestDebug, Log, All);

/**
 * 개발용 퀘스트 상태 덤프.
 *
 * 체인이 어디서 멈췄는지는 "지금 각 퀘스트가 어떤 상태인가"를 봐야 판단할 수 있다.
 * 진행 정지의 원인은 대개 셋 중 하나다.
 *  - Locked: 선행 퀘스트가 아직 Claimed 가 아님
 *  - Completed 로 머무름: 보상 팝업이 닫히지 않아 수령 연쇄가 재개되지 않음
 *  - NotStarted + QuestGiverNPC: 해당 NPC 와 대화해야 수락됨(정상)
 */
namespace
{
	const TCHAR* StatusToString(const ELastFPSQuestStatus Status)
	{
		switch (Status)
		{
		case ELastFPSQuestStatus::NotStarted: return TEXT("NotStarted");
		case ELastFPSQuestStatus::InProgress: return TEXT("InProgress");
		case ELastFPSQuestStatus::Completed:  return TEXT("Completed");
		case ELastFPSQuestStatus::Claimed:    return TEXT("Claimed");
		case ELastFPSQuestStatus::Locked:     return TEXT("Locked");
		}
		return TEXT("?");
	}

	void HandleQuestDumpCommand(const TArray<FString>& Args, UWorld* World)
	{
		ULastFPSQuestSubsystem* Quests = ULastFPSQuestSubsystem::Get(World);
		if (!Quests)
		{
			UE_LOG(LogLastFPSQuestDebug, Warning,
				TEXT("QuestSubsystem 을 찾지 못했습니다. 플레이 중에 실행하십시오."));
			return;
		}

		const UDataTable* Table = Quests->GetQuestTable();
		if (!Table)
		{
			UE_LOG(LogLastFPSQuestDebug, Warning, TEXT("DT_QuestData 를 찾지 못했습니다."));
			return;
		}

		// 인자를 주면 그 문자열이 들어간 퀘스트만 출력한다(행이 많을 때 눈으로 찾기 위함).
		const FString Filter = Args.Num() > 0 ? Args[0] : FString();

		UE_LOG(LogLastFPSQuestDebug, Log, TEXT("── 퀘스트 상태 덤프 ──────────────────────"));

		static const FString Ctx(TEXT("LastFPS.Quest.Dump"));
		Table->ForeachRow<FLastFPSQuestData>(Ctx,
			[Quests, &Filter](const FName& RowName, const FLastFPSQuestData& Row)
			{
				if (!Filter.IsEmpty() && !RowName.ToString().Contains(Filter))
				{
					return;
				}

				const ELastFPSQuestStatus Status = Quests->GetStatus(RowName);

				// 선행 퀘스트가 왜 안 열렸는지 바로 보이도록 선행 상태를 함께 찍는다.
				FString PrereqInfo(TEXT("-"));
				if (!Row.PrereqQuestId.IsNone())
				{
					PrereqInfo = FString::Printf(TEXT("%s(%s)"),
						*Row.PrereqQuestId.ToString(),
						StatusToString(Quests->GetStatus(Row.PrereqQuestId)));
				}

				FString Progress;
				for (int32 Index = 0; Index < Row.Objectives.Num(); ++Index)
				{
					Progress += FString::Printf(TEXT("%s%d/%d"),
						Index > 0 ? TEXT(" ") : TEXT(""),
						Quests->GetObjectiveProgress(RowName, Index),
						Quests->GetObjectiveRequiredCount(RowName, Index));
				}

				UE_LOG(LogLastFPSQuestDebug, Log,
					TEXT("%-16s %-11s %-6s prereq=%-22s giver=%-18s autoClaim=%d  목표[%s]"),
					*RowName.ToString(),
					StatusToString(Status),
					Quests->IsQuestTracked(RowName) ? TEXT("추적중") : TEXT(""),
					*PrereqInfo,
					*Row.QuestGiverNPC.ToString(),
					Row.bAutoClaim ? 1 : 0,
					*Progress);
			});

		UE_LOG(LogLastFPSQuestDebug, Log, TEXT("──────────────────────────────────────────"));
	}

	void HandleQuestUnlockCommand(const TArray<FString>& Args, UWorld* World)
	{
		ULastFPSQuestSubsystem* Quests = ULastFPSQuestSubsystem::Get(World);
		if (!Quests)
		{
			UE_LOG(LogLastFPSQuestDebug, Warning,
				TEXT("QuestSubsystem 을 찾지 못했습니다. 플레이 중에 실행하십시오."));
			return;
		}

		if (Args.Num() < 1)
		{
			UE_LOG(LogLastFPSQuestDebug, Warning,
				TEXT("사용법: LastFPS.Quest.UnlockTo <QuestId>   (예: LastFPS.Quest.UnlockTo Q_Dungeon_Gate)"));
			return;
		}

		Quests->DebugUnlockChainTo(FName(*Args[0]));
	}

	FAutoConsoleCommandWithWorldAndArgs QuestUnlockCommand(
		TEXT("LastFPS.Quest.UnlockTo"),
		TEXT("대상 퀘스트의 선행 사슬을 전부 수령 완료로 만들어 대상을 수락 가능하게 한다."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleQuestUnlockCommand));

	FAutoConsoleCommandWithWorldAndArgs QuestDumpCommand(
		TEXT("LastFPS.Quest.Dump"),
		TEXT("퀘스트 런타임 상태를 출력한다. 인자로 이름 일부를 주면 필터링한다."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleQuestDumpCommand));
}

#endif // !UE_BUILD_SHIPPING
