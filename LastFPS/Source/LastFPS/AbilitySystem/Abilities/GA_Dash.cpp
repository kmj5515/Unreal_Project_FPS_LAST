#include "AbilitySystem/Abilities/GA_Dash.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/LastFPSHero.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Utility/LastFPSTags.h"

UGA_Dash::UGA_Dash()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    FGameplayTagContainer Tags;
    Tags.AddTag(LastFPSGameplayTags::Ability_Dash);
    Tags.AddTag(LastFPSGameplayTags::Input_Dash);
    SetAssetTags(Tags);
}

void UGA_Dash::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
    if (!Hero || !Hero->IsAlive() || Hero->GetCombatState() != EMMCombatState::Idle)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ELastFPSDashDirection DashDirection = ELastFPSDashDirection::Forward;
    const FVector DashVector = GetCardinalDashDirection(Hero, DashDirection);
    const FDashMontageInfo& DashMontageInfo = GetDashMontageInfoForDirection(DashDirection);

    Hero->SetCombatState(EMMCombatState::Dashing);
    if (!DashMontageInfo.bUseMontageRootMotion)
    {
        Hero->LaunchCharacter(DashVector * DashStrength, true, false);
    }

    bool bWaitingForMontageEnd = false;
    UAnimMontage* SelectedDashMontage = DashMontageInfo.Montage.Get();
    if (SelectedDashMontage && Hero->GetMesh())
    {
        if (UAnimInstance* AnimInstance = Hero->GetMesh()->GetAnimInstance())
        {
            const float PlayedDuration = AnimInstance->Montage_Play(SelectedDashMontage, MontagePlayRate);
            if (PlayedDuration > 0.f)
            {
                FOnMontageEnded EndDelegate;
                EndDelegate.BindUObject(this, &UGA_Dash::OnDashMontageEnded);
                AnimInstance->Montage_SetEndDelegate(EndDelegate, SelectedDashMontage);
                bWaitingForMontageEnd = true;
            }
        }
    }

    if (!bWaitingForMontageEnd)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                DashTimerHandle,
                this,
                &UGA_Dash::FinishDash,
                DefaultDashDuration,
                false);
        }
        else
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        }
    }
}

void UGA_Dash::FinishDash()
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Dash::OnDashMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (!Montage)
    {
        return;
    }

    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, bInterrupted);
}

FVector UGA_Dash::GetCardinalDashDirection(const ALastFPSHero* Hero, ELastFPSDashDirection& OutDashDirection) const
{
    OutDashDirection = ELastFPSDashDirection::Forward;

    if (!Hero)
    {
        return FVector::ForwardVector;
    }

    const FRotator ControlRotation = Hero->GetController()
        ? Hero->GetController()->GetControlRotation()
        : Hero->GetActorRotation();
    const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);
    const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    const FVector2D MoveInput = Hero->GetCachedMoveInput();
    if (MoveInput.IsNearlyZero())
    {
        return Forward;
    }

    if (FMath::Abs(MoveInput.X) > FMath::Abs(MoveInput.Y))
    {
        OutDashDirection = MoveInput.X >= 0.f
            ? ELastFPSDashDirection::Right
            : ELastFPSDashDirection::Left;
        return MoveInput.X >= 0.f ? Right : -Right;
    }

    OutDashDirection = MoveInput.Y >= 0.f
        ? ELastFPSDashDirection::Forward
        : ELastFPSDashDirection::Backward;
    return MoveInput.Y >= 0.f ? Forward : -Forward;
}

const FDashMontageInfo& UGA_Dash::GetDashMontageInfoForDirection(ELastFPSDashDirection DashDirection) const
{
    const FDashMontageInfo* DirectionalMontageInfo = nullptr;
    switch (DashDirection)
    {
    case ELastFPSDashDirection::Forward:
        DirectionalMontageInfo = &DashForwardMontage;
        break;
    case ELastFPSDashDirection::Backward:
        DirectionalMontageInfo = &DashBackwardMontage;
        break;
    case ELastFPSDashDirection::Left:
        DirectionalMontageInfo = &DashLeftMontage;
        break;
    case ELastFPSDashDirection::Right:
        DirectionalMontageInfo = &DashRightMontage;
        break;
    default:
        break;
    }
    
    return DirectionalMontageInfo && DirectionalMontageInfo->Montage.Get()
        ? *DirectionalMontageInfo
        : DefaultDashMontage;
}

void UGA_Dash::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DashTimerHandle);
    }

    if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()))
    {
        if (Hero->GetCombatState() == EMMCombatState::Dashing)
        {
            Hero->SetCombatState(EMMCombatState::Idle);
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
