#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponIcons/LastFPSWeaponCaptureRig.h"
#include "LastFPSWeaponIconCaptureActor.generated.h"

class UDataTable;
class ULastFPSWeaponDefinition;
class USceneCaptureComponent2D;
class USceneComponent;
class USkeletalMesh;
class UTexture2D;
class UTextureRenderTarget2D;

/**
 * 무기 슬롯 아이콘을 동일한 구도와 조명으로 생성하는 에디터 전용 촬영 액터다.
 * 아이템 테이블을 순회하고 무기 메시 바운드에 맞춰 카메라를 자동 조정한다.
 *
 * 알파는 깊이 버퍼로 만든다. 깊이에는 안티에일리어싱이 없어 그대로 쓰면 외곽이 계단이 되므로,
 * 출력 해상도의 정수배로 찍은 뒤 소프트웨어에서 축소해 서브픽셀 커버리지를 알파로 환산한다.
 */
UCLASS(meta=(DisplayName="Last FPS Weapon Icon Capture Actor"))
class LASTFPSEDITOR_API ALastFPSWeaponIconCaptureActor final : public AActor
{
	GENERATED_BODY()

public:
	ALastFPSWeaponIconCaptureActor();

	virtual bool IsEditorOnly() const override { return true; }

	/** ItemTable의 모든 무기 행을 촬영하고 아이콘 에셋을 연결한다. */
	UFUNCTION(CallInEditor, Category="Weapon Icon Capture")
	void CaptureAllWeaponIcons();

	/** PreviewWeapon 한 개만 촬영한다. 조명과 구도를 빠르게 확인할 때 사용한다. */
	UFUNCTION(CallInEditor, Category="Weapon Icon Capture")
	void CapturePreviewWeaponIcon();

	/** 사용자 UI 없이 실행하는 에디터 명령이 일괄 촬영 입력을 전달한다. */
	void ConfigureBatchCapture(UDataTable* InItemTable, const FString& InPngOutputDirectory);

	int32 GetLastCapturedCount() const { return LastCapturedCount; }

protected:
	UPROPERTY(VisibleAnywhere, Category="Weapon Icon Capture|Components")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, Category="Weapon Icon Capture|Components")
	TObjectPtr<USceneCaptureComponent2D> SceneCapture;

	/** FLastFPSItemData 행 구조를 사용하는 아이템 테이블이다. */
	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Input")
	TObjectPtr<UDataTable> ItemTable;

	/** 단일 촬영 버튼이 사용할 무기 정의다. */
	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Input")
	TObjectPtr<ULastFPSWeaponDefinition> PreviewWeapon;

	/** 생성할 Texture2D의 콘텐츠 경로다. */
	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Output")
	FString TextureOutputPath = TEXT("/Game/UI/Icons/WeaponSlots");

	/** 가로형 무기 슬롯에 맞춘 출력 너비다. */
	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Output", meta=(ClampMin="128", ClampMax="4096", UIMin="512", UIMax="2048"))
	int32 OutputWidth = 1024;

	/** 가로형 무기 슬롯에 맞춘 출력 높이다. */
	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Output", meta=(ClampMin="64", ClampMax="2048", UIMin="128", UIMax="512"))
	int32 OutputHeight = 256;

	/**
	 * 출력 해상도의 몇 배로 촬영할지 정한다. 축소하면서 외곽 커버리지가 알파 계조가 되고
	 * 표면 앨리어싱도 함께 정리된다. 촬영 픽셀 수가 제곱으로 늘어나 4를 상한으로 둔다.
	 */
	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Output", meta=(ClampMin="1", ClampMax="4"))
	int32 SupersampleFactor = 4;

	/** 같은 이름의 텍스처가 있으면 픽셀을 갱신한다. 끄면 기존 에셋을 보존한다. */
	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Output")
	bool bOverwriteExistingTextures = true;

	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Output")
	bool bAssignToWeaponDefinition = true;

	/** Texture2D 생성과 함께 투명 배경 PNG를 디스크에 저장한다. */
	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Output")
	bool bExportPng = true;

	/** 절대 경로가 아니면 프로젝트 디렉터리를 기준으로 해석한다. */
	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Output", meta=(EditCondition="bExportPng"))
	FString PngOutputDirectory = TEXT("Saved/WeaponSlotIcons");

	/** 메시 회전 이후 화면 가장자리에 남길 여백 비율이다. */
	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Framing", meta=(ClampMin="1.0", ClampMax="2.0"))
	float FramingPadding = 1.15f;

	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Framing")
	FRotator MeshRotation = FRotator::ZeroRotator;

	/** 무기의 높낮이 왜곡 없이 정측면을 보여 주는 카메라 피치다. */
	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Framing", meta=(ClampMin="-90.0", ClampMax="90.0"))
	float SideViewPitch = 0.f;

	/** 무기의 정측면을 바라보는 카메라 요다. 반대쪽 측면은 -90도로 설정한다. */
	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Framing", meta=(ClampMin="-180.0", ClampMax="180.0"))
	float SideViewYaw = 90.f;

	/** 원근감은 약하게 남기면서 예시 이미지처럼 입체감을 주는 수평 화각이다. */
	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Framing", meta=(ClampMin="5.0", ClampMax="60.0", Units="deg"))
	float CameraFieldOfView = 20.f;

	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Lighting")
	FLastFPSIconLightSetup KeyLightSetup;

	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Lighting")
	FLastFPSIconLightSetup FillLightSetup;

	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Lighting")
	FLastFPSIconLightSetup RimLightSetup;

	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Lighting", meta=(ClampMin="-10.0", ClampMax="10.0"))
	float ExposureCompensation = -0.5f;

	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Post Process", meta=(ClampMin="0.0", ClampMax="2.0"))
	float ColorContrast = 1.f;

	UPROPERTY(EditAnywhere, Category="Weapon Icon Capture|Post Process", meta=(ClampMin="0.0", ClampMax="2.0"))
	float ColorSaturation = 1.f;

private:
	bool CreateCaptureRig();
	void DestroyCaptureRig();
	void ApplyCaptureSettings();
	bool PrepareWeapon(USkeletalMesh* WeaponMesh);

	/** 색상과 깊이를 촬영해 출력 해상도로 축소한 BGRA 픽셀을 만든다. 알파는 서브픽셀 커버리지다. */
	bool ResolveIconPixels(const FString& AssetName, TArray<FColor>& OutPixels);

	bool ExportPng(const FString& AssetName, const TArray<FColor>& Pixels) const;
	UTexture2D* CreateOrUpdateTexture(const FString& AssetName, const TArray<FColor>& Pixels);
	UTexture2D* CaptureWeapon(const FString& AssetName, ULastFPSWeaponDefinition* WeaponDefinition);

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> ColorRenderTarget;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> DepthRenderTarget;

	UPROPERTY(Transient)
	TObjectPtr<ALastFPSWeaponCaptureRig> CaptureRig;

	/** 실제 촬영 해상도. 출력 해상도에 유효 슈퍼샘플 배수를 곱한 값이다. */
	int32 CaptureWidth = 0;
	int32 CaptureHeight = 0;

	/** 촬영 해상도 상한에 맞춰 낮춰진 실제 슈퍼샘플 배수다. */
	int32 EffectiveSupersample = 1;

	int32 LastCapturedCount = 0;
};
