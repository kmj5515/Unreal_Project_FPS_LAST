#include "WeaponIcons/LastFPSWeaponIconCaptureActor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetCompilingManager.h"
#include "Components/SceneComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "Data/Tables/LastFPSItemData.h"
#include "Engine/DataTable.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Misc/ScopedSlowTask.h"
#include "ObjectTools.h"
#include "RenderingThread.h"
#include "TextureResource.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSWeaponIconCapture, Log, All);

namespace
{
	constexpr float MinimumBoundsSize = 1.f;

	/** 근평면에 물리지 않도록 두는 최소 여유. 화면 여백은 FramingPadding 이 담당한다. */
	constexpr float NearPlaneSafetyMargin = 10.f;

	/** 촬영 텍스처 한 변의 상한. 슈퍼샘플 배수가 이를 넘기면 배수를 낮춘다. */
	constexpr int32 MaxCaptureDimension = 4096;

	/** 배경으로 볼 최대 깊이 차이. 피사체가 있는 픽셀은 배경 깊이에서 이보다 크게 벗어난다. */
	constexpr float MinDepthSeparation = 1.e-5f;

	/** 실루엣이 화면을 이만큼 넘게 덮으면 배경까지 찍힌 것으로 보고 실패로 끊는다. */
	constexpr float MaxSilhouetteCoverage = 0.9f;
}

ALastFPSWeaponIconCaptureActor::ALastFPSWeaponIconCaptureActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
	RootScene->SetMobility(EComponentMobility::Movable);

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(RootScene);
	SceneCapture->SetMobility(EComponentMobility::Movable);
	SceneCapture->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
	SceneCapture->ProjectionType = ECameraProjectionMode::Perspective;
	SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;
	SceneCapture->bAlwaysPersistRenderingState = true;
	SceneCapture->ShowFlags.SetAtmosphere(false);
	SceneCapture->ShowFlags.SetFog(false);
	SceneCapture->ShowFlags.SetVolumetricFog(false);
	SceneCapture->ShowFlags.SetMotionBlur(false);

	// 조명 기본값. 방향은 카메라 기준이라 촬영 각도를 바꿔도 3점 조명 구성이 유지된다.
	KeyLightSetup.Direction = FVector(-1.f, -1.2f, 0.9f);
	KeyLightSetup.DistanceScale = 3.f;
	KeyLightSetup.Color = FLinearColor(1.f, 0.92f, 0.82f);
	KeyLightSetup.Intensity = 5000.f;
	KeyLightSetup.bCastShadows = true;

	FillLightSetup.Direction = FVector(-0.8f, 1.4f, 0.2f);
	FillLightSetup.DistanceScale = 3.5f;
	FillLightSetup.Color = FLinearColor(0.72f, 0.82f, 1.f);
	FillLightSetup.Intensity = 1800.f;
	FillLightSetup.bCastShadows = false;

	RimLightSetup.Direction = FVector(1.2f, 0.3f, 0.8f);
	RimLightSetup.DistanceScale = 3.f;
	RimLightSetup.Color = FLinearColor::White;
	RimLightSetup.Intensity = 2800.f;
	RimLightSetup.bCastShadows = false;
}

