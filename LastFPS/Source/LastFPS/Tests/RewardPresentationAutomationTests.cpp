#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/Common/LastFPSNoticeWidget.h"
#include "UI/HUD/Audio/LastFPSRadioAudioSettings.h"
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

	const ULastFPSRadioAudioSettings* RadioSettings =
		ULastFPSRadioAudioSettings::Get();
	if (!TestNotNull(TEXT("무전 자막 설정 조회"), RadioSettings))
	{
		return false;
	}

	TestEqual(
		TEXT("대사 타이핑 속도 전역 배율"),
		RadioSettings->TypingSpeedScale,
		0.75f,
		KINDA_SMALL_NUMBER);

	return true;
}

#endif
