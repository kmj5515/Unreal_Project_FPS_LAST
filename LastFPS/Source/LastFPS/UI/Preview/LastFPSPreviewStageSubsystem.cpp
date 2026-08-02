#include "UI/Preview/LastFPSPreviewStageSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UI/Preview/LastFPSPreviewStageActor.h"
#include "UI/Theme/LastFPSUISettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSPreviewStage, Log, All);

namespace
{
	/** 플레이어가 닿지 않는 높이. 무대가 실제 레벨 지오메트리와 겹치지 않게 한다. */
	constexpr float PreviewStageWorldHeight = 100000.f;
}

ULastFPSPreviewStageSubsystem* ULastFPSPreviewStageSubsystem::Get(const UObject* WorldContext)
{
	if (!GEngine || !WorldContext)
	{
		return nullptr;
	}

	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull))
	{
		return World->GetSubsystem<ULastFPSPreviewStageSubsystem>();
	}
	return nullptr;
}

bool ULastFPSPreviewStageSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// 에디터 프리뷰 월드까지 무대를 만들면 툴 작업 중에 쓸모없는 액터가 생긴다.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void ULastFPSPreviewStageSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// 레벨이 시작할 때 미리 만들어 둔다. 화면을 여는 프레임에 스폰 비용이 남지 않게 하는 것이 목적이다.
	GetStage();
}

void ULastFPSPreviewStageSubsystem::Deinitialize()
{
	if (Stage)
	{
		Stage->Destroy();
		Stage = nullptr;
	}
	StageUserCount = 0;

	Super::Deinitialize();
}

ALastFPSPreviewStageActor* ULastFPSPreviewStageSubsystem::GetStage()
{
	if (Stage)
	{
		return Stage;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// 어떤 무대를 쓸지는 프로젝트 설정이 정한다. 지정이 없으면 C++ 베이스로 폴백한다.
	UClass* StageClass = ALastFPSPreviewStageActor::StaticClass();
	if (const ULastFPSUISettings* Settings = ULastFPSUISettings::Get())
	{
		if (!Settings->PreviewStageClass.IsNull())
		{
			if (UClass* ConfiguredClass = Settings->PreviewStageClass.LoadSynchronous())
			{
				StageClass = ConfiguredClass;
			}
			else
			{
				UE_LOG(LogLastFPSPreviewStage, Warning,
					TEXT("프리뷰 무대 클래스를 불러오지 못해 기본 클래스를 사용합니다: %s"),
					*Settings->PreviewStageClass.ToString());
			}
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.ObjectFlags = RF_Transient;
	Stage = World->SpawnActor<ALastFPSPreviewStageActor>(
		StageClass,
		FVector(0.f, 0.f, PreviewStageWorldHeight),
		FRotator::ZeroRotator,
		SpawnParameters);

	if (!Stage)
	{
		UE_LOG(LogLastFPSPreviewStage, Error,
			TEXT("프리뷰 무대를 스폰하지 못했습니다: 클래스=%s"), *GetNameSafe(StageClass));
		return nullptr;
	}

	ApplyActiveState();
	return Stage;
}

void ULastFPSPreviewStageSubsystem::AddStageUser()
{
	++StageUserCount;
	ApplyActiveState();
}

void ULastFPSPreviewStageSubsystem::RemoveStageUser()
{
	if (StageUserCount <= 0)
	{
		// 짝이 맞지 않으면 무대가 계속 켜져 있거나 너무 일찍 꺼진다. 조용히 넘기지 않는다.
		UE_LOG(LogLastFPSPreviewStage, Warning,
			TEXT("프리뷰 무대 사용자 해제가 등록보다 많습니다. AddStageUser 와 짝을 확인하세요."));
		StageUserCount = 0;
		return;
	}

	--StageUserCount;
	ApplyActiveState();
}

void ULastFPSPreviewStageSubsystem::ApplyActiveState()
{
	if (Stage)
	{
		Stage->SetStageActive(StageUserCount > 0);
	}
}
