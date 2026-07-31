#include "UI/HUD/Audio/LastFPSRadioAudioPlayer.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundEffectSource.h"
#include "SourceEffects/SourceEffectBitCrusher.h"
#include "SourceEffects/SourceEffectDynamicsProcessor.h"
#include "SourceEffects/SourceEffectFilter.h"
#include "SourceEffects/SourceEffectWaveShaper.h"
#include "UI/HUD/Audio/LastFPSRadioAudioSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSRadioAudio, Log, All);

void ULastFPSRadioAudioPlayer::Initialize(UObject* InWorldContext)
{
	WorldContext = InWorldContext;
	BuildEffectChain();
}

void ULastFPSRadioAudioPlayer::PlayTransmissionStart()
{
	const ULastFPSRadioAudioSettings* Settings = ULastFPSRadioAudioSettings::Get();
	PlaySignal(Settings->TransmissionOpenSound);
}

void ULastFPSRadioAudioPlayer::PlayVoice(const TSoftObjectPtr<USoundBase>& VoiceSound)
{
	Stop();

	if (VoiceSound.IsNull() || !WorldContext.IsValid())
	{
		return;
	}

	USoundBase* LoadedVoice = VoiceSound.LoadSynchronous();
	if (!IsValid(LoadedVoice))
	{
		UE_LOG(
			LogLastFPSRadioAudio,
			Warning,
			TEXT("무전 음성을 불러오지 못했습니다. Asset=%s"),
			*VoiceSound.ToSoftObjectPath().ToString());
		return;
	}

	const ULastFPSRadioAudioSettings* Settings = ULastFPSRadioAudioSettings::Get();
	UAudioComponent* AudioComponent = UGameplayStatics::CreateSound2D(
		WorldContext.Get(),
		LoadedVoice,
		FMath::Max(0.0f, Settings->VoiceVolume),
		1.0f,
		0.0f,
		nullptr,
		false,
		true);

	if (!IsValid(AudioComponent))
	{
		UE_LOG(
			LogLastFPSRadioAudio,
			Warning,
			TEXT("무전 음성 컴포넌트를 생성하지 못했습니다. Sound=%s"),
			*GetNameSafe(LoadedVoice));
		return;
	}

	if (Settings->bEnableRadioEffect && IsValid(RadioEffectChain))
	{
		AudioComponent->SetSourceEffectChain(RadioEffectChain);
	}

	ActiveVoiceAudioComponent = AudioComponent;
	AudioComponent->Play();
}

void ULastFPSRadioAudioPlayer::PlayTransmissionEnd()
{
	const ULastFPSRadioAudioSettings* Settings = ULastFPSRadioAudioSettings::Get();
	PlaySignal(Settings->TransmissionCloseSound);
}

void ULastFPSRadioAudioPlayer::Stop()
{
	if (ActiveVoiceAudioComponent.IsValid())
	{
		ActiveVoiceAudioComponent->Stop();
	}
	ActiveVoiceAudioComponent.Reset();
}