void ALastFPSWeaponIconCaptureActor::CaptureAllWeaponIcons()
{
	LastCapturedCount = 0;
	if (!ItemTable)
	{
		UE_LOG(LogLastFPSWeaponIconCapture, Error, TEXT("무기 아이콘 촬영 실패: ItemTable이 지정되지 않았습니다."));
		return;
	}

	if (ItemTable->GetRowStruct() != FLastFPSItemData::StaticStruct())
	{
		UE_LOG(
			LogLastFPSWeaponIconCapture,
			Error,
			TEXT("무기 아이콘 촬영 실패: '%s'의 행 구조가 FLastFPSItemData가 아닙니다."),
			*GetNameSafe(ItemTable));
		return;
	}

	TArray<FName> RowNames = ItemTable->GetRowNames();
	RowNames.Sort(FNameLexicalLess());
	FScopedSlowTask SlowTask(RowNames.Num(), FText::FromString(TEXT("무기 슬롯 아이콘을 생성하는 중입니다.")));
	if (!IsRunningCommandlet())
	{
		SlowTask.MakeDialog(true);
	}

	int32 CapturedCount = 0;
	int32 SkippedCount = 0;
	for (const FName RowName : RowNames)
	{
		SlowTask.EnterProgressFrame(1.f, FText::FromName(RowName));
		if (SlowTask.ShouldCancel())
		{
			break;
		}

		FLastFPSItemData* Item = ItemTable->FindRow<FLastFPSItemData>(RowName, TEXT("무기 아이콘 일괄 촬영"));
		if (!Item || Item->ItemType != ELastFPSItemType::Weapon || Item->WeaponDefinition.IsNull())
		{
			++SkippedCount;
			continue;
		}

		ULastFPSWeaponDefinition* WeaponDefinition = Item->WeaponDefinition.LoadSynchronous();
		if (!WeaponDefinition || !WeaponDefinition->SkeletalMesh)
		{
			UE_LOG(
				LogLastFPSWeaponIconCapture,
				Warning,
				TEXT("무기 아이콘 촬영 건너뜀: 행='%s', 원인=WeaponDefinition 또는 SkeletalMesh가 유효하지 않습니다."),
				*RowName.ToString());
			++SkippedCount;
			continue;
		}

		const FString StableName = WeaponDefinition->WeaponId.IsNone()
			? RowName.ToString()
			: WeaponDefinition->WeaponId.ToString();
		const FString AssetName = ObjectTools::SanitizeObjectName(TEXT("T_WeaponSlot_") + StableName);
		UTexture2D* IconTexture = CaptureWeapon(AssetName, WeaponDefinition);
		if (!IconTexture)
		{
			++SkippedCount;
			continue;
		}

		if (bAssignToWeaponDefinition)
		{
			WeaponDefinition->Modify();
			WeaponDefinition->Icon = IconTexture;
			WeaponDefinition->MarkPackageDirty();
		}
		++CapturedCount;
	}

	LastCapturedCount = CapturedCount;
	UE_LOG(
		LogLastFPSWeaponIconCapture,
		Log,
		TEXT("무기 아이콘 촬영 완료: 성공=%d, 건너뜀=%d, 출력=%s"),
		CapturedCount,
		SkippedCount,
		*TextureOutputPath);

	// 커맨드렛은 종료 직전에 패키지를 저장하지만 에디터 버튼 경로는 더티 표시만 남는다.
	// 저장을 잊으면 다음 세션에 아이콘 연결이 사라지므로 명시적으로 알린다.
	if (!IsRunningCommandlet() && CapturedCount > 0)
	{
		UE_LOG(
			LogLastFPSWeaponIconCapture,
			Warning,
			TEXT("무기 아이콘 %d개가 아직 저장되지 않았습니다. 에디터에서 '모두 저장'을 실행하세요."),
			CapturedCount);
	}
}

void ALastFPSWeaponIconCaptureActor::ConfigureBatchCapture(
	UDataTable* InItemTable,
	const FString& InPngOutputDirectory)
{
	ItemTable = InItemTable;
	PngOutputDirectory = InPngOutputDirectory;
	bExportPng = true;
	bAssignToWeaponDefinition = true;
	bOverwriteExistingTextures = true;
}

void ALastFPSWeaponIconCaptureActor::CapturePreviewWeaponIcon()
{
	if (!PreviewWeapon || !PreviewWeapon->SkeletalMesh)
	{
		UE_LOG(LogLastFPSWeaponIconCapture, Error, TEXT("단일 무기 아이콘 촬영 실패: PreviewWeapon 또는 SkeletalMesh가 유효하지 않습니다."));
		return;
	}

	const FString StableName = PreviewWeapon->WeaponId.IsNone()
		? PreviewWeapon->GetName()
		: PreviewWeapon->WeaponId.ToString();
	const FString AssetName = ObjectTools::SanitizeObjectName(TEXT("T_WeaponSlot_") + StableName);
	if (UTexture2D* IconTexture = CaptureWeapon(AssetName, PreviewWeapon))
	{
		if (bAssignToWeaponDefinition)
		{
			PreviewWeapon->Modify();
			PreviewWeapon->Icon = IconTexture;
			PreviewWeapon->MarkPackageDirty();
		}

		UE_LOG(
			LogLastFPSWeaponIconCapture,
			Log,
			TEXT("단일 무기 아이콘 촬영 완료: 무기=%s, 아이콘=%s"),
			*GetNameSafe(PreviewWeapon),
			*IconTexture->GetPathName());
	}
}

