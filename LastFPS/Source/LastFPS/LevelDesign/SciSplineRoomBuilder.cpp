#include "LevelDesign/SciSplineRoomBuilder.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Templates/UnrealTemplate.h"

DEFINE_LOG_CATEGORY_STATIC(LogSciSplineRoomBuilder, Log, All);

namespace SciSplineRoomBuilder
{
    constexpr float MinimumSplineLength = 1.0f;
    constexpr float BoundsMinimumSize = 1.0f;
    constexpr float RoomBoundarySampleRatio = 0.5f;
}

bool ASciSplineRoomBuilder::FResolvedMeshes::HasRequiredMeshes() const
{
    return FloorBase && FloorPanel && WallBacking && Ceiling;
}

ASciSplineRoomBuilder::ASciSplineRoomBuilder()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
    SceneRoot->SetMobility(EComponentMobility::Static);

    LayoutSpline = CreateDefaultSubobject<USplineComponent>(TEXT("LayoutSpline"));
    LayoutSpline->SetupAttachment(SceneRoot);
    LayoutSpline->SetMobility(EComponentMobility::Static);

    FloorBaseInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("FloorBaseInstances"));
    FloorBaseInstances->SetupAttachment(SceneRoot);

    FloorPanelInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("FloorPanelInstances"));
    FloorPanelInstances->SetupAttachment(SceneRoot);

    WallBackingInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("WallBackingInstances"));
    WallBackingInstances->SetupAttachment(SceneRoot);

    CeilingInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CeilingInstances"));
    CeilingInstances->SetupAttachment(SceneRoot);

    CeilingBackingInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CeilingBackingInstances"));
    CeilingBackingInstances->SetupAttachment(SceneRoot);

    CeilingPipeInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CeilingPipeInstances"));
    CeilingPipeInstances->SetupAttachment(SceneRoot);

    const TArray<UHierarchicalInstancedStaticMeshComponent*> GeneratedComponents = {
        FloorBaseInstances,
        FloorPanelInstances,
        WallBackingInstances,
        CeilingInstances,
        CeilingBackingInstances,
        CeilingPipeInstances,
    };
    for (UHierarchicalInstancedStaticMeshComponent* Component : GeneratedComponents)
    {
        Component->SetMobility(EComponentMobility::Static);
        Component->SetGenerateOverlapEvents(false);
    }

    FloorBaseInstances->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    WallBackingInstances->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    FloorPanelInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CeilingInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CeilingBackingInstances->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    CeilingPipeInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    FloorBaseMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")));
    FloorPanelMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/ModSci_Engineer/Meshes/SM_Intersection_Floor.SM_Intersection_Floor")));
    WallBackingMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")));
    CeilingMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Assets/FAB/ModSciInteriors/Meshes/SM_Ceiling_Main.SM_Ceiling_Main")));
    CeilingPipeMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Assets/FAB/ModSciInteriors/Meshes/SM_Ceiling_Main_B2_Pipe.SM_Ceiling_Main_B2_Pipe")));
    FloorBaseMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/ModSci_Engineer/Materials/MI_Trim_A_DarkGray.MI_Trim_A_DarkGray")));
    WallBackingMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/ModSci_Engineer/Materials/MI_Trim_A_DarkGray.MI_Trim_A_DarkGray")));
}

void ASciSplineRoomBuilder::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (bAutoRebuild)
    {
        RebuildGeneratedGeometry();
    }
}

