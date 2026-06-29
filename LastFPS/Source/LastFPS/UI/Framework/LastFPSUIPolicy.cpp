#include "UI/Framework/LastFPSUIPolicy.h"

#include "GameUIPolicy.h"
#include "UI/Framework/LastFPSPrimaryGameLayout.h"
#include "UObject/UnrealType.h"

ULastFPSUIPolicy::ULastFPSUIPolicy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ULastFPSUIPolicy::PostInitProperties()
{
	Super::PostInitProperties();

	// CDO is irrelevant — UGameUIManagerSubsystem::Initialize spawns a fresh instance.
	// We need to set LayoutClass on every instance, not just the CDO.
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	// LayoutClass is private on UGameUIPolicy — write directly via reflected memory so no BP policy is required.
	static const FName LayoutClassName(TEXT("LayoutClass"));
	if (FProperty* LayoutClassProp = FindFProperty<FProperty>(UGameUIPolicy::StaticClass(), LayoutClassName))
	{
		if (void* ValuePtr = LayoutClassProp->ContainerPtrToValuePtr<void>(this))
		{
			TSoftClassPtr<UPrimaryGameLayout>* TypedPtr = static_cast<TSoftClassPtr<UPrimaryGameLayout>*>(ValuePtr);
			*TypedPtr = TSoftClassPtr<UPrimaryGameLayout>(ULastFPSPrimaryGameLayout::StaticClass());
		}
	}
}