bool ALastFPSWeaponIconCaptureActor::CreateCaptureRig()
{
	DestroyCaptureRig();

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogLastFPSWeaponIconCapture, Error, TEXT("무기 아이콘 캡처 리그 생성 실패: World가 없습니다."));
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.ObjectFlags = RF_Transient;
	CaptureRig = World->SpawnActor<ALastFPSWeaponCaptureRig>(
		ALastFPSWeaponCaptureRig::StaticClass(),
		GetActorTransform(),
		SpawnParameters);
	if (!CaptureRig)
	{
		UE_LOG(LogLastFPSWeaponIconCapture, Error, TEXT("무기 아이콘 캡처 리그를 생성하지 못했습니다."));
		return false;
	}

	// 조명 배치는 피사체 바운드를 알아야 정해지므로 PrepareWeapon 에서 수행한다.
	return true;
}

void ALastFPSWeaponIconCaptureActor::DestroyCaptureRig()
{
	if (!CaptureRig)
	{
		return;
	}

	if (UWorld* World = CaptureRig->GetWorld())
	{
		World->DestroyActor(CaptureRig);
	}
	CaptureRig = nullptr;
}

void ALastFPSWeaponIconCaptureActor::ApplyCaptureSettings()
{
	OutputWidth = FMath::Clamp(OutputWidth, 128, 4096);
	OutputHeight = FMath::Clamp(OutputHeight, 64, 2048);

	// 깊이 버퍼에는 안티에일리어싱이 없다. 크게 찍어 축소하는 것이 외곽 계조를 얻는 유일한 방법이다.
	EffectiveSupersample = FMath::Clamp(SupersampleFactor, 1, 4);
	while (EffectiveSupersample > 1
		&& (OutputWidth * EffectiveSupersample > MaxCaptureDimension
			|| OutputHeight * EffectiveSupersample > MaxCaptureDimension))
	{
		--EffectiveSupersample;
	}
	CaptureWidth = OutputWidth * EffectiveSupersample;
	CaptureHeight = OutputHeight * EffectiveSupersample;

	auto EnsureRenderTarget = [this](
		TObjectPtr<UTextureRenderTarget2D>& RenderTarget,
		const FName Name,
		const EPixelFormat PixelFormat,
		const bool bForceLinearGamma)
	{
		if (!RenderTarget)
		{
			RenderTarget = NewObject<UTextureRenderTarget2D>(this, Name, RF_Transient);
		}

		RenderTarget->ClearColor = FLinearColor::Transparent;
		RenderTarget->bAutoGenerateMips = false;
		// 0 이면 포맷에 맞는 엔진 기본 감마가 쓰인다. 직접 2.2 를 넣으면 sRGB 포맷에서 감마가 두 번 걸린다.
		RenderTarget->TargetGamma = 0.f;
		if (RenderTarget->SizeX != CaptureWidth
			|| RenderTarget->SizeY != CaptureHeight
			|| RenderTarget->GetFormat() != PixelFormat)
		{
			// InitAutoFormat 은 RenderTargetFormat 을 참조하지 않는다. 포맷을 확정하려면 InitCustomFormat 이어야 한다.
			RenderTarget->InitCustomFormat(CaptureWidth, CaptureHeight, PixelFormat, bForceLinearGamma);
			RenderTarget->UpdateResourceImmediate(true);
		}
	};

	EnsureRenderTarget(ColorRenderTarget, TEXT("WeaponIconColorRT"), PF_B8G8R8A8, false);
	// 깊이를 8비트로 받으면 배경과 피사체가 같은 값으로 뭉개져 실루엣이 만들어지지 않는다.
	EnsureRenderTarget(DepthRenderTarget, TEXT("WeaponIconDepthRT"), PF_A32B32G32R32F, true);

	FPostProcessSettings& PostProcess = SceneCapture->PostProcessSettings;
	PostProcess.bOverride_AutoExposureMethod = true;
	PostProcess.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	PostProcess.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	PostProcess.AutoExposureApplyPhysicalCameraExposure = false;
	PostProcess.bOverride_AutoExposureBias = true;
	PostProcess.AutoExposureBias = ExposureCompensation;
	PostProcess.bOverride_BloomIntensity = true;
	PostProcess.BloomIntensity = 0.f;
	PostProcess.bOverride_VignetteIntensity = true;
	PostProcess.VignetteIntensity = 0.f;
	PostProcess.bOverride_MotionBlurAmount = true;
	PostProcess.MotionBlurAmount = 0.f;
	PostProcess.bOverride_ColorContrast = true;
	PostProcess.ColorContrast = FVector4(ColorContrast, ColorContrast, ColorContrast, 1.f);
	PostProcess.bOverride_ColorSaturation = true;
	PostProcess.ColorSaturation = FVector4(ColorSaturation, ColorSaturation, ColorSaturation, 1.f);
	SceneCapture->PostProcessBlendWeight = 1.f;
	SceneCapture->ProjectionType = ECameraProjectionMode::Perspective;
	SceneCapture->FOVAngle = FMath::Clamp(CameraFieldOfView, 5.f, 60.f);
	SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;

	// 한 프레임만 찍으므로 TAA 는 누적할 히스토리가 없다. 지터만 남아 흐려지니 끄고 슈퍼샘플로 대신한다.
	SceneCapture->ShowFlags.SetAntiAliasing(false);
	// 반사할 환경이 없어 SSR 은 검은 얼룩만 남긴다.
	SceneCapture->ShowFlags.SetScreenSpaceReflections(false);

	SceneCapture->ClearShowOnlyComponents();
	if (CaptureRig)
	{
		SceneCapture->ShowOnlyActorComponents(CaptureRig, true);
	}
}