void ASciSplineRoomBuilder::RebuildGeneratedGeometry()
{
    if (bIsRebuilding || !LayoutSpline)
    {
        return;
    }

    TGuardValue<bool> RebuildGuard(bIsRebuilding, true);
    EnsureGeneratedComponentHierarchy();
    ClearGeneratedGeometry();

    ModuleSize = FMath::Max(ModuleSize, 100.0f);
    CorridorHalfWidth = FMath::Max(CorridorHalfWidth, 100.0f);
    WallModuleHeight = FMath::Max(WallModuleHeight, 100.0f);
    WallStackCount = FMath::Clamp(WallStackCount, 1, 8);
    WallThickness = FMath::Max(WallThickness, 1.0f);
    FloorBaseThickness = FMath::Max(FloorBaseThickness, 1.0f);
    CeilingHeight = FMath::Max(CeilingHeight, WallModuleHeight);
    CeilingBackingThickness = FMath::Max(CeilingBackingThickness, 1.0f);
    PipeCeilingInterval = FMath::Max(PipeCeilingInterval, 1);
    MaxGeneratedCells = FMath::Max(MaxGeneratedCells, 1);

    const FResolvedMeshes Meshes = ResolveMeshes();
    if (!Meshes.HasRequiredMeshes())
    {
        UE_LOG(LogSciSplineRoomBuilder, Warning, TEXT("Actor '%s'의 SCI 생성에 필요한 Static Mesh 설정이 비어 있습니다."), *GetPathName());
        return;
    }

    ConfigureGeneratedComponents(Meshes);

    if (LayoutSpline->GetSplineLength() < SciSplineRoomBuilder::MinimumSplineLength)
    {
        return;
    }

    if (bFillClosedSpline && LayoutSpline->IsClosedLoop())
    {
        BuildClosedRoom(Meshes);
    }
    else
    {
        BuildOpenCorridor(Meshes);
    }
}

void ASciSplineRoomBuilder::ClearGeneratedGeometry()
{
    const TArray<UHierarchicalInstancedStaticMeshComponent*> GeneratedComponents = {
        FloorBaseInstances,
        FloorPanelInstances,
        WallBackingInstances,
        CeilingInstances,
        CeilingBackingInstances,
        CeilingPipeInstances,
    };
    for (UHierarchicalInstancedStaticMeshComponent* Component : GeneratedComponents)
    {
        if (Component)
        {
            Component->ClearInstances();
        }
    }
}

ASciSplineRoomBuilder::FResolvedMeshes ASciSplineRoomBuilder::ResolveMeshes() const
{
    FResolvedMeshes Result;
    Result.FloorBase = FloorBaseMesh.LoadSynchronous();
    Result.FloorPanel = FloorPanelMesh.LoadSynchronous();
    Result.WallBacking = WallBackingMesh.LoadSynchronous();
    Result.Ceiling = CeilingMesh.LoadSynchronous();
    Result.CeilingPipe = CeilingPipeMesh.LoadSynchronous();
    Result.FloorMaterial = FloorBaseMaterial.LoadSynchronous();
    Result.WallMaterial = WallBackingMaterial.LoadSynchronous();
    return Result;
}

void ASciSplineRoomBuilder::EnsureGeneratedComponentHierarchy()
{
    if (!SceneRoot || !LayoutSpline)
    {
        return;
    }

    // 기존 Blueprint 인스턴스에 직렬화된 Mobility 값도 재구성 시 동일하게 맞춘다.
    SceneRoot->SetMobility(EComponentMobility::Static);
    LayoutSpline->SetMobility(EComponentMobility::Static);

    const TArray<UHierarchicalInstancedStaticMeshComponent*> GeneratedComponents = {
        FloorBaseInstances,
        FloorPanelInstances,
        WallBackingInstances,
        CeilingInstances,
        CeilingBackingInstances,
        CeilingPipeInstances,
    };

    for (UHierarchicalInstancedStaticMeshComponent* Component : GeneratedComponents)
    {
        if (!Component)
        {
            continue;
        }

        Component->SetMobility(EComponentMobility::Static);
        if (Component->GetAttachParent() != SceneRoot)
        {
            Component->AttachToComponent(SceneRoot, FAttachmentTransformRules::SnapToTargetIncludingScale);
        }
        Component->SetRelativeTransform(FTransform::Identity);
    }
}

