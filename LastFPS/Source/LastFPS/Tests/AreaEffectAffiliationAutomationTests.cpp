#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystem/Actors/LastFPSAreaEffectActor.h"
#include "Game/LastFPSPlayerController.h"
#include "Utility/LastFPSCombatAffiliation.h"
#include "Utility/LastFPSTeamTypes.h"
#include "UObject/StrongObjectPtr.h"

/**
 * 영역 스킬이 아군 플레이어를 대상으로 잡지 않는지 검증한다.
 * 오버랩 수집은 진영을 구분하지 않으므로, 진영 계약(AreFriendlyActors)과
 * 기본 제외 설정이 함께 유지되어야만 멀티에서 아군이 스킬에 맞지 않는다.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLastFPSAreaEffectAffiliationTest,
	"LastFPS.AbilitySystem.AreaEffect.FriendlyTargetExclusion",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FLastFPSAreaEffectAffiliationTest::RunTest(const FString&)
{
	// 기본값이 꺼지면 데이터 저작 실수 하나로 전 스킬이 아군을 때린다.
	TestTrue(
		TEXT("영역 효과 기본 설정: 아군 제외"),
		FLastFPSAreaEffectConfig().bIgnoreFriendlyTargets);

	TStrongObjectPtr<ALastFPSPlayerController> Caster(
		NewObject<ALastFPSPlayerController>(GetTransientPackage()));
	TStrongObjectPtr<ALastFPSPlayerController> Teammate(
		NewObject<ALastFPSPlayerController>(GetTransientPackage()));
	TStrongObjectPtr<ALastFPSPlayerController> Enemy(
		NewObject<ALastFPSPlayerController>(GetTransientPackage()));
	Enemy->SetGenericTeamId(FGenericTeamId(static_cast<uint8>(ELastFPSTeam::Enemy)));

	// 두 번째 플레이어가 들어온 상황: 같은 Player 진영이라 대상에서 빠져야 한다.
	TestTrue(
		TEXT("플레이어끼리는 아군"),
		LastFPSCombatAffiliation::AreFriendlyActors(Caster.Get(), Teammate.Get()));

	TestFalse(
		TEXT("다른 진영은 아군이 아님"),
		LastFPSCombatAffiliation::AreFriendlyActors(Caster.Get(), Enemy.Get()));

	return true;
}

#endif