bool ALastFPSWeaponIconCaptureActor::PrepareWeapon(USkeletalMesh* WeaponMesh)
{
	USkeletalMeshComponent* CaptureMesh = CaptureRig ? CaptureRig->GetWeaponMesh() : nullptr;
	if (!WeaponMesh || !CaptureMesh || !SceneCapture)
	{
		return false;
	}

	CaptureMesh->SetSkeletalMeshAsset(WeaponMesh);
	// ForcedLOD 는 1부터가 LOD0 이다. 0 은 자동 선택이라 촬영 거리에 따라 저품질 LOD 가 잡힐 수 있다.
	CaptureMesh->SetForcedLOD(1);
	CaptureMesh->SetRelativeRotation(MeshRotation);
	CaptureMesh->SetVisibility(true, true);
	CaptureMesh->SetHiddenInGame(false, true);

	// FBoxSphereBounds::TransformBy 는 회전 결과를 다시 축정렬 상자로 감싸 최대 41% 부풀린다.
	// 코너를 직접 돌려야 MeshRotation 값에 상관없이 무기가 일정한 크기로 담긴다.
	constexpr int32 BoxCornerCount = 8;
	const FBox LocalBox = WeaponMesh->GetImportedBounds().GetBox();
	const FQuat MeshQuat = MeshRotation.Quaternion();
	FVector RotatedCorners[BoxCornerCount];
	for (int32 CornerIndex = 0; CornerIndex < BoxCornerCount; ++CornerIndex)
	{
		const FVector Corner(
			(CornerIndex & 1) ? LocalBox.Max.X : LocalBox.Min.X,
			(CornerIndex & 2) ? LocalBox.Max.Y : LocalBox.Min.Y,
			(CornerIndex & 4) ? LocalBox.Max.Z : LocalBox.Min.Z);
		RotatedCorners[CornerIndex] = MeshQuat.RotateVector(Corner);
	}

	const FBox RotatedBox(RotatedCorners, BoxCornerCount);
	if (RotatedBox.GetExtent().GetMax() < MinimumBoundsSize)
	{
		UE_LOG(
			LogLastFPSWeaponIconCapture,
			Warning,
			TEXT("무기 아이콘 촬영 실패: '%s'의 메시 바운드가 너무 작습니다."),
			*GetNameSafe(WeaponMesh));
		return false;
	}

	// 회전된 바운드 중심을 촬영 원점에 놓아 메시 피벗이 달라도 같은 구도를 유지한다.
	const FVector RotatedCenter = RotatedBox.GetCenter();
	CaptureMesh->SetRelativeLocation(-RotatedCenter);

	const FRotator CameraRotation(SideViewPitch, SideViewYaw, 0.f);
	const FQuat CameraQuat = CameraRotation.Quaternion();

	// 화면 가로·세로·깊이는 카메라 축 기준이다. 월드 X/Z/Y 를 그대로 쓰면 Yaw 가 ±90 일 때만 맞는다.
	FBox CameraSpaceBox(ForceInit);
	for (const FVector& RotatedCorner : RotatedCorners)
	{
		CameraSpaceBox += CameraQuat.UnrotateVector(RotatedCorner - RotatedCenter);
	}
	const FVector CameraExtent = CameraSpaceBox.GetExtent();

	const float AspectRatio = static_cast<float>(OutputWidth) / FMath::Max(1, OutputHeight);
	const float HalfHorizontalFov = FMath::DegreesToRadians(FMath::Clamp(CameraFieldOfView, 5.f, 60.f) * 0.5f);
	const float TanHalfHorizontalFov = FMath::Max(FMath::Tan(HalfHorizontalFov), UE_SMALL_NUMBER);
	const float TanHalfVerticalFov = FMath::Max(
		TanHalfHorizontalFov / FMath::Max(AspectRatio, UE_SMALL_NUMBER), UE_SMALL_NUMBER);
	const float Padding = FMath::Max(FramingPadding, 1.f);
	const float HorizontalDistance = CameraExtent.Y * Padding / TanHalfHorizontalFov;
	const float VerticalDistance = CameraExtent.Z * Padding / TanHalfVerticalFov;
	const float CameraDistance =
		FMath::Max(HorizontalDistance, VerticalDistance) + CameraExtent.X + NearPlaneSafetyMargin;

	SceneCapture->SetRelativeRotation(CameraRotation);
	SceneCapture->SetRelativeLocation(-CameraRotation.Vector() * CameraDistance);

	// 조명은 피사체 크기를 알아야 배치할 수 있다. 바운드 반경에 맞춰야 무기마다 같은 조명이 된다.
	CaptureRig->ConfigureLighting(
		KeyLightSetup,
		FillLightSetup,
		RimLightSetup,
		CameraQuat,
		RotatedBox.GetExtent().Size());

	// 커맨드렛에서는 메시와 머티리얼 컴파일이 비동기로 남을 수 있으므로 촬영 전에 렌더 상태를 확정한다.
	TArray<UObject*> AssetsToFinish;
	AssetsToFinish.Add(WeaponMesh);
	FAssetCompilingManager::Get().FinishCompilationForObjects(AssetsToFinish);

	CaptureMesh->RefreshBoneTransforms();
	CaptureMesh->UpdateBounds();
	CaptureMesh->MarkRenderStateDirty();
	SceneCapture->ClearShowOnlyComponents();
	SceneCapture->ShowOnlyActorComponents(CaptureRig, true);
	SceneCapture->MarkRenderStateDirty();

	// 월드를 Tick 하면 에디터에 열려 있는 레벨의 다른 액터까지 한 프레임 돌아간다.
	// 여기서 필요한 것은 컴포넌트 렌더 상태 반영뿐이므로 프레임 종료 갱신만 흘려보낸다.
	if (UWorld* World = GetWorld())
	{
		World->UpdateWorldComponents(true, false);
		World->SendAllEndOfFrameUpdates();
	}
	FlushRenderingCommands();
	return true;
}