void ASciSplineRoomBuilder::ConfigureGeneratedComponents(const FResolvedMeshes& Meshes)
{
    FloorBaseInstances->SetStaticMesh(Meshes.FloorBase);
    FloorPanelInstances->SetStaticMesh(Meshes.FloorPanel);
    WallBackingInstances->SetStaticMesh(Meshes.WallBacking);
    CeilingInstances->SetStaticMesh(Meshes.Ceiling);
    CeilingBackingInstances->SetStaticMesh(Meshes.FloorBase);
    CeilingPipeInstances->SetStaticMesh(Meshes.CeilingPipe);

    if (Meshes.FloorMaterial)
    {
        FloorBaseInstances->SetMaterial(0, Meshes.FloorMaterial);
        CeilingBackingInstances->SetMaterial(0, Meshes.FloorMaterial);
    }
    if (Meshes.WallMaterial)
    {
        WallBackingInstances->SetMaterial(0, Meshes.WallMaterial);
    }
}

void ASciSplineRoomBuilder::BuildOpenCorridor(const FResolvedMeshes& Meshes)
{
    const float SplineLength = LayoutSpline->GetSplineLength();
    const int32 SplinePointCount = LayoutSpline->GetNumberOfSplinePoints();
    if (SplinePointCount < 2 || SplineLength < SciSplineRoomBuilder::MinimumSplineLength)
    {
        return;
    }

    const float FullWidth = CorridorHalfWidth * 2.0f;
    const int32 WidthCellCount = FMath::Max(1, FMath::CeilToInt(FullWidth / ModuleSize));
    const float CellWidth = FullWidth / static_cast<float>(WidthCellCount);
    int32 CellIndex = 0;

    // 꺾이는 통로가 코너를 가로지르지 않도록 각 스플라인 포인트 구간을 독립적으로 채운다.
    const int32 SplineSegmentCount = LayoutSpline->IsClosedLoop() ? SplinePointCount : SplinePointCount - 1;
    for (int32 SplineSegmentIndex = 0; SplineSegmentIndex < SplineSegmentCount; ++SplineSegmentIndex)
    {
        const float SplineSegmentStart = LayoutSpline->GetDistanceAlongSplineAtSplinePoint(SplineSegmentIndex);
        const float SplineSegmentEnd = SplineSegmentIndex + 1 < SplinePointCount
            ? LayoutSpline->GetDistanceAlongSplineAtSplinePoint(SplineSegmentIndex + 1)
            : SplineLength;
        const float SplineSegmentLength = SplineSegmentEnd - SplineSegmentStart;
        if (SplineSegmentLength < SciSplineRoomBuilder::MinimumSplineLength)
        {
            continue;
        }

        const int32 ModuleCount = FMath::Max(1, FMath::CeilToInt(SplineSegmentLength / ModuleSize));
        const float SegmentModuleLength = SplineSegmentLength / static_cast<float>(ModuleCount);
        for (int32 ModuleIndex = 0; ModuleIndex < ModuleCount; ++ModuleIndex)
        {
            const float SegmentStart = SplineSegmentStart + SegmentModuleLength * ModuleIndex;
            const float SegmentEnd = SplineSegmentStart + SegmentModuleLength * (ModuleIndex + 1);
            const float CenterDistance = (SegmentStart + SegmentEnd) * 0.5f;
            const FVector Center = GetSplineLocationInActorSpace(CenterDistance);
            const FRotator Rotation = GetSplineRotationInActorSpace(CenterDistance);
            const FVector Right = GetSplineRightVectorInActorSpace(CenterDistance);

            for (int32 WidthIndex = 0; WidthIndex < WidthCellCount; ++WidthIndex)
            {
                const float LateralOffset = -CorridorHalfWidth + CellWidth * (WidthIndex + 0.5f);
                AddFloorAndCeilingCell(
                    Meshes,
                    Center + Right * LateralOffset,
                    Rotation,
                    SegmentModuleLength,
                    CellWidth,
                    CellIndex++);
            }
        }
    }

    BuildPerimeterWalls(Meshes, true);
}

