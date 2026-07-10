#include "UI/Inventory/LastFPSWeaponPreviewRig.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"

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
	// 오프스크린에서도 포즈 갱신 — 없으면 첫 프레임에 무기가 T-포즈로 보일 수 있다.
	WeaponMeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	
	PreviewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PreviewCamera"));
	PreviewCamera->SetupAttachment(Pivot);
}

void ALastFPSWeaponPreviewRig::BeginPlay()
{
	Super::BeginPlay();
}

void ALastFPSWeaponPreviewRig::InitPreview(USkeletalMesh* WeaponMesh)
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
}

void ALastFPSWeaponPreviewRig::AddYaw(float DeltaDegrees)
{
	if (Pivot)
	{
		Pivot->AddLocalRotation(FRotator(0.f, DeltaDegrees, 0.f));
	}
}