bool ALastFPSWeaponIconCaptureActor::ResolveIconPixels(const FString& AssetName, TArray<FColor>& OutPixels)
{
	if (!ColorRenderTarget || !DepthRenderTarget || !SceneCapture || !CaptureRig)
	{
		return false;
	}

	SceneCapture->ClearShowOnlyComponents();
	SceneCapture->ShowOnlyActorComponents(CaptureRig, true);

	SceneCapture->TextureTarget = ColorRenderTarget;
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCapture->CaptureScene();
	FlushRenderingCommands();

	FImage ColorImage;
	if (!FImageUtils::GetRenderTargetImage(ColorRenderTarget, ColorImage))
	{
		UE_LOG(
			LogLastFPSWeaponIconCapture,
			Error,
			TEXT("무기 아이콘 촬영 실패: 색상 Render Target을 읽지 못했습니다. 무기=%s"),
			*AssetName);
		SceneCapture->TextureTarget = nullptr;
		return false;
	}
	ColorImage.ChangeFormat(ERawImageFormat::BGRA8, EGammaSpace::sRGB);
	const TArrayView64<FColor> ColorPixels = ColorImage.AsBGRA8();

	SceneCapture->TextureTarget = DepthRenderTarget;
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_DeviceDepth;
	SceneCapture->CaptureScene();
	FlushRenderingCommands();

	TArray<FLinearColor> DepthPixels;
	FTextureRenderTargetResource* DepthResource = DepthRenderTarget->GameThread_GetRenderTargetResource();
	const bool bReadDepth = DepthResource && DepthResource->ReadLinearColorPixels(DepthPixels);
	SceneCapture->TextureTarget = nullptr;
	if (!bReadDepth)
	{
		UE_LOG(
			LogLastFPSWeaponIconCapture,
			Error,
			TEXT("무기 아이콘 깊이 버퍼를 읽지 못했습니다. 무기=%s"),
			*AssetName);
		return false;
	}

	const int64 CapturePixelCount = static_cast<int64>(CaptureWidth) * CaptureHeight;
	if (ColorPixels.Num() != CapturePixelCount || DepthPixels.Num() != CapturePixelCount)
	{
		UE_LOG(
			LogLastFPSWeaponIconCapture,
			Error,
			TEXT("무기 아이콘 픽셀 수가 촬영 해상도와 다릅니다: 무기=%s, 색상=%lld, 깊이=%d, 기대=%lld"),
			*AssetName,
			ColorPixels.Num(),
			DepthPixels.Num(),
			CapturePixelCount);
		return false;
	}

	// 피사체만 렌더링하므로 배경은 깊이가 한 값으로 모인다. 네 모서리가 서로 다르면 배경에 무언가
	// 찍혔다는 뜻이고, 이 상태로 계속하면 화면 전체가 불투명한 아이콘이 나온다. 추정 대신 끊는다.
	const int64 CornerIndices[4] = {
		0,
		CaptureWidth - 1,
		static_cast<int64>(CaptureHeight - 1) * CaptureWidth,
		CapturePixelCount - 1};
	const float BackgroundDepth = DepthPixels[CornerIndices[0]].R;
	for (const int64 CornerIndex : CornerIndices)
	{
		if (FMath::Abs(DepthPixels[CornerIndex].R - BackgroundDepth) > MinDepthSeparation)
		{
			UE_LOG(
				LogLastFPSWeaponIconCapture,
				Error,
				TEXT("무기 아이콘 촬영 실패: 배경이 비어 있지 않습니다. 무기=%s, 모서리 깊이=%g/%g/%g/%g"),
				*AssetName,
				DepthPixels[CornerIndices[0]].R,
				DepthPixels[CornerIndices[1]].R,
				DepthPixels[CornerIndices[2]].R,
				DepthPixels[CornerIndices[3]].R);
			return false;
		}
	}

	// 축소하면서 서브픽셀 커버리지를 알파로, 덮인 샘플의 평균을 색으로 만든다.
	// 색은 선형 공간에서 더해야 어두운 쪽으로 치우치지 않는다.
	const float SampleCount = static_cast<float>(EffectiveSupersample * EffectiveSupersample);
	OutPixels.SetNumUninitialized(OutputWidth * OutputHeight);
	int64 CoveredSampleTotal = 0;

	for (int32 Y = 0; Y < OutputHeight; ++Y)
	{
		for (int32 X = 0; X < OutputWidth; ++X)
		{
			FLinearColor ColorSum(0.f, 0.f, 0.f, 0.f);
			int32 CoveredSamples = 0;
			for (int32 SampleY = 0; SampleY < EffectiveSupersample; ++SampleY)
			{
				const int64 RowOffset =
					static_cast<int64>(Y * EffectiveSupersample + SampleY) * CaptureWidth;
				for (int32 SampleX = 0; SampleX < EffectiveSupersample; ++SampleX)
				{
					const int64 SampleIndex = RowOffset + X * EffectiveSupersample + SampleX;
					if (FMath::Abs(DepthPixels[SampleIndex].R - BackgroundDepth) <= MinDepthSeparation)
					{
						continue;
					}

					ColorSum += FLinearColor(ColorPixels[SampleIndex]);
					++CoveredSamples;
				}
			}

			FColor& OutPixel = OutPixels[Y * OutputWidth + X];
			if (CoveredSamples > 0)
			{
				// 덮인 샘플만 평균낸다. 배경까지 섞으면 외곽에 어두운 테두리가 남는다.
				OutPixel = (ColorSum / static_cast<float>(CoveredSamples)).ToFColor(true);
			}
			else
			{
				OutPixel = FColor(0, 0, 0, 0);
			}
			OutPixel.A = static_cast<uint8>(
				FMath::RoundToInt(static_cast<float>(CoveredSamples) / SampleCount * 255.f));
			CoveredSampleTotal += CoveredSamples;
		}
	}

	const float CoverageRatio =
		static_cast<float>(CoveredSampleTotal) / static_cast<float>(CapturePixelCount);
	if (CoverageRatio <= 0.f || CoverageRatio > MaxSilhouetteCoverage)
	{
		UE_LOG(
			LogLastFPSWeaponIconCapture,
			Error,
			TEXT("무기 아이콘 실루엣이 올바르지 않습니다: 무기=%s, 화면 점유율=%.3f, 허용=0 초과 %.2f 이하, 배경깊이=%g"),
			*AssetName,
			CoverageRatio,
			MaxSilhouetteCoverage,
			BackgroundDepth);
		return false;
	}

	return true;
}

