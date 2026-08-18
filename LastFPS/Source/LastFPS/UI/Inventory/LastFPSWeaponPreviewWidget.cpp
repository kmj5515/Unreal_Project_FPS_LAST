#include "UI/Inventory/LastFPSWeaponPreviewWidget.h"

#include "UI/Preview/LastFPSPreviewStageActor.h"
#include "UI/Preview/LastFPSPreviewStageSubsystem.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "Localization/LastFPSLocalization.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "UI/Framework/LastFPSButtonBase.h"
#include "UI/Framework/LastFPSUITags.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void ULastFPSWeaponPreviewWidget::Setup(ULastFPSWeaponDefinition* InWeaponDef, const FLastFPSItemData& InItem, FName InRowId, ALastFPSPreviewStageActor* InStage)
{
	WeaponDef = InWeaponDef;
	CachedItem = InItem;
	CachedRowId = InRowId;
	PreviewStage = InStage;

	// Setup 은 push 시점(위젯 construct 이전일 수 있음)에 호출된다. 실제 텍스트는
	// 바인딩이 끝난 NativeConstruct 에서 채운다. 이미 construct 됐다면 즉시 반영.
	if (IsConstructed())
	{
		PopulateStats();
		BindPreviewStage();
	}
}

void ULastFPSWeaponPreviewWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 이 화면이 뜨면 아래 껍데기는 같은 레이어 스택이라 비활성화된다. 껍데기가 쥐고 있던 무대도 함께
	// 놓이므로, 여기서 따로 쥐지 않으면 정작 무기를 보여줄 때 무대가 꺼져 있다.
	if (ULastFPSPreviewStageSubsystem* PreviewStageSubsystem = ULastFPSPreviewStageSubsystem::Get(this))
	{
		PreviewStageSubsystem->AddStageUser();
		bHoldingPreviewStage = true;
	}

	PopulateStats();
	BindPreviewStage();
}

void ULastFPSWeaponPreviewWidget::HandleCloseClicked()
{
	DeactivateWidget(); // Modal 레이어에서 자신을 pop (ESC 와 동일).
}

void ULastFPSWeaponPreviewWidget::NativeDestruct()
{
	// 프리뷰를 닫으면 원래 뷰타깃(폰 카메라 등)으로 복구한다.
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bIsUsingStreamingVolumes = true;
		if (AActor* Prev = PrevViewTarget.Get())
		{
			PC->SetViewTarget(Prev);
		}
	}
	PrevViewTarget = nullptr;

	// 무대는 레벨이 소유·재사용 → 여기서 파괴하지 않는다. 다음 화면이 다른 대상을 올려 그대로 쓴다.
	// 무대에 올려 둔 무기도 치우지 않는다. 껍데기가 다시 활성화되면서 캐릭터로 되돌린다.
	PreviewStage = nullptr;

	if (bHoldingPreviewStage)
	{
		if (ULastFPSPreviewStageSubsystem* PreviewStageSubsystem = ULastFPSPreviewStageSubsystem::Get(this))
		{
			PreviewStageSubsystem->RemoveStageUser();
		}
		bHoldingPreviewStage = false;
	}

	Super::NativeDestruct();
}

void ULastFPSWeaponPreviewWidget::BindPreviewStage()
{
	if (!PreviewStage || !WeaponDef)
	{
		return;
	}

	USkeletalMesh* Mesh = WeaponDef->SkeletalMesh.LoadSynchronous();
	if (!Mesh)
	{
		return;
	}

	// 무기는 정지 상태라 애님 인스턴스 없이 레퍼런스 포즈로 충분하다.
	FLastFPSPreviewSubject Subject;
	Subject.Mesh = Mesh;

	// 무기 자리에만 올린다. 캐릭터 자리는 건드리지 않으므로 닫을 때 다시 조립할 필요가 없다.
	PreviewStage->SetSubject(LastFPSUITags::PreviewSlot_Weapon(), Subject);
	PreviewStage->SetActiveView(LastFPSUITags::PreviewView_Weapon());

	// 플레이어 뷰를 무대 카메라로 전환(SceneCapture 대신). 닫을 때 복구할 이전 뷰타깃을 캐시.
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (!PrevViewTarget.IsValid())
		{
			PrevViewTarget = PC->GetViewTarget();
		}
		PC->SetViewTarget(PreviewStage);

		// 무대는 레벨 지오메트리를 피해 Z=100000 에 있다. 스트리밍 볼륨은 뷰타깃 위치로 판정하므로
		// 그대로 두면 프리뷰가 열린 동안 볼륨이 쥐고 있던 서브레벨이 통째로 언로드된다.
		PC->bIsUsingStreamingVolumes = false;
	}
}

FReply ULastFPSWeaponPreviewWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && PreviewStage)
	{
		bDragging = true;

		// 마우스를 캡처하면 키보드 포커스가 이 위젯에서 빠져 ESC 가 NativeOnKeyDown 까지 오지 않는다.
		// 캡처와 함께 포커스를 이 위젯으로 되돌려 둔다.
		return FReply::Handled()
			.CaptureMouse(TakeWidget())
			.SetUserFocus(TakeWidget(), EFocusCause::Mouse);
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULastFPSWeaponPreviewWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging && PreviewStage)
	{
		const float DeltaYaw = -InMouseEvent.GetCursorDelta().X * RotationSpeed;
		if (PreviewStage->AddYaw(LastFPSUITags::PreviewSlot_Weapon(), DeltaYaw))
		{
			DraggedYaw += DeltaYaw;
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply ULastFPSWeaponPreviewWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bDragging)
	{
		bDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void ULastFPSWeaponPreviewWidget::NativeTick(const FGeometry& InGeometry, const float InDeltaTime)
{
	Super::NativeTick(InGeometry, InDeltaTime);

	// 놓은 뒤에만 복귀시킨다. 드래그 중이면 사용자가 잡고 있는 각도가 우선이다.
	if (bDragging || !PreviewStage || FMath::IsNearlyZero(DraggedYaw))
	{
		return;
	}

	// FInterpTo 는 목표에 충분히 가까우면 정확히 0 을 돌려주므로 잔여 각도가 남지 않는다.
	const float NextYaw = FMath::FInterpTo(DraggedYaw, 0.f, InDeltaTime, ReturnInterpSpeed);
	PreviewStage->AddYaw(LastFPSUITags::PreviewSlot_Weapon(), NextYaw - DraggedYaw);
	DraggedYaw = NextYaw;
}

void ULastFPSWeaponPreviewWidget::PopulateStats()
{
	if (TB_WeaponName)
	{
		TB_WeaponName->SetText(CachedItem.ItemName);
	}
	if (TB_Rarity)
	{
		TB_Rarity->SetText(FLastFPSLocalization::GetUIEnumText(
			StaticEnum<ELastFPSItemRarity>(),
			static_cast<int64>(CachedItem.Rarity)));
	}
	if (TB_Description)
	{
		TB_Description->SetText(CachedItem.Description);
	}

	// 타입 표시는 무기 정의(분류)에서 — 정의가 있을 때만.
	if (TB_Type && WeaponDef)
	{
		TB_Type->SetText(FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponTypeFormat),
			FLastFPSLocalization::GetUIEnumText(
				StaticEnum<EMMWeaponType>(),
				static_cast<int64>(WeaponDef->WeaponType))));
	}

	// 스탯 시트는 아이템 행의 표시용 스펙(DT_ItemData.WeaponStats)에서 읽는다.
	// 라벨/값 두 열로 빌드(줄 수를 맞춰 정렬). 그룹 헤더 줄은 값 열이 비어 있다.
	const FLastFPSWeaponDisplayStats& S = CachedItem.WeaponStats;

	FNumberFormattingOptions ZeroDecimals;
	ZeroDecimals.SetMinimumFractionalDigits(0);
	ZeroDecimals.SetMaximumFractionalDigits(0);
	FNumberFormattingOptions OneDecimal;
	OneDecimal.SetMinimumFractionalDigits(1);
	OneDecimal.SetMaximumFractionalDigits(1);
	FNumberFormattingOptions TwoDecimals;
	TwoDecimals.SetMinimumFractionalDigits(2);
	TwoDecimals.SetMaximumFractionalDigits(2);

	TArray<FText> Labels;
	TArray<FText> Values;
	auto AddLine = [&Labels, &Values](const FText& Label, const FText& Value)
	{
		Labels.Add(Label);
		Values.Add(Value);
	};
	auto Head = [&AddLine](const TCHAR* Key)
	{
		AddLine(
			FText::Format(
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::SectionHeaderFormat),
				FLastFPSLocalization::GetUIText(Key)),
			FText::GetEmpty());
	};
	auto Pct = [&ZeroDecimals](const float Value)
	{
		return FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::PercentFormat),
			FText::AsNumber(Value * 100.f, &ZeroDecimals));
	};
	auto Mult = [&OneDecimal](const float Value)
	{
		return FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::MultiplierFormat),
			FText::AsNumber(Value, &OneDecimal));
	};
	auto Mtr = [&ZeroDecimals](const float Centimeters)
	{
		return FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::DistanceMetersFormat),
			FText::AsNumber(Centimeters / 100.f, &ZeroDecimals));
	};

	Head(LastFPSUIStringKeys::WeaponStatsAttackSection);
	AddLine(
		FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsDamage),
		FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::RangeFormat),
			FText::AsNumber(S.MinDamage, &ZeroDecimals),
			FText::AsNumber(S.MaxDamage, &ZeroDecimals)));
	AddLine(
		FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsFireRate),
		FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::SecondsFormat),
			FText::AsNumber(S.FireRate, &TwoDecimals)));
	AddLine(
		FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsMagazineSize),
		FText::AsNumber(S.MagazineSize));
	AddLine(
		FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsReloadTime),
		FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::SecondsFormat),
			FText::AsNumber(S.ReloadTime, &OneDecimal)));
	AddLine(
		FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsPenetration),
		FText::AsNumber(S.Penetration, &OneDecimal));

	Head(LastFPSUIStringKeys::WeaponStatsCriticalSection);
	AddLine(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsCriticalChance), Pct(S.CriticalChance));
	AddLine(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsCriticalMultiplier), Mult(S.CriticalMultiplier));
	AddLine(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsWeakpointMultiplier), Mult(S.WeakpointMultiplier));
	AddLine(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsMultiHitMultiplier), Mult(S.MultiHitMultiplier));
	AddLine(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsStatusEffectChance), Pct(S.StatusEffectChance));
	AddLine(
		FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsBreakPower),
		FText::AsNumber(S.BreakPower, &ZeroDecimals));

	Head(LastFPSUIStringKeys::WeaponStatsAccuracySection);
	AddLine(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsHipFireAccuracy), Pct(S.HipFireAccuracy));
	AddLine(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsAdsAccuracy), Pct(S.AdsAccuracy));
	AddLine(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsEffectiveRange), Mtr(S.EffectiveRange));
	AddLine(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsMaxRange), Mtr(S.MaxRange));
	AddLine(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsDamageFalloff), Pct(S.DamageFalloffPercent));

	Head(LastFPSUIStringKeys::WeaponStatsMobilitySection);
	AddLine(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::StatMoveSpeed), FText::AsNumber(S.MoveSpeed, &ZeroDecimals));
	AddLine(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsMoveWhileFiring), FText::AsNumber(S.MoveSpeedWhileFiring, &ZeroDecimals));
	AddLine(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsMoveWhileAiming), FText::AsNumber(S.MoveSpeedWhileAiming, &ZeroDecimals));
	AddLine(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::WeaponStatsSprintSpeed), FText::AsNumber(S.SprintSpeed, &ZeroDecimals));

	const FText LineBreak = FText::FromString(TEXT("\n"));

	if (TB_StatLabels)
	{
		TB_StatLabels->SetText(FText::Join(LineBreak, Labels));
	}
	if (TB_StatValues)
	{
		TB_StatValues->SetText(FText::Join(LineBreak, Values));
	}
}
