#include "UI/Inventory/LastFPSWeaponPreviewRig.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
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

	// 카메라는 무기를 -X 쪽에서 바라본다(+X 축을 향해 촬영).
	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	Capture->SetupAttachment(RootScene);
	Capture->SetRelativeLocationAndRotation(FVector(-260.f, 0.f, 40.f), FRotator(-6.f, 0.f, 0.f));
	Capture->FOVAngle = 35.f;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList; // 허브 배경 제외, 무기만
	Capture->bCaptureEveryFrame = true;
	Capture->bCaptureOnMovement = false;

	// show-only 리스트는 월드 조명을 배제하므로 리그 자체 조명을 둔다.
	KeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(RootScene);
	KeyLight->SetRelativeLocation(FVector(-120.f, 120.f, 160.f));
	KeyLight->SetIntensity(60000.f);
	KeyLight->SetAttenuationRadius(1500.f);

	FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(RootScene);
	FillLight->SetRelativeLocation(FVector(-120.f, -140.f, 80.f));
	FillLight->SetIntensity(30000.f);
	FillLight->SetAttenuationRadius(1500.f);
}

void ALastFPSWeaponPreviewRig::BeginPlay()
{
	Super::BeginPlay();

	if (Capture)
	{
		Capture->ShowOnlyActors.AddUnique(this);
	}
}

UTextureRenderTarget2D* ALastFPSWeaponPreviewRig::InitPreview(USkeletalMesh* WeaponMesh)
{
	if (WeaponMeshComp && WeaponMesh)
	{
		WeaponMeshComp->SetSkeletalMeshAsset(WeaponMesh);
	}

	if (!RenderTarget)
	{
		RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, RenderTargetSize, RenderTargetSize);
	}

	if (Capture)
	{
		Capture->ShowOnlyActors.AddUnique(this);
		Capture->TextureTarget = RenderTarget;
	}

	return RenderTarget;
}

void ALastFPSWeaponPreviewRig::AddYaw(float DeltaDegrees)
{
	if (Pivot)
	{
		Pivot->AddLocalRotation(FRotator(0.f, DeltaDegrees, 0.f));
	}
}
