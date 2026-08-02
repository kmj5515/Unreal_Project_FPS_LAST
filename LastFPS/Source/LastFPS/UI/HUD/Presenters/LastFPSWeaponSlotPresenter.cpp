#include "UI/HUD/Presenters/LastFPSWeaponSlotPresenter.h"

#include "Character/Components/WeaponComponent.h"
#include "Components/PanelWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Inventory/LastFPSEquipmentSubsystem.h"
#include "UI/HUD/LastFPSWeaponSlotWidget.h"

void ULastFPSWeaponSlotPresenter::Initialize(
	UPanelWidget* InSlotContainer, TSubclassOf<ULastFPSWeaponSlotWidget> InSlotWidgetClass)
{
	SlotContainer   = InSlotContainer;
	SlotWidgetClass = InSlotWidgetClass;
	RebuildSlots();
}

void ULastFPSWeaponSlotPresenter::BindToWeaponComponent(UWeaponComponent* InWeaponComponent)
{
	if (WeaponComponent.Get() == InWeaponComponent)
	{
		return;
	}

	Reset();

	if (!InWeaponComponent)
	{
		RebuildSlots();
		return;
	}

	WeaponComponent = InWeaponComponent;
	InWeaponComponent->OnWeaponLoadoutChanged.AddUniqueDynamic(
		this, &ULastFPSWeaponSlotPresenter::HandleLoadoutChanged);
	InWeaponComponent->OnWeaponSlotChanged.AddUniqueDynamic(
		this, &ULastFPSWeaponSlotPresenter::HandleActiveSlotChanged);

	RebuildSlots();
}

void ULastFPSWeaponSlotPresenter::Reset()
{
	if (UWeaponComponent* WeaponComp = WeaponComponent.Get())
	{
		WeaponComp->OnWeaponLoadoutChanged.RemoveDynamic(
			this, &ULastFPSWeaponSlotPresenter::HandleLoadoutChanged);
		WeaponComp->OnWeaponSlotChanged.RemoveDynamic(
			this, &ULastFPSWeaponSlotPresenter::HandleActiveSlotChanged);
	}

	WeaponComponent = nullptr;

	if (UPanelWidget* Container = SlotContainer.Get())
	{
		Container->ClearChildren();
	}
	SlotWidgets.Reset();
}

void ULastFPSWeaponSlotPresenter::HandleLoadoutChanged()
{
	RebuildSlots();
}

void ULastFPSWeaponSlotPresenter::HandleActiveSlotChanged(int32 ActiveSlotIndex)
{
	RefreshActiveHighlight();
}

void ULastFPSWeaponSlotPresenter::RebuildSlots()
{
	UPanelWidget* Container = SlotContainer.Get();
	if (!Container)
	{
		return;
	}

	UWeaponComponent* WeaponComp = WeaponComponent.Get();
	int32 SlotCount = WeaponComp ? WeaponComp->GetWeaponSlotCount() : 0;
	if (const UWorld* World = Container->GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (const ULastFPSEquipmentSubsystem* Equipment =
				GameInstance->GetSubsystem<ULastFPSEquipmentSubsystem>())
			{
				// 장비 시스템이 슬롯 개수의 단일 출처이며, 비어 있어도 HUD는 그 개수를 유지한다.
				SlotCount = FMath::Max(
					SlotCount,
					Equipment->GetSlotCount(ELastFPSEquipmentSlotType::Weapon));
			}
		}
	}

	Container->SetVisibility(SlotCount > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (SlotCount != SlotWidgets.Num())
	{
		Container->ClearChildren();
		SlotWidgets.Reset();

		if (!SlotWidgetClass)
		{
			// 클래스 미지정은 설정 실수다. 슬롯 개수가 바뀔 때만 알려 매 프레임 로그가 쌓이지 않게 한다.
			if (SlotCount > 0)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("HUD 무기 슬롯 위젯 클래스가 지정되지 않아 슬롯 %d칸을 표시하지 못합니다."),
					SlotCount);
			}
			return;
		}

		for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
		{
			ULastFPSWeaponSlotWidget* SlotWidget =
				CreateWidget<ULastFPSWeaponSlotWidget>(Container->GetWorld(), SlotWidgetClass);
			if (!SlotWidget)
			{
				continue;
			}

			Container->AddChild(SlotWidget);
			SlotWidgets.Add(SlotWidget);
		}
	}

	const int32 ActiveSlot = WeaponComp ? WeaponComp->GetActiveWeaponSlot() : INDEX_NONE;
	for (int32 SlotIndex = 0; SlotIndex < SlotWidgets.Num(); ++SlotIndex)
	{
		if (ULastFPSWeaponSlotWidget* SlotWidget = SlotWidgets[SlotIndex])
		{
			const ULastFPSWeaponDefinition* Definition =
				WeaponComp ? WeaponComp->GetWeaponDefinitionForSlot(SlotIndex) : nullptr;
			SlotWidget->SetupSlot(
				SlotIndex,
				Definition,
				Definition && SlotIndex == ActiveSlot);
		}
	}
}

void ULastFPSWeaponSlotPresenter::RefreshActiveHighlight()
{
	UWeaponComponent* WeaponComp = WeaponComponent.Get();
	if (!WeaponComp)
	{
		return;
	}

	const int32 ActiveSlot = WeaponComp->GetActiveWeaponSlot();
	for (int32 SlotIndex = 0; SlotIndex < SlotWidgets.Num(); ++SlotIndex)
	{
		if (ULastFPSWeaponSlotWidget* SlotWidget = SlotWidgets[SlotIndex])
		{
			SlotWidget->SetActive(
				WeaponComp->GetWeaponDefinitionForSlot(SlotIndex) && SlotIndex == ActiveSlot);
		}
	}
}
