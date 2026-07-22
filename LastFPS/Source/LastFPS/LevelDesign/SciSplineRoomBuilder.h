#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPtr.h"

#include "SciSplineRoomBuilder.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class USceneComponent;
class USplineComponent;
class UStaticMesh;

UCLASS(Blueprintable)
class LASTFPS_API ASciSplineRoomBuilder : public AActor
{
    GENERATED_BODY()

public:
    ASciSplineRoomBuilder();

    virtual void OnConstruction(const FTransform& Transform) override;

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "SCI Spline Builder")
    void RebuildGeneratedGeometry();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "SCI Spline Builder")
    void ClearGeneratedGeometry();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SCI Spline Builder")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SCI Spline Builder")
    TObjectPtr<USplineComponent> LayoutSpline;

    UPROPERTY(VisibleAnywhere, Category = "SCI Spline Builder|Generated")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FloorBaseInstances;

    UPROPERTY(VisibleAnywhere, Category = "SCI Spline Builder|Generated")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FloorPanelInstances;

    UPROPERTY(VisibleAnywhere, Category = "SCI Spline Builder|Generated")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> WallBackingInstances;

    UPROPERTY(VisibleAnywhere, Category = "SCI Spline Builder|Generated")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> CeilingInstances;

    UPROPERTY(VisibleAnywhere, Category = "SCI Spline Builder|Generated")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> CeilingBackingInstances;

    UPROPERTY(VisibleAnywhere, Category = "SCI Spline Builder|Generated")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> CeilingPipeInstances;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Generation")
    bool bAutoRebuild = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Generation")
    bool bFillClosedSpline = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Generation", meta = (ClampMin = "100.0", UIMin = "100.0"))
    float ModuleSize = 720.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Corridor", meta = (ClampMin = "100.0", UIMin = "100.0"))
    float CorridorHalfWidth = 720.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Wall", meta = (ClampMin = "100.0", UIMin = "100.0"))
    float WallModuleHeight = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Wall", meta = (ClampMin = "1", ClampMax = "8", UIMin = "1", UIMax = "8"))
    int32 WallStackCount = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Wall", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float WallThickness = 32.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Floor", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float FloorBaseThickness = 70.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Ceiling", meta = (ClampMin = "100.0", UIMin = "100.0"))
    float CeilingHeight = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Ceiling", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float CeilingBackingThickness = 40.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Ceiling")
    bool bGenerateCeiling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Ceiling")
    bool bGeneratePipeCeiling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Ceiling", meta = (ClampMin = "1", UIMin = "1"))
    int32 PipeCeilingInterval = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Safety", meta = (ClampMin = "1", UIMin = "1"))
    int32 MaxGeneratedCells = 4096;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Assets")
    TSoftObjectPtr<UStaticMesh> FloorBaseMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Assets")
    TSoftObjectPtr<UStaticMesh> FloorPanelMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Assets")
    TSoftObjectPtr<UStaticMesh> WallBackingMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Assets")
    TSoftObjectPtr<UStaticMesh> CeilingMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Assets")
    TSoftObjectPtr<UStaticMesh> CeilingPipeMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Assets")
    TSoftObjectPtr<UMaterialInterface> FloorBaseMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SCI Spline Builder|Assets")
    TSoftObjectPtr<UMaterialInterface> WallBackingMaterial;

private:
    struct FResolvedMeshes
    {
        UStaticMesh* FloorBase = nullptr;
        UStaticMesh* FloorPanel = nullptr;
        UStaticMesh* WallBacking = nullptr;
        UStaticMesh* Ceiling = nullptr;
        UStaticMesh* CeilingPipe = nullptr;
        UMaterialInterface* FloorMaterial = nullptr;
        UMaterialInterface* WallMaterial = nullptr;

        bool HasRequiredMeshes() const;
    };

    FResolvedMeshes ResolveMeshes() const;
    void EnsureGeneratedComponentHierarchy();
    void ConfigureGeneratedComponents(const FResolvedMeshes& Meshes);
    void BuildOpenCorridor(const FResolvedMeshes& Meshes);
    void BuildClosedRoom(const FResolvedMeshes& Meshes);
    void BuildPerimeterWalls(const FResolvedMeshes& Meshes, bool bBuildBothSides);
    void AddFloorAndCeilingCell(
        const FResolvedMeshes& Meshes,
        const FVector& CellCenter,
        const FRotator& CellRotation,
        float CellLength,
        float CellWidth,
        int32 CellIndex);
    void AddWallModule(
        const UStaticMesh& WallMesh,
        const FVector& WallCenter,
        const FRotator& WallRotation,
        float SegmentLength);

    static bool IsPointInsidePolygon(const FVector2D& Point, const TArray<FVector2D>& Polygon);
    static FVector CalculateXYFitScale(const UStaticMesh& Mesh, float TargetLength, float TargetWidth);
    static FVector CalculateXYZFitScale(
        const UStaticMesh& Mesh,
        float TargetLength,
        float TargetWidth,
        float TargetHeight);
    static FTransform MakePivotCorrectedTransform(
        const UStaticMesh& Mesh,
        const FVector& TargetCenter,
        const FRotator& Rotation,
        const FVector& Scale);
    static FTransform MakeBoundsCenteredTransform(
        const UStaticMesh& Mesh,
        const FVector& TargetCenter,
        const FRotator& Rotation,
        const FVector& Scale);

    FVector GetSplineLocationInActorSpace(float Distance) const;
    FVector GetSplineDirectionInActorSpace(float Distance) const;
    FVector GetSplineRightVectorInActorSpace(float Distance) const;
    FRotator GetSplineRotationInActorSpace(float Distance) const;

    bool bIsRebuilding = false;
};