void ASciSplineRoomBuilder::BuildClosedRoom(const FResolvedMeshes& Meshes)
{
    const float SplineLength = LayoutSpline->GetSplineLength();
    const float BoundarySampleStep = FMath::Max(ModuleSize * SciSplineRoomBuilder::RoomBoundarySampleRatio, 1.0f);
    const int32 SplinePointCount = LayoutSpline->GetNumberOfSplinePoints();
    if (SplinePointCount < 3)
    {
        return;
    }

    FVector BasisX = GetSplineDirectionInActorSpace(0.0f);
    BasisX.Z = 0.0f;
    if (!BasisX.Normalize())
    {
        BasisX = FVector::ForwardVector;
    }
    const FVector BasisY(-BasisX.Y, BasisX.X, 0.0f);
    const float RoomZ = GetSplineLocationInActorSpace(0.0f).Z;

    TArray<FVector2D> Polygon;
    Polygon.Reserve(FMath::Max(SplinePointCount, FMath::CeilToInt(SplineLength / BoundarySampleStep)));
    FVector2D Minimum(FLT_MAX, FLT_MAX);
    FVector2D Maximum(-FLT_MAX, -FLT_MAX);

    // 각 제어점을 구간 시작으로 반드시 포함해 좁은 통로와 직각 코너가 잘리지 않게 한다.
    for (int32 SplineSegmentIndex = 0; SplineSegmentIndex < SplinePointCount; ++SplineSegmentIndex)
    {
        const float SegmentStart = LayoutSpline->GetDistanceAlongSplineAtSplinePoint(SplineSegmentIndex);
        const float SegmentEnd = SplineSegmentIndex + 1 < SplinePointCount
            ? LayoutSpline->GetDistanceAlongSplineAtSplinePoint(SplineSegmentIndex + 1)
            : SplineLength;
        const float SegmentLength = SegmentEnd - SegmentStart;
        const int32 SegmentSampleCount = FMath::Max(1, FMath::CeilToInt(SegmentLength / BoundarySampleStep));

        for (int32 SampleIndex = 0; SampleIndex < SegmentSampleCount; ++SampleIndex)
        {
            const float Alpha = static_cast<float>(SampleIndex) / static_cast<float>(SegmentSampleCount);
            const FVector Position = GetSplineLocationInActorSpace(SegmentStart + SegmentLength * Alpha);
            const FVector2D Projected(FVector::DotProduct(Position, BasisX), FVector::DotProduct(Position, BasisY));
            Polygon.Add(Projected);
            Minimum.X = FMath::Min(Minimum.X, Projected.X);
            Minimum.Y = FMath::Min(Minimum.Y, Projected.Y);
            Maximum.X = FMath::Max(Maximum.X, Projected.X);
            Maximum.Y = FMath::Max(Maximum.Y, Projected.Y);
        }
    }

    const int32 CandidateCountX = FMath::Max(1, FMath::CeilToInt((Maximum.X - Minimum.X) / ModuleSize));
    const int32 CandidateCountY = FMath::Max(1, FMath::CeilToInt((Maximum.Y - Minimum.Y) / ModuleSize));
    const int64 CandidateCount = static_cast<int64>(CandidateCountX) * static_cast<int64>(CandidateCountY);
    if (CandidateCount > static_cast<int64>(MaxGeneratedCells) * 4)
    {
        UE_LOG(LogSciSplineRoomBuilder, Warning, TEXT("Actor '%s'의 닫힌 Spline 후보 셀이 너무 많아 생성을 중단했습니다. 후보=%lld, 제한=%d"), *GetPathName(), CandidateCount, MaxGeneratedCells);
        return;
    }

    const FRotator RoomRotation = BasisX.Rotation();
    int32 GeneratedCellCount = 0;
    for (int32 Y = 0; Y < CandidateCountY; ++Y)
    {
        for (int32 X = 0; X < CandidateCountX; ++X)
        {
            const FVector2D CellPoint(Minimum.X + (X + 0.5f) * ModuleSize, Minimum.Y + (Y + 0.5f) * ModuleSize);
            if (!IsPointInsidePolygon(CellPoint, Polygon))
            {
                continue;
            }

            if (GeneratedCellCount >= MaxGeneratedCells)
            {
                UE_LOG(LogSciSplineRoomBuilder, Warning, TEXT("Actor '%s'가 최대 생성 셀 수 %d에 도달했습니다."), *GetPathName(), MaxGeneratedCells);
                BuildPerimeterWalls(Meshes, false);
                return;
            }

            const FVector CellCenter = BasisX * CellPoint.X + BasisY * CellPoint.Y + FVector::UpVector * RoomZ;
            AddFloorAndCeilingCell(Meshes, CellCenter, RoomRotation, ModuleSize, ModuleSize, GeneratedCellCount++);
        }
    }

    BuildPerimeterWalls(Meshes, false);
}

