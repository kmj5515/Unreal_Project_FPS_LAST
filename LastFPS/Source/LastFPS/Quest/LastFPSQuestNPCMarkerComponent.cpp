#include "Quest/LastFPSQuestNPCMarkerComponent.h"

#include "Quest/LastFPSQuestMarkerTarget.h"
#include "Quest/LastFPSQuestSubsystem.h"
#include "UI/HUD/Quest/LastFPSQuestNPCMarkerWidget.h"

#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSQuestNPCMarker, Log, All);

ULastFPSQuestNPCMarkerComponent::ULastFPSQuestNPCMarkerComponent()
{
	// 스크린 스페이스 위젯을 화면에 붙이는 경로가 TickComponent → UpdateWidget →
	// UpdateWidgetOnScreen → AddWidgetToScreen 뿐이라, 틱을 끄면 마커가 영영 그려지지 않는다.
	// 대신 TickMode(기본 Automatic)가 위젯이 안 보이는 동안 스스로 틱을 멈춘다.
	PrimaryComponentTick.bCanEverTick = true;

	SetWidgetSpace(EWidgetSpace::Screen);

	// 스크린 스페이스는 컴포넌트 등록 시점의 크기로 화면 레이어에 자리를 잡는다.
	// bDrawAtDesiredSize 를 켜면 그 시점 크기가 0이라 아무것도 그려지지 않는다.
	bDrawAtDesiredSize = false;
	SetDrawSize(FVector2D(MarkerDrawSize, MarkerDrawSize));

	// 컴포넌트 자체는 항상 보이는 상태로 등록해야 화면 레이어에 편입된다.
	// 표시/숨김은 위젯의 Slate 가시성으로만 전환한다(RefreshVisibility 참조).
	SetVisibility(true);
	SetHiddenInGame(false);
}

void ULastFPSQuestNPCMarkerComponent::SetMarkerHeight(const float InMarkerHeight)
{
	MarkerHeight = FMath::Max(0.f, InMarkerHeight);

	const FVector Placed = GetRelativeLocation();
	SetRelativeLocation(FVector(Placed.X, Placed.Y, MarkerHeight));
}

void ULastFPSQuestNPCMarkerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Z 는 프로퍼티가 단일 소스다(배치로 잡은 X/Y 오프셋은 유지).
	const FVector Placed = GetRelativeLocation();
	SetRelativeLocation(FVector(Placed.X, Placed.Y, MarkerHeight));

	// 이미 배치된 BP 컴포넌트는 예전 직렬화 값을 갖고 있으므로 여기서 한 번 더 맞춘다.
	// (스크린 스페이스는 자동 크기를 쓰면 등록 시점 크기가 0이라 아무것도 그려지지 않는다.)
	bDrawAtDesiredSize = false;
	SetDrawSize(FVector2D(MarkerDrawSize, MarkerDrawSize));

	MarkerNPCRowName = ResolveMarkerId();
	if (MarkerNPCRowName.IsNone())
	{
		// 키가 없으면 어떤 목표와도 대응시킬 수 없다. 매 프레임이 아니라 여기서 한 번만 알린다.
		UE_LOG(
			LogLastFPSQuestNPCMarker,
			Warning,
			TEXT("[Quest] NPC 마커(%s)가 NPC 행 이름을 얻지 못해 마커를 띄우지 않습니다. ")
			TEXT("오너가 ILastFPSQuestMarkerTarget 을 구현하지 않았거나 행 이름이 비어 있습니다."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("Owner 없음"));
	}
	else if (ULastFPSQuestSubsystem* Quest = ULastFPSQuestSubsystem::Get(this))
	{
		CachedQuestSubsystem = Quest;
		Quest->OnQuestStateChanged.AddUniqueDynamic(this, &ULastFPSQuestNPCMarkerComponent::HandleQuestStateChanged);
	}
	else
	{
		UE_LOG(
			LogLastFPSQuestNPCMarker,
			Warning,
			TEXT("[Quest] NPC 마커(%s)가 QuestSubsystem 을 찾지 못해 '%s' 표시 갱신을 구독하지 못했습니다."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("Owner 없음"),
			*MarkerNPCRowName.ToString());
	}

	// 실패 경로도 여기로 합류한다 — 대상이 없으면 심볼 None 으로 접히는 것이 올바른 결과다.
	// (이 NPC 가 생성되기 전에 이미 추적이 걸려 있었을 수 있어 현재 상태로 한 번 맞춘다.)
	RefreshVisibility();
}

void ULastFPSQuestNPCMarkerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ULastFPSQuestSubsystem* Quest = CachedQuestSubsystem.Get())
	{
		Quest->OnQuestStateChanged.RemoveDynamic(this, &ULastFPSQuestNPCMarkerComponent::HandleQuestStateChanged);
	}
	CachedQuestSubsystem.Reset();

	Super::EndPlay(EndPlayReason);
}

void ULastFPSQuestNPCMarkerComponent::HandleQuestStateChanged()
{
	RefreshVisibility();
}

void ULastFPSQuestNPCMarkerComponent::RefreshVisibility()
{
	FLastFPSQuestNPCMarkerInfo Info;

	if (!MarkerNPCRowName.IsNone())
	{
		if (const ULastFPSQuestSubsystem* Quest = CachedQuestSubsystem.Get())
		{
			// 표시 판정은 서브시스템이 상태 변경 때 한 번만 하고, 여기서는 그 결과만 읽는다.
			Info = Quest->GetNPCMarkerInfo(MarkerNPCRowName);
		}
	}

	// 상태 변경은 잦지만 이 NPC 의 마커가 실제로 바뀌는 일은 드물다.
	// 같은 상태를 다시 적용하면 브러시 교체·머티리얼 갱신·BP 이벤트가 NPC 수만큼 헛돈다.
	if (bHasAppliedInfo && Info == AppliedInfo)
	{
		return;
	}

	const bool bShouldShow = Info.Symbol != ELastFPSQuestNPCMarkerSymbol::None;

	// 컴포넌트 가시성은 건드리지 않는다. 스크린 스페이스 위젯 컴포넌트는 숨긴 채 등록되면
	// 화면 레이어에 편입되지 않아, 나중에 다시 켜도 영영 그려지지 않는다.
	UUserWidget* MarkerUserWidget = GetUserWidgetObject();
	if (!MarkerUserWidget)
	{
		return;
	}

	MarkerUserWidget->SetVisibility(bShouldShow
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed);

	if (bShouldShow)
	{
		if (ULastFPSQuestNPCMarkerWidget* MarkerWidget = Cast<ULastFPSQuestNPCMarkerWidget>(MarkerUserWidget))
		{
			MarkerWidget->ApplyMarkerState(Info.Symbol, Info.bTracked);
		}
	}

	AppliedInfo = Info;
	bHasAppliedInfo = true;
}

FName ULastFPSQuestNPCMarkerComponent::ResolveMarkerId() const
{
	AActor* Owner = GetOwner();
	if (Owner && Owner->Implements<ULastFPSQuestMarkerTarget>())
	{
		return ILastFPSQuestMarkerTarget::Execute_GetQuestMarkerId(Owner);
	}

	return NAME_None;
}
