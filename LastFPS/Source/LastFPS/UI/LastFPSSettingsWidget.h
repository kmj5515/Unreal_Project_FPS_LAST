#pragma once

#include "UI/LastFPSContentScreenWidget.h"
#include "LastFPSSettingsWidget.generated.h"

class USlider;
class UTextBlock;
class ULastFPSButtonBase;

/**
 * 설정 화면 — 그래픽 품질 / 사운드 볼륨 / 마우스 감도.
 * Apply 버튼을 눌러야 GameUserSettings에 저장됨.
 * 오디오·감도 실제 적용은 BlueprintImplementableEvent(OnAudioSettingsApplied / OnSensitivityApplied)로
 * WBP_Settings BP에서 프로젝트 사운드 클래스·Enhanced Input Modifier와 연결한다.
 */
UCLASS()
class LASTFPS_API ULastFPSSettingsWidget : public ULastFPSContentScreenWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// ── 그래픽 품질 버튼 (0=Low / 1=Medium / 2=High / 3=Ultra) ──────
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_QualityLow;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_QualityMedium;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_QualityHigh;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_QualityUltra;

	// ── 볼륨 슬라이더 (0.0 ~ 1.0) ────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<USlider> Slider_MasterVolume;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<USlider> Slider_MusicVolume;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<USlider> Slider_SFXVolume;

	// ── 마우스 감도 슬라이더 (0.0 ~ 1.0 정규화) ──────────────────────
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<USlider> Slider_Sensitivity;

	// ── 슬라이더 옆 수치 표시 텍스트 (백분율, 선택 바인딩) ────────────
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_MasterVolume;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_MusicVolume;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_SFXVolume;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Sensitivity;

	// ── 적용 / 되돌리기 ───────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Apply;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Revert;

	/**
	 * 그래픽 품질 레벨 선택이 바뀔 때 호출.
	 * 기본 구현: 선택된 버튼 비활성화로 강조. BP에서 스타일 오버라이드 가능.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|Settings")
	void OnQualityLevelChanged(int32 NewLevel);
	virtual void OnQualityLevelChanged_Implementation(int32 NewLevel);

	/** Apply 시 호출 — BP에서 Sound Class / Sound Mix에 볼륨 연결 */
	UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Settings")
	void OnAudioSettingsApplied(float InMasterVolume, float InMusicVolume, float InSFXVolume);

	/** Apply 시 호출 — BP에서 Enhanced Input Modifier 스케일에 연결 */
	UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Settings")
	void OnSensitivityApplied(float InSensitivity);

private:
	void LoadCurrentSettings();
	void ApplyAndSave();
	void RevertToSaved();
	void SetGraphicsQuality(int32 QualityLevel);

	/** 슬라이더 값을 옆 텍스트(백분율)에 반영 */
	void RefreshSliderLabels();

	/** 슬라이더가 움직일 때마다 옆 수치 텍스트 갱신 (어느 슬라이더든 전체 재반영) */
	UFUNCTION() void HandleSliderValueChanged(float Value);

	UFUNCTION() void HandleQualityLowClicked();
	UFUNCTION() void HandleQualityMediumClicked();
	UFUNCTION() void HandleQualityHighClicked();
	UFUNCTION() void HandleQualityUltraClicked();
	UFUNCTION() void HandleApplyClicked();
	UFUNCTION() void HandleRevertClicked();

	// Apply 전까지 보관하는 그래픽 품질 (슬라이더는 위젯 값을 직접 읽음)
	int32 PendingQualityLevel = 2;
};
