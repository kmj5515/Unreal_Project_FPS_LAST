#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "LastFPSGasInspectorSubsystem.generated.h"

class SLastFPSGasInspectorPanel;
class APlayerController;
class ULocalPlayer;

/**
 * 런타임 GAS 인스펙터 컨트롤러(개발 빌드 전용).
 *
 * 콘솔 명령 "LastFPS.GasInspector" 로 오버레이를 켜고, 마우스 커서로 캐릭터를
 * 스포이드 클릭하면 그 대상과 플레이어의 GAS 상태를 나란히 보여준다.
 *
 * 책임은 "입력→픽→갱신"의 조율뿐이다. GAS 읽기는 SnapshotBuilder, 표시는
 * SLastFPSGasInspectorPanel 이 담당한다(단일 책임/낮은 결합도).
 * 열려 있는 동안에만 틱한다.
 */
UCLASS()
class LASTFPS_API ULastFPSGasInspectorSubsystem :
	public ULocalPlayerSubsystem,
	public FTickableGameObject
{
	GENERATED_BODY()

public:
	// 셰이핑(배포) 빌드에서는 생성하지 않아 디버그 코드가 남지 않게 한다.
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;

	// 오버레이 열림/닫힘 토글. 콘솔 명령이 호출한다.
	void Toggle();
	bool IsOpen() const { return bOpen; }

	// ── FTickableGameObject ──────────────────────────────────────────
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return bOpen; }
	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Conditional;
	}
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual TStatId GetStatId() const override;

private:
	void Open();
	void Close();
	// "다른 캐릭터 지정" 버튼/최초 진입 시 다음 클릭을 픽 대상으로 무장한다.
	void ArmPick();
	void TryPickTargetUnderCursor();
	void RefreshPanel();
	APlayerController* GetOwningPlayerController() const;

	TSharedPtr<SLastFPSGasInspectorPanel> InspectorPanel;
	TWeakObjectPtr<AActor> PickedTarget;

	bool bOpen = false;
	// 무장 상태에서만 월드 클릭이 새 대상을 채택한다(실수 재선택 방지).
	bool bPickArmed = false;
	bool bPreviousShowMouseCursor = false;
	float RefreshAccumulator = 0.f;
};
