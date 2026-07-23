#include "UI/HUD/Presenters/LastFPSSkillCooldownPresenter.h"

#include "Character/LastFPSCharacterBase.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Data/Tables/LastFPSCharacterSkillData.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Skills/LastFPSSkillDataSubsystem.h"
#include "UI/HUD/LastFPSSkillCooldownSlotWidget.h"
#include "Utility/LastFPSTags.h"

void ULastFPSSkillCooldownPresenter::Initialize(
    ULastFPSSkillCooldownSlotWidget* InSlotQ,
    ULastFPSSkillCooldownSlotWidget* InSlotE,
    ULastFPSSkillCooldownSlotWidget* InSlotZ,
    ULastFPSSkillCooldownSlotWidget* InSlotF)
{
    SlotQ = InSlotQ;
    SlotE = InSlotE;
    SlotZ = InSlotZ;
    SlotF = InSlotF;
}

bool ULastFPSSkillCooldownPresenter::TryInitialize(const APlayerController* OwningPlayer, UAbilitySystemComponent* ASC)
{
    if (bInitialized)
    {
        return true;
    }

    if (!SlotQ || !SlotE || !SlotZ || !SlotF || !ASC)
    {
        return false;
    }

    const ALastFPSCharacterBase* Character = OwningPlayer ? Cast<ALastFPSCharacterBase>(OwningPlayer->GetPawn()) : nullptr;
    const ULastFPSCharacterDefinition* Definition = Character ? Character->GetCharacterDefinition() : nullptr;
    const UGameInstance* GameInstance = Character ? Character->GetGameInstance() : nullptr;
    const ULastFPSSkillDataSubsystem* SkillDataSubsystem =
        GameInstance ? GameInstance->GetSubsystem<ULastFPSSkillDataSubsystem>() : nullptr;
    if (!Definition || !SkillDataSubsystem)
    {
        return false;
    }

    const FLastFPSCharacterSkillData* Skill1 = SkillDataSubsystem->FindSkill(
        Definition->CharacterId, ELastFPSCharacterSkillSlot::Skill1);
    const FLastFPSCharacterSkillData* Skill2 = SkillDataSubsystem->FindSkill(
        Definition->CharacterId, ELastFPSCharacterSkillSlot::Skill2);
    const FLastFPSCharacterSkillData* Skill3 = SkillDataSubsystem->FindSkill(
        Definition->CharacterId, ELastFPSCharacterSkillSlot::Skill3);
    const FLastFPSCharacterSkillData* Ultimate = SkillDataSubsystem->FindSkill(
        Definition->CharacterId, ELastFPSCharacterSkillSlot::Ultimate);
    const bool bHasDefinitionLoadout = Skill1 && Skill2 && Skill3 && Ultimate;
    if (!bHasDefinitionLoadout)
    {
        SlotQ->ConfigureCooldownSlot(LastFPSGameplayTags::Cooldown_Skill1, nullptr);
        SlotQ->SetKeyLabel(FText::FromString(TEXT("Q")));
        SlotE->ConfigureCooldownSlot(LastFPSGameplayTags::Cooldown_Skill2, nullptr);
        SlotE->SetKeyLabel(FText::FromString(TEXT("E")));
        SlotZ->ConfigureCooldownSlot(LastFPSGameplayTags::Cooldown_Skill3, nullptr);
        SlotZ->SetKeyLabel(FText::FromString(TEXT("Z")));
        SlotF->ConfigureCooldownSlot(LastFPSGameplayTags::Cooldown_Ultimate, nullptr);
        SlotF->SetKeyLabel(FText::FromString(TEXT("F")));
        bInitialized = true;
        return true;
    }

    const auto ConfigureSlot = [](
        ULastFPSSkillCooldownSlotWidget* Widget,
        const FLastFPSCharacterSkillData* SkillData,
        const ELastFPSCharacterSkillSlot ExpectedSlot)
    {
        if (!Widget || !SkillData || SkillData->Slot != ExpectedSlot
            || !SkillData->CooldownTag.IsValid())
        {
            return false;
        }

        Widget->ConfigureCooldownSlot(SkillData->CooldownTag, nullptr);
        Widget->SetConfiguredKeyLabel(SkillData->KeyLabel);
        if (UTexture2D* Icon = SkillData->Icon.LoadSynchronous())
        {
            Widget->SetSkillIconTexture(Icon);
        }
        return true;
    };

    if (!ConfigureSlot(SlotQ, Skill1, ELastFPSCharacterSkillSlot::Skill1)
        || !ConfigureSlot(SlotE, Skill2, ELastFPSCharacterSkillSlot::Skill2)
        || !ConfigureSlot(SlotZ, Skill3, ELastFPSCharacterSkillSlot::Skill3)
        || !ConfigureSlot(SlotF, Ultimate, ELastFPSCharacterSkillSlot::Ultimate))
    {
        return false;
    }

    bInitialized = true;
    return true;
}

void ULastFPSSkillCooldownPresenter::Tick(UAbilitySystemComponent* ASC)
{
    if (!bInitialized || !ASC)
    {
        return;
    }

    if (SlotQ) { SlotQ->UpdateFromASC(ASC); }
    if (SlotE) { SlotE->UpdateFromASC(ASC); }
    if (SlotZ) { SlotZ->UpdateFromASC(ASC); }
    if (SlotF) { SlotF->UpdateFromASC(ASC); }
}
