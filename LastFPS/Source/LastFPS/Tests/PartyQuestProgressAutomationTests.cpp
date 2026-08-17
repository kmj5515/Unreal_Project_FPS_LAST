#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Game/LastFPSGameStateBase.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLastFPSPartyQuestProgressMergeTest,
	"LastFPS.Quest.Party.ProgressMerge",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FLastFPSPartyQuestProgressMergeTest::RunTest(const FString& Parameters)
{
	using FShared = FLastFPSSharedQuestProgress;

	// 처치처럼 파티원 기여를 합산하는 목표.
	TestEqual(
		TEXT("누적은 파티원 기여를 더한다"),
		FShared::MergeProgress(3, 2, /*bAbsolute=*/false, 10),
		5);
	TestEqual(
		TEXT("누적은 요구량을 넘지 않는다"),
		FShared::MergeProgress(9, 5, /*bAbsolute=*/false, 10),
		10);

	// 도달·인카운터처럼 한 명이 달성하면 확정되는 사실형 목표.
	TestEqual(
		TEXT("절대값은 관측값으로 승격한다"),
		FShared::MergeProgress(0, 1, /*bAbsolute=*/true, 1),
		1);
	TestEqual(
		TEXT("절대값이 낮아도 진행은 되돌아가지 않는다"),
		FShared::MergeProgress(4, 1, /*bAbsolute=*/true, 10),
		4);

	// 인카운터 완료 통지가 보내는 MAX_int32 는 요구량으로 잘려야 한다(넘침 없이).
	TestEqual(
		TEXT("완료 통지의 MAX_int32 는 요구량으로 제한된다"),
		FShared::MergeProgress(2, MAX_int32, /*bAbsolute=*/true, 7),
		7);
	TestEqual(
		TEXT("누적 경로에서도 넘침 없이 요구량으로 제한된다"),
		FShared::MergeProgress(MAX_int32, MAX_int32, /*bAbsolute=*/false, 7),
		7);

	// 방어적 입력.
	TestEqual(
		TEXT("음수 기여는 진행을 깎지 않는다"),
		FShared::MergeProgress(5, -3, /*bAbsolute=*/false, 10),
		5);
	TestEqual(
		TEXT("요구량이 0 이면 진행도 0"),
		FShared::MergeProgress(0, 4, /*bAbsolute=*/true, 0),
		0);

	return true;
}

#endif