bool ALastFPSWeaponIconCaptureActor::ExportPng(
	const FString& AssetName,
	const TArray<FColor>& Pixels) const
{
	if (Pixels.Num() != OutputWidth * OutputHeight)
	{
		UE_LOG(
			LogLastFPSWeaponIconCapture,
			Error,
			TEXT("PNG 출력 실패: 픽셀 수가 출력 해상도와 다릅니다. 무기=%s, 픽셀=%d, 기대=%d"),
			*AssetName,
			Pixels.Num(),
			OutputWidth * OutputHeight);
		return false;
	}

	FImage Image;
	Image.Init(OutputWidth, OutputHeight, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
	FMemory::Memcpy(Image.RawData.GetData(), Pixels.GetData(), Pixels.Num() * sizeof(FColor));

	TArray64<uint8> CompressedPng;
	if (!FImageUtils::CompressImage(CompressedPng, TEXT("PNG"), Image))
	{
		UE_LOG(LogLastFPSWeaponIconCapture, Error, TEXT("PNG 출력 실패: 이미지 압축에 실패했습니다. 무기=%s"), *AssetName);
		return false;
	}

	FString OutputDirectory = PngOutputDirectory;
	if (FPaths::IsRelative(OutputDirectory))
	{
		OutputDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / OutputDirectory);
	}
	FPaths::NormalizeDirectoryName(OutputDirectory);
	if (!IFileManager::Get().MakeDirectory(*OutputDirectory, true))
	{
		UE_LOG(LogLastFPSWeaponIconCapture, Error, TEXT("PNG 출력 폴더를 만들 수 없습니다: %s"), *OutputDirectory);
		return false;
	}

	const FString OutputFilename = OutputDirectory / (AssetName + TEXT(".png"));
	if (!FFileHelper::SaveArrayToFile(CompressedPng, *OutputFilename))
	{
		UE_LOG(LogLastFPSWeaponIconCapture, Error, TEXT("PNG 파일을 저장하지 못했습니다: %s"), *OutputFilename);
		return false;
	}

	UE_LOG(LogLastFPSWeaponIconCapture, Log, TEXT("무기 아이콘 PNG 저장: %s"), *OutputFilename);
	return true;
}