void ASciSplineRoomBuilder::BuildPerimeterWalls(const FResolvedMeshes& Meshes, const bool bBuildBothSides)
{
    if (!Meshes.WallBacking)
    {
        return;
    }

    const float SplineLength = LayoutSpline->GetSplineLength();
    const int32 PointCount = LayoutSpline->GetNumberOfSplinePoints();
    if (PointCount < 2 || SplineLength < SciSplineRoomBuilder::MinimumSplineLength)
    {
        return;
    }

    // 각 스플라인 포인트를 벽 구간의 경계로 사용해야 직각 모서리를 가로지르는 벽이 생기지 않는다.
    const int32 SplineSegmentCount = LayoutSpline->IsClosedLoop() ? PointCount : PointCount - 1;
    for (int32 SplineSegmentIndex = 0; SplineSegmentIndex < SplineSegmentCount; ++SplineSegmentIndex)
    {
        // 포인트 Scale Z가 0인 구간은 출입구로 사용하며 벽을 생성하지 않는다.
        if (LayoutSpline->GetScaleAtSplinePoint(SplineSegmentIndex).Z <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        const float SplineSegmentStart = LayoutSpline->GetDistanceAlongSplineAtSplinePoint(SplineSegmentIndex);
        const float SplineSegmentEnd = SplineSegmentIndex + 1 < PointCount
            ? LayoutSpline->GetDistanceAlongSplineAtSplinePoint(SplineSegmentIndex + 1)
            : SplineLength;
        const float SplineSegmentLength = SplineSegmentEnd - SplineSegmentStart;
        if (SplineSegmentLength < SciSplineRoomBuilder::MinimumSplineLength)
        {
            continue;
        }

        const int32 WallModuleCount = FMath::Max(1, FMath::CeilToInt(SplineSegmentLength / ModuleSize));
        const float WallModuleLength = SplineSegmentLength / static_cast<float>(WallModuleCount);
        for (int32 WallModuleIndex = 0; WallModuleIndex < WallModuleCount; ++WallModuleIndex)
        {
            const float SegmentStart = SplineSegmentStart + WallModuleLength * WallModuleIndex;
            const float SegmentEnd = SplineSegmentStart + WallModuleLength * (WallModuleIndex + 1);
            const float CenterDistance = (SegmentStart + SegmentEnd) * 0.5f;
            const FVector Center = GetSplineLocationInActorSpace(CenterDistance);
            const FRotator Rotation = GetSplineRotationInActorSpace(CenterDistance);

            // 양 끝을 반 두께씩 연장해 모듈 이음새와 직각 코너에서 빛이 새는 틈을 막는다.
            const float OverlappedLength = WallModuleLength + WallThickness;
            if (bBuildBothSides)
            {
                const FVector Right = GetSplineRightVectorInActorSpace(CenterDistance);
                AddWallModule(*Meshes.WallBacking, Center - Right * CorridorHalfWidth, Rotation, OverlappedLength);
                AddWallModule(*Meshes.WallBacking, Center + Right * CorridorHalfWidth, Rotation, OverlappedLength);
            }
            else
            {
                AddWallModule(*Meshes.WallBacking, Center, Rotation, OverlappedLength);
            }
        }
    }
}

void ASciSplineRoomBuilder::AddFloorAndCeilingCell(
    const FResolvedMeshes& Meshes,
    const FVector& CellCenter,
    const FRotator& CellRotation,
    const float CellLength,
    const float CellWidth,
    const int32 CellIndex)
{
    const FVector Up = CellRotation.RotateVector(FVector::UpVector);
    const FVector BaseCenter = CellCenter - Up * (FloorBaseThickness * 0.5f);
    const FVector BaseScale(CellLength / 100.0f, CellWidth / 100.0f, FloorBaseThickness / 100.0f);
    FloorBaseInstances->AddInstance(FTransform(CellRotation.Quaternion(), BaseCenter, BaseScale), false);

    const FVector FloorScale = CalculateXYFitScale(*Meshes.FloorPanel, CellLength, CellWidth);
    FloorPanelInstances->AddInstance(MakePivotCorrectedTransform(*Meshes.FloorPanel, CellCenter, CellRotation, FloorScale), false);

    if (!bGenerateCeiling)
    {
        return;
    }

    const FVector CeilingCenter = CellCenter + Up * CeilingHeight;
    const FVector CeilingBackingCenter = CeilingCenter + Up * (CeilingBackingThickness * 0.5f);
    const FVector CeilingBackingScale(CellLength / 100.0f, CellWidth / 100.0f, CeilingBackingThickness / 100.0f);
    CeilingBackingInstances->AddInstance(
        FTransform(CellRotation.Quaternion(), CeilingBackingCenter, CeilingBackingScale),
        false);

    const FVector CeilingScale = CalculateXYFitScale(*Meshes.Ceiling, CellLength, CellWidth);
    const FTransform CeilingTransform = MakePivotCorrectedTransform(*Meshes.Ceiling, CeilingCenter, CellRotation, CeilingScale);
    CeilingInstances->AddInstance(CeilingTransform, false);

    if (bGeneratePipeCeiling && Meshes.CeilingPipe && CellIndex % PipeCeilingInterval == 0)
    {
        CeilingPipeInstances->AddInstance(CeilingTransform, false);
    }
}

void ASciSplineRoomBuilder::AddWallModule(
    const UStaticMesh& WallMesh,
    const FVector& WallCenter,
    const FRotator& WallRotation,
    const float SegmentLength)
{
    const FVector Up = WallRotation.RotateVector(FVector::UpVector);
    for (int32 StackIndex = 0; StackIndex < WallStackCount; ++StackIndex)
    {
        const float CenterHeight = WallModuleHeight * (StackIndex + 0.5f);
        const FVector Center = WallCenter + Up * CenterHeight;
        const FVector Scale = CalculateXYZFitScale(WallMesh, SegmentLength, WallThickness, WallModuleHeight);
        WallBackingInstances->AddInstance(
            MakeBoundsCenteredTransform(WallMesh, Center, WallRotation, Scale),
            false);
    }
}

bool ASciSplineRoomBuilder::IsPointInsidePolygon(const FVector2D& Point, const TArray<FVector2D>& Polygon)
{
    if (Polygon.Num() < 3)
    {
        return false;
    }

    bool bInside = false;
    int32 PreviousIndex = Polygon.Num() - 1;
    for (int32 Index = 0; Index < Polygon.Num(); ++Index)
    {
        const FVector2D& Current = Polygon[Index];
        const FVector2D& Previous = Polygon[PreviousIndex];
        const bool bCrossesScanLine = (Current.Y > Point.Y) != (Previous.Y > Point.Y);
        if (bCrossesScanLine)
        {
            const double IntersectionX = (Previous.X - Current.X) * (Point.Y - Current.Y) / (Previous.Y - Current.Y) + Current.X;
            if (Point.X < IntersectionX)
            {
                bInside = !bInside;
            }
        }
        PreviousIndex = Index;
    }
    return bInside;
}

FVector ASciSplineRoomBuilder::CalculateXYFitScale(const UStaticMesh& Mesh, const float TargetLength, const float TargetWidth)
{
    const FBox Bounds = Mesh.GetBoundingBox();
    const float MeshLength = FMath::Max(Bounds.Max.X - Bounds.Min.X, SciSplineRoomBuilder::BoundsMinimumSize);
    const float MeshWidth = FMath::Max(Bounds.Max.Y - Bounds.Min.Y, SciSplineRoomBuilder::BoundsMinimumSize);
    return FVector(TargetLength / MeshLength, TargetWidth / MeshWidth, 1.0f);
}

FVector ASciSplineRoomBuilder::CalculateXYZFitScale(
    const UStaticMesh& Mesh,
    const float TargetLength,
    const float TargetWidth,
    const float TargetHeight)
{
    const FBox Bounds = Mesh.GetBoundingBox();
    const FVector MeshSize = Bounds.GetSize().ComponentMax(FVector(SciSplineRoomBuilder::BoundsMinimumSize));
    return FVector(TargetLength / MeshSize.X, TargetWidth / MeshSize.Y, TargetHeight / MeshSize.Z);
}

FTransform ASciSplineRoomBuilder::MakePivotCorrectedTransform(
    const UStaticMesh& Mesh,
    const FVector& TargetCenter,
    const FRotator& Rotation,
    const FVector& Scale)
{
    const FBox Bounds = Mesh.GetBoundingBox();
    const FVector LocalBoundsCenter(
        (Bounds.Min.X + Bounds.Max.X) * 0.5f * Scale.X,
        (Bounds.Min.Y + Bounds.Max.Y) * 0.5f * Scale.Y,
        0.0f);
    const FVector PivotLocation = TargetCenter - Rotation.RotateVector(LocalBoundsCenter);
    return FTransform(Rotation.Quaternion(), PivotLocation, Scale);
}

FTransform ASciSplineRoomBuilder::MakeBoundsCenteredTransform(
    const UStaticMesh& Mesh,
    const FVector& TargetCenter,
    const FRotator& Rotation,
    const FVector& Scale)
{
    const FVector ScaledBoundsCenter = Mesh.GetBoundingBox().GetCenter() * Scale;
    const FVector PivotLocation = TargetCenter - Rotation.RotateVector(ScaledBoundsCenter);
    return FTransform(Rotation.Quaternion(), PivotLocation, Scale);
}

FVector ASciSplineRoomBuilder::GetSplineLocationInActorSpace(const float Distance) const
{
    const FVector WorldLocation = LayoutSpline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
    return GetActorTransform().InverseTransformPosition(WorldLocation);
}

FVector ASciSplineRoomBuilder::GetSplineDirectionInActorSpace(const float Distance) const
{
    const FVector WorldDirection = LayoutSpline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
    return GetActorTransform().InverseTransformVectorNoScale(WorldDirection).GetSafeNormal();
}

FVector ASciSplineRoomBuilder::GetSplineRightVectorInActorSpace(const float Distance) const
{
    const FVector WorldRight = LayoutSpline->GetRightVectorAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
    return GetActorTransform().InverseTransformVectorNoScale(WorldRight).GetSafeNormal();
}

FRotator ASciSplineRoomBuilder::GetSplineRotationInActorSpace(const float Distance) const
{
    const FQuat WorldRotation = LayoutSpline->GetQuaternionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
    return (GetActorQuat().Inverse() * WorldRotation).Rotator();
}
