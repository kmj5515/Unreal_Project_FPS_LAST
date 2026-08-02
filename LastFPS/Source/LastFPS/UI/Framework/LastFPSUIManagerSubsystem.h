#pragma once

#include "GameUIManagerSubsystem.h"
#include "GameplayTagContainer.h"
#include "UI/Framework/LastFPSScreenTabHost.h"
#include "LastFPSUIManagerSubsystem.generated.h"

class UCommonActivatableWidget;
class ULastFPSScreenRegistry;
class APlayerController;

/**
 * LastFPS UI manager — UGameUIPolicy로 로컬 플레이어별 PrimaryGameLayout 생성 +
 * 태그 기반 화면 라우팅(OpenScreen) 제공.
 *
 * 콘텐츠 추가 = ScreenRegistry에 행 1개. 호출부(NPC/버튼/GameMode)는 OpenScreen(Tag)만.
 */
UCLASS()
class LASTFPS_API ULastFPSUIManagerSubsystem : public UGameUIManagerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** WorldContext로 인스턴스 획득 (BP/C++ 공용) */
	static ULastFPSUIManagerSubsystem* Get(const UObject* WorldContext);

	/**
	 * 화면 태그에 해당하는 위젯을 레지스트리에서 찾아 해당 레이어에 push.
	 * 이미 열려 있으면 기존 위젯 반환. OwningPlayer 미지정 시 첫 로컬 PC 사용.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
	UCommonActivatableWidget* OpenScreen(FGameplayTag ScreenTag, APlayerController* OwningPlayer = nullptr);

	/**
	 * 화면을 열면서 생성 직후 초기화까지 넘긴다.
	 *
	 * 표시할 내용을 밖에서 받아야 하는 화면(무기 상세 프리뷰 등)이 레지스트리를 우회하지 않게 하려는 것이다.
	 * 우회하면 레이어·위젯 클래스·프리뷰 시점 같은 표시 규칙을 그 화면만 코드에 따로 들고 있게 된다.
	 */
	UCommonActivatableWidget* OpenScreenWithInit(
		FGameplayTag ScreenTag,
		APlayerController* OwningPlayer,
		TFunction<void(UCommonActivatableWidget&)> InitFunc);

	/** 열려 있던 화면을 닫는다(pop). */
	UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
	void CloseScreen(FGameplayTag ScreenTag);

	UFUNCTION(BlueprintPure, Category="LastFPS|UI")
	bool IsScreenOpen(FGameplayTag ScreenTag) const;


	/**
	 * 탭 shell 을 등록한다. 등록된 동안 shell 이 들고 있는 화면 태그는 레이어 push 대신 shell 안에서 전환된다.
	 * 호출부(NPC·퀘스트·버튼)는 계속 OpenScreen 만 부르면 되므로 진입 경로가 하나로 유지된다.
	 * shell 은 자신이 사라질 때 반드시 Unregister 해야 한다.
	 */
	void RegisterScreenTabHost(TScriptInterface<ILastFPSScreenTabHost> InTabHost);
	void UnregisterScreenTabHost(const TScriptInterface<ILastFPSScreenTabHost>& InTabHost);

	/**
	 * 화면 정의 조회용. shell 이 탭 콘텐츠를 직접 만들 때 필요하다.
	 * 레지스트리를 어디서 가져오는지(GameDataSubsystem)는 여전히 이 클래스만 안다.
	 */
	const ULastFPSScreenRegistry* GetScreenRegistry() { return GetRegistry(); }

	/**
	 * 껍데기가 생기기 전에 들어온 탭 요청을 가져가고 비운다.
	 *
	 * 껍데기는 탭 목록을 다 만든 뒤에 호출해야 한다. 그래야 요청받은 탭을 첫 탭으로 삼아
	 * 쓰지도 않을 기본 탭을 만들지 않는다(탭 하나가 화면 위젯 클래스와 그 참조 에셋을 통째로 끌고 온다).
	 */
	FGameplayTag ConsumePendingTabScreenTag();

private:
	const ULastFPSScreenRegistry* GetRegistry();

	UPROPERTY(Transient)
	TObjectPtr<ULastFPSScreenRegistry> CachedRegistry;

	/** 현재 열려 있는 화면 추적 (중복 오픈 방지 / 닫기) */
	UPROPERTY(Transient)
	TMap<FGameplayTag, TWeakObjectPtr<UCommonActivatableWidget>> OpenScreens;

	/**
	 * 등록된 탭 shell. 아웃게임에 하나만 존재하므로 목록이 아니라 단일 참조로 둔다.
	 * shell 이 파괴돼도 매니저가 살아 있을 수 있어 약참조로 잡는다.
	 */
	TWeakObjectPtr<UObject> ScreenTabHost;

	/**
	 * 껍데기가 생기기 전에 들어온 탭 요청. 호스트가 등록되는 순간 한 번 적용하고 비운다.
	 * 껍데기 생성이 같은 프레임에 끝나지 않을 수 있어 호출부가 재시도하지 않아도 되게 여기서 들고 있는다.
	 */
	FGameplayTag PendingTabScreenTag;
};