UTexture2D* ALastFPSWeaponIconCaptureActor::CreateOrUpdateTexture(
	const FString& AssetName,
	const TArray<FColor>& Pixels)
{
	FString NormalizedOutputPath = TextureOutputPath;
	NormalizedOutputPath.RemoveFromEnd(TEXT("/"));
	FText PathError;
	if (!FPackageName::IsValidLongPackageName(NormalizedOutputPath, false, &PathError))
	{
		UE_LOG(
			LogLastFPSWeaponIconCapture,
			Error,
			TEXT("무기 아이콘 출력 경로가 유효하지 않습니다: 경로='%s', 원인=%s"),
			*NormalizedOutputPath,
			*PathError.ToString());
		return nullptr;
	}

	const FString PackageName = NormalizedOutputPath / AssetName;
	const FString ObjectPath = PackageName + TEXT(".") + AssetName;
	UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
	if (Texture && !bOverwriteExistingTextures)
	{
		return Texture;
	}

	bool bCreatedTexture = false;
	if (!Texture)
	{
		UPackage* Package = CreatePackage(*PackageName);
		Texture = NewObject<UTexture2D>(Package, *AssetName, RF_Public | RF_Standalone);
		if (!Texture)
		{
			UE_LOG(LogLastFPSWeaponIconCapture, Error, TEXT("무기 아이콘 에셋 생성 실패: %s"), *ObjectPath);
			return nullptr;
		}
		bCreatedTexture = true;
	}

	Texture->Modify();
	// 축소까지 끝난 픽셀을 소스로 직접 넣는다. Render Target 은 촬영 해상도라 그대로 쓸 수 없다.
	Texture->Source.Init(
		OutputWidth,
		OutputHeight,
		1,
		1,
		TSF_BGRA8,
		reinterpret_cast<const uint8*>(Pixels.GetData()));

	Texture->CompressionSettings = TextureCompressionSettings::TC_EditorIcon;
	Texture->LODGroup = TextureGroup::TEXTUREGROUP_UI;
	Texture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
	Texture->SRGB = true;
	Texture->NeverStream = true;
	Texture->AddressX = TextureAddress::TA_Clamp;
	Texture->AddressY = TextureAddress::TA_Clamp;
	// PostEditChange 가 리소스 재생성까지 처리하므로 UpdateResource 를 따로 부르지 않는다.
	Texture->PostEditChange();
	if (bCreatedTexture)
	{
		FAssetRegistryModule::AssetCreated(Texture);
	}
	Texture->MarkPackageDirty();
	return Texture;
}

UTexture2D* ALastFPSWeaponIconCaptureActor::CaptureWeapon(
	const FString& AssetName,
	ULastFPSWeaponDefinition* WeaponDefinition)
{
	if (!WeaponDefinition || !WeaponDefinition->SkeletalMesh)
	{
		return nullptr;
	}

	if (!CreateCaptureRig())
	{
		return nullptr;
	}
	ON_SCOPE_EXIT
	{
		DestroyCaptureRig();
	};

	ApplyCaptureSettings();
	if (!PrepareWeapon(WeaponDefinition->SkeletalMesh))
	{
		return nullptr;
	}

	TArray<FColor> IconPixels;
	if (!ResolveIconPixels(AssetName, IconPixels))
	{
		return nullptr;
	}

	UTexture2D* Texture = CreateOrUpdateTexture(AssetName, IconPixels);
	if (Texture && bExportPng)
	{
		ExportPng(AssetName, IconPixels);
	}
	return Texture;
}