void ULastFPSRadioAudioPlayer::BuildEffectChain()
{
	RadioEffectChain = nullptr;
	EffectPresets.Reset();

	const ULastFPSRadioAudioSettings* Settings = ULastFPSRadioAudioSettings::Get();
	if (!Settings->bEnableRadioEffect)
	{
		return;
	}

	const float HighPassFrequency = FMath::Clamp(
		Settings->HighPassCutoffFrequency,
		20.0f,
		19980.0f);
	const float LowPassFrequency = FMath::Clamp(
		Settings->LowPassCutoffFrequency,
		HighPassFrequency + 20.0f,
		20000.0f);
	const float FilterQ = FMath::Clamp(Settings->FilterQ, 0.5f, 10.0f);

	USourceEffectFilterPreset* HighPassPreset = NewObject<USourceEffectFilterPreset>(this);
	FSourceEffectFilterSettings HighPassSettings;
	HighPassSettings.FilterCircuit = ESourceEffectFilterCircuit::StateVariable;
	HighPassSettings.FilterType = ESourceEffectFilterType::HighPass;
	HighPassSettings.CutoffFrequency = HighPassFrequency;
	HighPassSettings.FilterQ = FilterQ;
	HighPassPreset->SetSettings(HighPassSettings);
	EffectPresets.Add(HighPassPreset);

	USourceEffectFilterPreset* LowPassPreset = NewObject<USourceEffectFilterPreset>(this);
	FSourceEffectFilterSettings LowPassSettings;
	LowPassSettings.FilterCircuit = ESourceEffectFilterCircuit::StateVariable;
	LowPassSettings.FilterType = ESourceEffectFilterType::LowPass;
	LowPassSettings.CutoffFrequency = LowPassFrequency;
	LowPassSettings.FilterQ = FilterQ;
	LowPassPreset->SetSettings(LowPassSettings);
	EffectPresets.Add(LowPassPreset);

	if (Settings->bEnableDistortion)
	{
		USourceEffectWaveShaperPreset* DistortionPreset =
			NewObject<USourceEffectWaveShaperPreset>(this);
		FSourceEffectWaveShaperSettings DistortionSettings;
		DistortionSettings.Amount = FMath::Clamp(
			Settings->DistortionAmount,
			0.0f,
			500.0f);
		DistortionSettings.OutputGainDb = FMath::Clamp(
			Settings->DistortionOutputGainDb,
			-60.0f,
			20.0f);
		DistortionPreset->SetSettings(DistortionSettings);
		EffectPresets.Add(DistortionPreset);
	}

	USourceEffectDynamicsProcessorPreset* CompressorPreset =
		NewObject<USourceEffectDynamicsProcessorPreset>(this);
	FSourceEffectDynamicsProcessorSettings CompressorSettings;
	CompressorSettings.DynamicsProcessorType = ESourceEffectDynamicsProcessorType::Compressor;
	CompressorSettings.PeakMode = ESourceEffectDynamicsPeakMode::RootMeanSquared;
	CompressorSettings.ThresholdDb = FMath::Clamp(
		Settings->CompressorThresholdDb,
		-60.0f,
		0.0f);
	CompressorSettings.Ratio = FMath::Clamp(Settings->CompressorRatio, 1.0f, 20.0f);
	CompressorSettings.AttackTimeMsec = FMath::Clamp(
		Settings->CompressorAttackTimeMsec,
		1.0f,
		300.0f);
	CompressorSettings.ReleaseTimeMsec = FMath::Clamp(
		Settings->CompressorReleaseTimeMsec,
		20.0f,
		5000.0f);
	CompressorSettings.OutputGainDb = FMath::Clamp(
		Settings->CompressorOutputGainDb,
		0.0f,
		20.0f);
	CompressorPreset->SetSettings(CompressorSettings);
	EffectPresets.Add(CompressorPreset);

	if (Settings->bEnableBitCrusher)
	{
		USourceEffectBitCrusherPreset* BitCrusherPreset =
			NewObject<USourceEffectBitCrusherPreset>(this);
		FSourceEffectBitCrusherBaseSettings BitCrusherSettings;
		BitCrusherSettings.SampleRate = FMath::Clamp(
			Settings->BitCrusherSampleRate,
			500.0f,
			96000.0f);
		BitCrusherSettings.BitDepth = FMath::Clamp(
			Settings->BitCrusherBitDepth,
			1.0f,
			24.0f);
		BitCrusherPreset->SetSettings(BitCrusherSettings);
		EffectPresets.Add(BitCrusherPreset);
	}

	RadioEffectChain = NewObject<USoundEffectSourcePresetChain>(this);
	RadioEffectChain->bPlayEffectChainTails = false;
	RadioEffectChain->Chain.Reserve(EffectPresets.Num());

	for (USoundEffectSourcePreset* Preset : EffectPresets)
	{
		FSourceEffectChainEntry& Entry = RadioEffectChain->Chain.AddDefaulted_GetRef();
		Entry.Preset = Preset;
		Entry.bBypass = false;
	}
}

void ULastFPSRadioAudioPlayer::PlaySignal(const TSoftObjectPtr<USoundBase>& SignalSound)
{
	if (SignalSound.IsNull() || !WorldContext.IsValid())
	{
		return;
	}

	USoundBase* LoadedSignal = SignalSound.LoadSynchronous();
	if (!IsValid(LoadedSignal))
	{
		UE_LOG(
			LogLastFPSRadioAudio,
			Warning,
			TEXT("무전 송신 신호음을 불러오지 못했습니다. Asset=%s"),
			*SignalSound.ToSoftObjectPath().ToString());
		return;
	}

	const ULastFPSRadioAudioSettings* Settings = ULastFPSRadioAudioSettings::Get();
	UGameplayStatics::SpawnSound2D(
		WorldContext.Get(),
		LoadedSignal,
		FMath::Max(0.0f, Settings->TransmissionSignalVolume));
}
