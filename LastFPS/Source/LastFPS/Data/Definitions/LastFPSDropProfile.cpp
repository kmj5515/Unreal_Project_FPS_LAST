#include "Data/Definitions/LastFPSDropProfile.h"

// TSubclassOf 의 유효성 검사가 T::StaticClass() 를 호출하므로 완전한 타입이 필요하다.
#include "Economy/LastFPSItemPickupActor.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

void ULastFPSDropProfile::RollDrops(const bool bIsBoss, TArray<FLastFPSDropEntry>& OutEntries) const
{
    OutEntries.Reset();

    for (const FLastFPSDropEntry& Entry : bIsBoss ? BossDrops : CommonDrops)
    {
        if (!Entry.PickupClass || Entry.Count <= 0 || FMath::FRand() >= Entry.Chance)
        {
            continue;
        }

        OutEntries.Add(Entry);
    }
}

#if WITH_EDITOR
EDataValidationResult ULastFPSDropProfile::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    auto ValidateEntries = [&](const TArray<FLastFPSDropEntry>& Entries, const TCHAR* ListName)
    {
        for (int32 Index = 0; Index < Entries.Num(); ++Index)
        {
            if (!Entries[Index].PickupClass)
            {
                Context.AddError(FText::FromString(FString::Printf(
                    TEXT("%s[%d] 에 PickupClass 가 없어 이 항목은 드랍되지 않는다."), ListName, Index)));
                Result = EDataValidationResult::Invalid;
            }
        }
    };

    ValidateEntries(CommonDrops, TEXT("CommonDrops"));
    ValidateEntries(BossDrops, TEXT("BossDrops"));

    if (CommonDrops.IsEmpty() && BossDrops.IsEmpty())
    {
        Context.AddWarning(FText::FromString(
            TEXT("CommonDrops 와 BossDrops 가 모두 비어 있어 처치 드랍이 없다.")));
    }

    return Result;
}
#endif
