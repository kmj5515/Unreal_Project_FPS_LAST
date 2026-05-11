#include "UI/LastFPSScoreRowWidget.h"

namespace
{
void SafeSetText(UTextBlock* Block, const FString& Value)
{
    if (Block)
    {
        Block->SetText(FText::FromString(Value));
    }
}
}

void ULastFPSScoreRowWidget::SetRow(const FString& PlayerName, int32 Kills, int32 Deaths, int32 Assists, int32 Damage, int32 DamageTaken, int32 HealingReceived, bool bIsLocalPlayer)
{
    SafeSetText(PlayerNameText,     PlayerName);
    SafeSetText(KillsText,          FString::FromInt(Kills));
    SafeSetText(DeathsText,         FString::FromInt(Deaths));
    SafeSetText(AssistsText,        FString::FromInt(Assists));
    SafeSetText(DamageText,         FString::FromInt(Damage));
    SafeSetText(DamageTakenText,    FString::FromInt(DamageTaken));
    SafeSetText(HealingReceivedText, FString::FromInt(HealingReceived));

    OnRowStyleChanged(/*bIsHeader=*/false, bIsLocalPlayer);
}

void ULastFPSScoreRowWidget::SetHeader()
{
    SafeSetText(PlayerNameText,      TEXT("Player"));
    SafeSetText(KillsText,           TEXT("K"));
    SafeSetText(DeathsText,          TEXT("D"));
    SafeSetText(AssistsText,         TEXT("A"));
    SafeSetText(DamageText,          TEXT("DMG"));
    SafeSetText(DamageTakenText,     TEXT("TAKEN"));
    SafeSetText(HealingReceivedText, TEXT("HEAL"));

    OnRowStyleChanged(/*bIsHeader=*/true, /*bIsLocalPlayer=*/false);
}
