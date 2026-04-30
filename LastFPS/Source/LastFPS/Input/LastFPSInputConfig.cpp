#include "Input/LastFPSInputConfig.h"

const UInputAction* ULastFPSInputConfig::FindNativeInputActionByTag(const FGameplayTag& Tag) const
{
    for (const FLastFPSInputAction& Entry : NativeInputActions)
    {
        if (Entry.InputAction && Entry.InputTag == Tag)
            return Entry.InputAction;
    }
    return nullptr;
}

const UInputAction* ULastFPSInputConfig::FindAbilityInputActionByTag(const FGameplayTag& Tag) const
{
    for (const FLastFPSInputAction& Entry : AbilityInputActions)
    {
        if (Entry.InputAction && Entry.InputTag == Tag)
            return Entry.InputAction;
    }
    return nullptr;
}
