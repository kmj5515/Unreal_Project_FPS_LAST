#include "UI/Inventory/LastFPSWeaponPreviewRig.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/KismetRenderingLibrary.h"

ALastFPSWeaponPreviewRig::ALastFPSWeaponPreviewRig()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	// 무기는 피벗의 자식 — 피벗을 돌리면 무기가 제자리에서 회전한다.
	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	Pivot->SetupAttachment(RootScene);

	WeaponMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMeshComp->SetupAttachment(Pivot);
	WeaponMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 오프스크린에서도 포즈 갱신 — 없으면 첫 캡처에 무기가 안 보임.
	WeaponMeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// 카메라는 무기를 -X 쪽에서 바라본다(+X 축을 향해 촬영).
	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	Capture->SetupAttachment(RootScene);
	Capture->SetRelativeLocationAndRotation(FVector(-260.f, 0.f, 40.f), FRotator(-6.f, 0.f, 0.f));
	Capture->FOVAngle = 35.f;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList; // 허브 배경 제외, 무기만
	// 온디맨드 캡처 — 초기화·회전 시에만 찍는다.
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->bAlwaysPersistRenderingState = true;

	// 노출 고정 — 안 하면 첫 프레임 빈 씬에서 자동노출이 화면을 하얗게 날린다.
	Capture->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	Capture->PostProcessSettings.AutoExposureMinBrightness = 1.f;
	Capture->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
	Capture->PostProcessSettings.AutoExposureMaxBrightness = 1.f;
}

void ALastFPSWeaponPreviewRig::BeginPlay()
{
	Super::BeginPlay();
}

UTextureRenderTarget2D* ALastFPSWeaponPreviewRig::InitPreview(USkeletalMesh* WeaponMesh)
{
	if (WeaponMeshComp && WeaponMesh)
	{
		WeaponMeshComp->SetSkeletalMeshAsset(WeaponMesh);
		WeaponMeshComp->SetForcedLOD(1);
	}

	if (Pivot)
	{
		Pivot->SetRelativeRotation(FRotator::ZeroRotator);
	}

	if (!RenderTarget)
	{
		RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, RenderTargetSize, RenderTargetSize);
	}

	if (Capture)
	{
		Capture->TextureTarget = RenderTarget;
	}

	RefreshCapture();
	// 첫 초기화 땐 메시 프록시가 아직이라 show-only에 안 잡힐 수 있어 다음 틱에 한 번 더 잡는다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ALastFPSWeaponPreviewRig::RefreshCapture);
	}

	return RenderTarget;
}

void ALastFPSWeaponPreviewRig::RefreshCapture()
{
	if (WeaponMeshComp)
	{
		WeaponMeshComp->SetVisibility(true, true);
		WeaponMeshComp->MarkRenderStateDirty();
	}

	if (Capture)
	{
		Capture->ShowOnlyActors.Empty();
		Capture->ShowOnlyComponents.Empty();
		if (WeaponMeshComp)
		{
			Capture->ShowOnlyComponent(WeaponMeshComp);
		}
		// BP에 붙인 스태틱 메시 소품도 함께 캡처.
		TInlineComponentArray<UStaticMeshComponent*> Props(this);
		for (UStaticMeshComponent* Prop : Props)
		{
			Capture->ShowOnlyComponent(Prop);
		}
		Capture->CaptureScene();
	}
}

void ALastFPSWeaponPreviewRig::AddYaw(float DeltaDegrees)
{
	if (Pivot)
	{
		Pivot->AddLocalRotation(FRotator(0.f, DeltaDegrees, 0.f));
	}

	if (Capture)
	{
		Capture->CaptureScene();
	}
}
