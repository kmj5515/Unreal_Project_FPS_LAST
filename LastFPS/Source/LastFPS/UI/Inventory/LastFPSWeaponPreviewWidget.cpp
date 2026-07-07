#include "UI/Inventory/LastFPSWeaponPreviewWidget.h"

#include "UI/Inventory/LastFPSWeaponPreviewRig.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "UI/Framework/LastFPSButtonBase.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"

void ULastFPSWeaponPreviewWidget::Setup(ULastFPSWeaponDefinition* InWeaponDef, const FLastFPSItemData& InItem, FName InRowId, ALastFPSWeaponPreviewRig* InRig)
{
	WeaponDef = InWeaponDef;
	CachedItem = InItem;
	CachedRowId = InRowId;
	Rig = InRig;

	// Setup 은 push 시점(위젯 construct 이전일 수 있음)에 호출된다. 실제 텍스트는
	// 바인딩이 끝난 NativeConstruct 에서 채운다. 이미 construct 됐다면 즉시 반영.
	if (IsConstructed())
	{
		PopulateStats();
		BindPreviewRig();
	}
}

void ULastFPSWeaponPreviewWidget::NativeConstruct()
{
	Super::NativeConstruct();
	PopulateStats();
	BindPreviewRig();

	if (Btn_Close)
	{
		Btn_Close->OnClicked().AddUObject(this, &ULastFPSWeaponPreviewWidget::HandleCloseClicked);
	}
}

void ULastFPSWeaponPreviewWidget::HandleCloseClicked()
{
	DeactivateWidget(); // Modal 레이어에서 자신을 pop (ESC 와 동일).
}

void ULastFPSWeaponPreviewWidget::NativeDestruct()
{
	// 리그는 인벤토리가 소유·재사용 → 여기서 파괴하지 않는다.
	Rig = nullptr;

	Super::NativeDestruct();
}

void ULastFPSWeaponPreviewWidget::BindPreviewRig()
{
	if (!Rig || !WeaponDef || !Image_Preview)
	{
		return;
	}

	USkeletalMesh* Mesh = WeaponDef->SkeletalMesh;
	if (!Mesh)
	{
		return;
	}

	if (UTextureRenderTarget2D* RT = Rig->InitPreview(Mesh))
	{
		Image_Preview->SetBrushResourceObject(RT);
	}
}

FReply ULastFPSWeaponPreviewWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Rig)
	{
		bDragging = true;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULastFPSWeaponPreviewWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging && Rig)
	{
		Rig->AddYaw(-InMouseEvent.GetCursorDelta().X * RotationSpeed);
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

void ULastFPSWeaponPreviewWidget::PopulateStats()
{
	if (TB_WeaponName)
	{
		TB_WeaponName->SetText(CachedItem.ItemName);
	}
	if (TB_Rarity)
	{
		TB_Rarity->SetText(UEnum::GetDisplayValueAsText(CachedItem.Rarity));
	}
	if (TB_Description)
	{
		TB_Description->SetText(CachedItem.Description);
	}

	// 타입 표시는 무기 정의(분류)에서 — 정의가 있을 때만.
	if (TB_Type && WeaponDef)
	{
		TB_Type->SetText(FText::Format(
			NSLOCTEXT("LastFPS", "WeaponPreview_Type", "타입: {0}"),
			UEnum::GetDisplayValueAsText(WeaponDef->WeaponType)));
	}

	// 스탯 시트는 아이템 행의 표시용 스펙(DT_ItemData.WeaponStats)에서 읽는다.
	// 라벨/값 두 열로 빌드(줄 수를 맞춰 정렬). 그룹 헤더 줄은 값 열이 비어 있다.
	const FLastFPSWeaponDisplayStats& S = CachedItem.WeaponStats;

	FString Labels;
	FString Values;
	bool bFirst = true;
	auto AddLine = [&Labels, &Values, &bFirst](const FString& Label, const FString& Value)
	{
		if (!bFirst)
		{
			Labels += TEXT("\n");
			Values += TEXT("\n");
		}
		bFirst = false;
		Labels += Label;
		Values += Value;
	};
	auto Head = [&AddLine](const TCHAR* Title) { AddLine(FString(TEXT("■ ")) + Title, FString()); };
	auto Pct  = [](float V) { return FString::Printf(TEXT("%d%%"), FMath::RoundToInt(V * 100.f)); };
	auto Mult = [](float V) { return FString::Printf(TEXT("x%.1f"), V); };
	auto Mtr  = [](float Cm) { return FString::Printf(TEXT("%.0fm"), Cm / 100.f); };

	Head(TEXT("공격"));
	AddLine(TEXT("  공격력"),        FString::Printf(TEXT("%d ~ %d"), FMath::RoundToInt(S.MinDamage), FMath::RoundToInt(S.MaxDamage)));
	AddLine(TEXT("  발사 속도"),      FString::Printf(TEXT("%.2fs"), S.FireRate));
	AddLine(TEXT("  장탄량"),        FString::FromInt(S.MagazineSize));
	AddLine(TEXT("  재장전 시간"),    FString::Printf(TEXT("%.1fs"), S.ReloadTime));
	AddLine(TEXT("  관통력"),        FString::Printf(TEXT("%.1f"), S.Penetration));

	Head(TEXT("치명타·속성"));
	AddLine(TEXT("  치명타 확률"),      Pct(S.CriticalChance));
	AddLine(TEXT("  치명타 배율"),      Mult(S.CriticalMultiplier));
	AddLine(TEXT("  약점 배율"),       Mult(S.WeakpointMultiplier));
	AddLine(TEXT("  다중 타격 배율"),   Mult(S.MultiHitMultiplier));
	AddLine(TEXT("  상태효과 부여확률"), Pct(S.StatusEffectChance));
	AddLine(TEXT("  격파"),          FString::Printf(TEXT("%.0f"), S.BreakPower));

	Head(TEXT("명중·사거리"));
	AddLine(TEXT("  지향 정확도"),   Pct(S.HipFireAccuracy));
	AddLine(TEXT("  조준 정확도"),   Pct(S.AdsAccuracy));
	AddLine(TEXT("  유효 사거리"),   Mtr(S.EffectiveRange));
	AddLine(TEXT("  최대 사거리"),   Mtr(S.MaxRange));
	AddLine(TEXT("  거리 감소율"),   Pct(S.DamageFalloffPercent));

	Head(TEXT("기동"));
	AddLine(TEXT("  이동 속도"),     FString::Printf(TEXT("%.0f"), S.MoveSpeed));
	AddLine(TEXT("  사격 중 이동"),  FString::Printf(TEXT("%.0f"), S.MoveSpeedWhileFiring));
	AddLine(TEXT("  조준 중 이동"),  FString::Printf(TEXT("%.0f"), S.MoveSpeedWhileAiming));
	AddLine(TEXT("  전력 질주"),     FString::Printf(TEXT("%.0f"), S.SprintSpeed));

	if (TB_StatLabels)
	{
		TB_StatLabels->SetText(FText::FromString(Labels));
	}
	if (TB_StatValues)
	{
		TB_StatValues->SetText(FText::FromString(Values));
	}
}
