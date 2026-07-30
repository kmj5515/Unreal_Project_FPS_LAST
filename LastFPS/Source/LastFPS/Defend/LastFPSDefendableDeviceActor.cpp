#include "Defend/LastFPSDefendableDeviceActor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Components/StaticMeshComponent.h"

ALastFPSDefendableDeviceActor::ALastFPSDefendableDeviceActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// 장치는 예측 입력이 없으므로 최소 복제 모드로 대역폭을 아낀다.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<ULastFPSAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* ALastFPSDefendableDeviceActor::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ALastFPSDefendableDeviceActor::BeginPlay()
{
	Super::BeginPlay();

	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// 체력 변경을 HUD 델리게이트로 옮긴다. 실패 판정은 목표 컴포넌트가 따로 구독한다.
	HealthChangedHandle = AbilitySystemComponent
		->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetHealthAttribute())
		.AddUObject(this, &ALastFPSDefendableDeviceActor::HandleHealthChanged);
	MaxHealthChangedHandle = AbilitySystemComponent
		->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &ALastFPSDefendableDeviceActor::HandleMaxHealthChanged);

	BroadcastHealth();
}

void ALastFPSDefendableDeviceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AbilitySystemComponent)
	{
		if (HealthChangedHandle.IsValid())
		{
			AbilitySystemComponent
				->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetHealthAttribute())
				.Remove(HealthChangedHandle);
			HealthChangedHandle.Reset();
		}

		if (MaxHealthChangedHandle.IsValid())
		{
			AbilitySystemComponent
				->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetMaxHealthAttribute())
				.Remove(MaxHealthChangedHandle);
			MaxHealthChangedHandle.Reset();
		}
	}

	Super::EndPlay(EndPlayReason);
}

float ALastFPSDefendableDeviceActor::GetHealth01() const
{
	if (!AbilitySystemComponent)
	{
		return 0.f;
	}

	const float MaxHealth =
		AbilitySystemComponent->GetNumericAttribute(ULastFPSAttributeSet::GetMaxHealthAttribute());
	if (MaxHealth <= 0.f)
	{
		return 0.f;
	}

	const float Health =
		AbilitySystemComponent->GetNumericAttribute(ULastFPSAttributeSet::GetHealthAttribute());
	return FMath::Clamp(Health / MaxHealth, 0.f, 1.f);
}

bool ALastFPSDefendableDeviceActor::IsDestroyed() const
{
	return AbilitySystemComponent
		&& AbilitySystemComponent->GetNumericAttribute(ULastFPSAttributeSet::GetHealthAttribute()) <= 0.f;
}

void ALastFPSDefendableDeviceActor::HandleHealthChanged(const FOnAttributeChangeData& /*Data*/)
{
	BroadcastHealth();
}

void ALastFPSDefendableDeviceActor::HandleMaxHealthChanged(const FOnAttributeChangeData& /*Data*/)
{
	BroadcastHealth();
}

void ALastFPSDefendableDeviceActor::BroadcastHealth() const
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	OnHealthChanged.Broadcast(
		AbilitySystemComponent->GetNumericAttribute(ULastFPSAttributeSet::GetHealthAttribute()),
		AbilitySystemComponent->GetNumericAttribute(ULastFPSAttributeSet::GetMaxHealthAttribute()));
}
