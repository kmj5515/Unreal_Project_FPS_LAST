#include "Debug/GasInspector/LastFPSGasInspectorSubsystem.h"

#include "Debug/GasInspector/LastFPSGasSnapshot.h"
#include "Debug/GasInspector/LastFPSGasSnapshotBuilder.h"
#include "Debug/GasInspector/SLastFPSGasInspectorPanel.h"

#include "AbilitySystemGlobals.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "LastFPSGasInspector"

namespace
{
	// 다른 뷰포트 위젯 위에 오도록 충분히 큰 ZOrder.
	constexpr int32 GasInspectorZOrder = 1000;
	// 값 갱신 주기(초). 매 프레임 위젯 트리를 다시 만들지 않기 위해 완만하게 둔다.
	constexpr float GasInspectorRefreshInterval = 0.15f;
}

bool ULastFPSGasInspectorSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return Super::ShouldCreateSubsystem(Outer);
#endif
}

void ULastFPSGasInspectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

#if !UE_BUILD_SHIPPING
	// 콘솔/키 바인딩에서 호출할 토글 명령을 등록한다.
	ToggleCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("LastFPS.GasInspector"),
		TEXT("GAS 런타임 인스펙터 열기/닫기 토글"),
		FConsoleCommandDelegate::CreateUObject(this, &ULastFPSGasInspectorSubsystem::Toggle),
		ECVF_Default);
#endif
}

void ULastFPSGasInspectorSubsystem::Deinitialize()
{
	Close();

	if (ToggleCommand)
	{
		IConsoleManager::Get().UnregisterConsoleObject(ToggleCommand);
		ToggleCommand = nullptr;
	}

	Super::Deinitialize();
}

void ULastFPSGasInspectorSubsystem::Toggle()
{
	if (bOpen)
	{
		Close();
	}
	else
	{
		Open();
	}
}

void ULastFPSGasInspectorSubsystem::Open()
{
	if (bOpen || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController)
	{
		return;
	}

	if (!InspectorPanel.IsValid())
	{
		InspectorPanel = SNew(SLastFPSGasInspectorPanel)
			.OnRequestPick(FSimpleDelegate::CreateUObject(this, &ULastFPSGasInspectorSubsystem::ArmPick));
	}

	GEngine->GameViewport->AddViewportWidgetContent(InspectorPanel.ToSharedRef(), GasInspectorZOrder);

	// 커서를 보이게 해 스포이드 픽을 가능하게 하고, 종료 시 원복하도록 이전 상태를 저장한다.
	bPreviousShowMouseCursor = PlayerController->bShowMouseCursor;
	PlayerController->bShowMouseCursor = true;
	PlayerController->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));

	bOpen = true;
	bPickArmed = true; // 진입 즉시 첫 대상을 찍을 수 있게 무장.
	RefreshAccumulator = 0.f;
	RefreshPanel();
}

void ULastFPSGasInspectorSubsystem::ArmPick()
{
	if (!bOpen)
	{
		return;
	}

	bPickArmed = true;
	RefreshPanel();
}

void ULastFPSGasInspectorSubsystem::Close()
{
	if (!bOpen)
	{
		return;
	}

	if (GEngine && GEngine->GameViewport && InspectorPanel.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(InspectorPanel.ToSharedRef());
	}

	if (APlayerController* PlayerController = GetOwningPlayerController())
	{
		PlayerController->bShowMouseCursor = bPreviousShowMouseCursor;
		PlayerController->SetInputMode(FInputModeGameOnly());
	}

	InspectorPanel.Reset();
	PickedTarget.Reset();
	bOpen = false;
	bPickArmed = false;
}

void ULastFPSGasInspectorSubsystem::Tick(float DeltaTime)
{
	if (!bOpen)
	{
		return;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController)
	{
		// 컨트롤러가 사라졌으면(레벨 전환 등) 안전하게 닫는다.
		Close();
		return;
	}

	// 무장 상태에서만 클릭으로 대상을 채택한다.
	if (bPickArmed && PlayerController->WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		TryPickTargetUnderCursor();
	}

	RefreshAccumulator += DeltaTime;
	if (RefreshAccumulator >= GasInspectorRefreshInterval)
	{
		RefreshAccumulator = 0.f;
		RefreshPanel();
	}
}

void ULastFPSGasInspectorSubsystem::TryPickTargetUnderCursor()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController)
	{
		return;
	}

	// 커서 아래를 Pawn 채널로 트레이스해 GAS를 가진 액터만 대상으로 채택한다.
	FHitResult HitResult;
	if (PlayerController->GetHitResultUnderCursor(ECC_Pawn, false, HitResult))
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor && UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor))
		{
			PickedTarget = HitActor;
			bPickArmed = false; // 대상 채택 성공 → 무장 해제(다시 지정 버튼으로 재무장).
		}
	}

	RefreshPanel();
}

void ULastFPSGasInspectorSubsystem::RefreshPanel()
{
	if (!InspectorPanel.IsValid())
	{
		return;
	}

	FLastFPSGasSnapshot PlayerSnapshot;
	if (const APlayerController* PlayerController = GetOwningPlayerController())
	{
		if (const APawn* PlayerPawn = PlayerController->GetPawn())
		{
			PlayerSnapshot = FLastFPSGasSnapshotBuilder::BuildFromActor(PlayerPawn);
		}
	}

	FLastFPSGasSnapshot TargetSnapshot;
	if (PickedTarget.IsValid())
	{
		TargetSnapshot = FLastFPSGasSnapshotBuilder::BuildFromActor(PickedTarget.Get());
	}

	InspectorPanel->RefreshSnapshots(PlayerSnapshot, TargetSnapshot);
	InspectorPanel->SetHintText(bPickArmed
		? LOCTEXT("HintArmed", "스포이드 무장됨 — 캐릭터를 클릭하세요")
		: LOCTEXT("HintIdle", "다른 캐릭터 지정 버튼을 눌러 대상을 바꾸세요"));
}

APlayerController* ULastFPSGasInspectorSubsystem::GetOwningPlayerController() const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetPlayerController(LocalPlayer->GetWorld()) : nullptr;
}

UWorld* ULastFPSGasInspectorSubsystem::GetTickableGameObjectWorld() const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetWorld() : nullptr;
}

TStatId ULastFPSGasInspectorSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULastFPSGasInspectorSubsystem, STATGROUP_Tickables);
}

#undef LOCTEXT_NAMESPACE
