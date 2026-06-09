#include "UI/LastFPSSettingsWidget.h"

#include "Game/LastFPSGameUserSettings.h"
#include "UI/LastFPSButtonBase.h"
#include "Components/Slider.h"

void ULastFPSSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_QualityLow)    Button_QualityLow->OnClicked().AddUObject(this,    &ULastFPSSettingsWidget::HandleQualityLowClicked);
	if (Button_QualityMedium) Button_QualityMedium->OnClicked().AddUObject(this, &ULastFPSSettingsWidget::HandleQualityMediumClicked);
	if (Button_QualityHigh)   Button_QualityHigh->OnClicked().AddUObject(this,   &ULastFPSSettingsWidget::HandleQualityHighClicked);
	if (Button_QualityUltra)  Button_QualityUltra->OnClicked().AddUObject(this,  &ULastFPSSettingsWidget::HandleQualityUltraClicked);
	if (Button_Apply)         Button_Apply->OnClicked().AddUObject(this,         &ULastFPSSettingsWidget::HandleApplyClicked);
	if (Button_Revert)        Button_Revert->OnClicked().AddUObject(this,        &ULastFPSSettingsWidget::HandleRevertClicked);

	LoadCurrentSettings();
}

void ULastFPSSettingsWidget::LoadCurrentSettings()
{
	const ULastFPSGameUserSettings* Settings = ULastFPSGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	// GetOverallScalabilityLevel()은 설정이 고르지 않으면 -1 반환 → 0으로 클램프
	PendingQualityLevel = FMath::Clamp(Settings->GetOverallScalabilityLevel(), 0, 3);
	OnQualityLevelChanged(PendingQualityLevel);

	if (Slider_MasterVolume) Slider_MasterVolume->SetValue(Settings->MasterVolume);
	if (Slider_MusicVolume)  Slider_MusicVolume->SetValue(Settings->MusicVolume);
	if (Slider_SFXVolume)    Slider_SFXVolume->SetValue(Settings->SFXVolume);
	if (Slider_Sensitivity)  Slider_Sensitivity->SetValue(Settings->MouseSensitivity);
}

void ULastFPSSettingsWidget::ApplyAndSave()
{
	ULastFPSGameUserSettings* Settings = ULastFPSGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	Settings->SetOverallScalabilityLevel(PendingQualityLevel);

	if (Slider_MasterVolume) Settings->MasterVolume      = Slider_MasterVolume->GetValue();
	if (Slider_MusicVolume)  Settings->MusicVolume       = Slider_MusicVolume->GetValue();
	if (Slider_SFXVolume)    Settings->SFXVolume         = Slider_SFXVolume->GetValue();
	if (Slider_Sensitivity)  Settings->MouseSensitivity  = Slider_Sensitivity->GetValue();

	Settings->ApplySettings(false);
	Settings->SaveSettings();

	OnAudioSettingsApplied(Settings->MasterVolume, Settings->MusicVolume, Settings->SFXVolume);
	OnSensitivityApplied(Settings->MouseSensitivity);
}

void ULastFPSSettingsWidget::RevertToSaved()
{
	// 저장된 값으로 UI를 다시 채움 (아직 Apply 안 한 변경 사항 버림)
	LoadCurrentSettings();
}

void ULastFPSSettingsWidget::SetGraphicsQuality(int32 QualityLevel)
{
	PendingQualityLevel = QualityLevel;
	OnQualityLevelChanged(PendingQualityLevel);
}

void ULastFPSSettingsWidget::OnQualityLevelChanged_Implementation(int32 NewLevel)
{
	// 선택된 버튼을 비활성화해서 '현재 선택' 표시. BP에서 스타일로 오버라이드 가능.
	if (Button_QualityLow)    Button_QualityLow->SetIsEnabled(NewLevel != 0);
	if (Button_QualityMedium) Button_QualityMedium->SetIsEnabled(NewLevel != 1);
	if (Button_QualityHigh)   Button_QualityHigh->SetIsEnabled(NewLevel != 2);
	if (Button_QualityUltra)  Button_QualityUltra->SetIsEnabled(NewLevel != 3);
}

void ULastFPSSettingsWidget::HandleQualityLowClicked()    { SetGraphicsQuality(0); }
void ULastFPSSettingsWidget::HandleQualityMediumClicked() { SetGraphicsQuality(1); }
void ULastFPSSettingsWidget::HandleQualityHighClicked()   { SetGraphicsQuality(2); }
void ULastFPSSettingsWidget::HandleQualityUltraClicked()  { SetGraphicsQuality(3); }

void ULastFPSSettingsWidget::HandleApplyClicked()  { ApplyAndSave(); }
void ULastFPSSettingsWidget::HandleRevertClicked() { RevertToSaved(); }
