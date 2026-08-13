#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/Tables/LastFPSQuestData.h"
#include "Engine/DataTable.h"
#include "UI/Common/LastFPSNoticeWidget.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLastFPSRewardDialogueAfterPopupTest,
	"LastFPS.UI.RewardPresentation.DialogueAfterPopupDeactivated",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FLastFPSRewardDialogueAfterPopupTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<ULastFPSNoticeWidget> Notice(
		NewObject<ULastFPSNoticeWidget>(GetTransientPackage()));
	if (!TestNotNull(TEXT("보상 공지 위젯 생성"), Notice.Get()))
	{
		return false;
	}

	bool bDialogueStarted = false;
	Notice->OnDeactivated().AddLambda(
		[&bDialogueStarted]()
		{
			bDialogueStarted = true;
		});

	Notice->ActivateWidget();
	TestFalse(
		TEXT("보상 팝업이 활성 상태인 동안에는 후속 대사가 시작되지 않는다"),
		bDialogueStarted);

	Notice->DeactivateWidget();
	TestTrue(
		TEXT("보상 팝업 비활성화가 끝난 뒤 후속 대사가 시작된다"),
		bDialogueStarted);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLastFPSFinalObjectiveRadioDeferredTest,
	"LastFPS.UI.RewardPresentation.FinalObjectiveRadioDeferred",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FLastFPSFinalObjectiveRadioDeferredTest::RunTest(const FString& Parameters)
{
	const UDataTable* QuestTable = LoadObject<UDataTable>(
		nullptr,
		TEXT("/Game/Data/Tables/Hub/DT_QuestData.DT_QuestData"));
	if (!TestNotNull(TEXT("퀘스트 데이터 테이블 로드"), QuestTable))
	{
		return false;
	}

	const FLastFPSQuestData* ReachQuest = QuestTable->FindRow<FLastFPSQuestData>(
		TEXT("Quest_Reach"),
		TEXT("RewardPresentationAutomationTest"));
	if (!TestNotNull(TEXT("방어선 정찰 퀘스트 행 조회"), ReachQuest))
	{
		return false;
	}

	if (!TestTrue(TEXT("방어선 정찰에 목표가 존재한다"), !ReachQuest->Objectives.IsEmpty()))
	{
		return false;
	}

	TestTrue(
		TEXT("마지막 목표에는 보상 팝업보다 먼저 재생되는 완료 무전이 없다"),
		ReachQuest->Objectives.Last().RadioOnComplete.IsEmpty());
	TestTrue(
		TEXT("콜의 완료 대사는 보상 팝업 종료 후 재생되는 퀘스트 완료 무전이다"),
		ReachQuest->RadioOnComplete.Contains(FName(TEXT("Radio_Reach_Clear"))));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLastFPSMainStoryMaintenanceMarkChainTest,
	"LastFPS.Quest.Story.MainChainContinuesToMaintenanceMark",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FLastFPSMainStoryMaintenanceMarkChainTest::RunTest(const FString& Parameters)
{
	const UDataTable* QuestTable = LoadObject<UDataTable>(
		nullptr,
		TEXT("/Game/Data/Tables/Hub/DT_QuestData.DT_QuestData"));
	if (!TestNotNull(TEXT("퀘스트 데이터 테이블을 불러온다"), QuestTable))
	{
		return false;
	}

	const FLastFPSQuestData* ReportQuest = QuestTable->FindRow<FLastFPSQuestData>(
		TEXT("Quest_Talk"),
		TEXT("MainStoryMaintenanceMarkChainTest"));
	const FLastFPSQuestData* MarkQuest = QuestTable->FindRow<FLastFPSQuestData>(
		TEXT("Q_Mark_Trace"),
		TEXT("MainStoryMaintenanceMarkChainTest"));
	if (!TestNotNull(TEXT("실종 기체 보고 퀘스트가 존재한다"), ReportQuest)
		|| !TestNotNull(TEXT("정비 각인 추적 퀘스트가 존재한다"), MarkQuest))
	{
		return false;
	}

	TestEqual(
		TEXT("실종 기체 보고 다음에는 정비 각인 추적이 이어진다"),
		ReportQuest->NextQuestId,
		FName(TEXT("Q_Mark_Trace")));
	TestEqual(
		TEXT("정비 각인 추적의 선행 퀘스트가 실종 기체 보고다"),
		MarkQuest->PrereqQuestId,
		FName(TEXT("Quest_Talk")));
	if (!TestTrue(TEXT("정비 각인 추적에 NPC 대화 목표가 존재한다"), !MarkQuest->Objectives.IsEmpty()))
	{
		return false;
	}

	TestEqual(
		TEXT("정비 각인 확인 대상은 보로스다"),
		MarkQuest->Objectives[0].TargetId,
		FName(TEXT("NPC_Mechanic")));
	TestTrue(
		TEXT("보로스와 대화한 뒤 정비 각인 단서 라디오가 재생된다"),
		MarkQuest->RadioOnComplete.Contains(FName(TEXT("Radio_Mark_Trace_Clear"))));

	return true;
}

#endif
