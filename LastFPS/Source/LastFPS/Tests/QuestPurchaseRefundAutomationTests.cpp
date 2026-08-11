#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/Tables/LastFPSQuestData.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLastFPSQuestPurchaseRefundCalculationTest,
	"LastFPS.Quest.Reward.PurchaseRefundCalculation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FLastFPSQuestPurchaseRefundCalculationTest::RunTest(const FString& Parameters)
{
	FLastFPSQuestPurchaseRefund Refund;

	TestEqual(TEXT("환급 비활성 상태는 실결제액이 있어도 0"), Refund.CalculateCredits(1800), 0);

	Refund.Rate = 1.f;
	TestEqual(TEXT("100% 환급은 실결제액과 동일"), Refund.CalculateCredits(1800), 1800);

	Refund.Rate = 0.5f;
	TestEqual(TEXT("부분 환급은 비율을 반올림 적용"), Refund.CalculateCredits(901), 451);

	Refund.Rate = 1.f;
	Refund.MaxCredits = 1000;
	TestEqual(TEXT("환급 상한을 초과하지 않음"), Refund.CalculateCredits(1800), 1000);
	TestEqual(TEXT("0 이하 실결제액은 환급하지 않음"), Refund.CalculateCredits(0), 0);

	Refund.MaxCredits = 0;
	Refund.Rate = 2.f;
	TestEqual(TEXT("잘못된 비율도 100%로 제한"), Refund.CalculateCredits(1800), 1800);

	return true;
}

#endif
