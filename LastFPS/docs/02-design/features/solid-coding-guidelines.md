# SOLID Coding Guidelines

## Purpose

All code proposals and implementation work in this project should use SOLID principles as the default design lens.

The goal is not to over-engineer every small feature. The goal is to keep gameplay systems easy to extend when abilities, projectiles, status effects, UI, weapons, enemies, and character logic grow.

## Default Rule

Before adding code, ask:

```text
What is the single responsibility of this class?
What should be data, not code?
What should be reusable across future abilities or systems?
What dependency can be inverted behind an interface, component, tag, or data asset?
```

## S: Single Responsibility

Each class should have one clear reason to change.

Good separation:

```text
GameplayAbility
- activation rules
- montage/event orchestration
- spawning projectiles or applying high-level effects

Projectile
- movement
- collision
- hit reporting

GameplayEffect
- attribute changes
- duration
- stacking
- granted/blocking tags

GameplayCue
- VFX
- SFX
- camera feedback
- UI feedback

AnimNotify
- send a timing event
```

Avoid placing animation timing, projectile collision, damage calculation, status logic, and visual effects all inside one ability class.

## O: Open-Closed

Systems should be open for extension and closed for repeated modification.

Prefer this:

```cpp
UPROPERTY(EditDefaultsOnly)
TArray<TSubclassOf<UGameplayEffect>> EffectsOnHit;
```

Over this:

```cpp
if (bIsIce)
{
    ApplyIceSlow();
}
else if (bIsPoison)
{
    ApplyPoisonSlow();
}
```

When a new ability is added, prefer configuring a new data asset, tag, GameplayEffect, or Blueprint subclass instead of editing a large switch statement.

## L: Liskov Substitution

Base classes should be usable by derived classes without surprising behavior.

Example:

```text
AAbilityProjectile
- can be used by ice, fire, poison, or electric projectiles
- does not assume one element type
- applies configured effects on hit
```

Avoid making a base class secretly depend on ice-only, rifle-only, player-only, or boss-only behavior.

## I: Interface Segregation

Use small interfaces or narrow dependencies.

Good examples:

```text
IAbilitySystemInterface
UAbilitySystemComponent
GameplayTag
DataAsset
ActorComponent
```

Avoid making systems depend on a concrete class when they only need one capability.

For example, a projectile should usually ask:

```text
Does this actor have an AbilitySystemComponent?
```

Not:

```text
Is this exactly ALastFPSHero?
```

## D: Dependency Inversion

High-level gameplay logic should not depend on many low-level concrete classes.

Prefer dependency injection through properties:

```cpp
UPROPERTY(EditDefaultsOnly)
TSubclassOf<AActor> ProjectileClass;

UPROPERTY(EditDefaultsOnly)
TArray<TSubclassOf<UGameplayEffect>> EffectsOnHit;

UPROPERTY(EditDefaultsOnly)
FGameplayTag MontageSpawnEventTag;
```

Avoid hard-coding concrete classes unless they are true defaults.

```cpp
ProjectileClass = ALastFPSProjectile::StaticClass(); // acceptable default
EffectsOnHit.Add(ULastFPSGE_DamageInstant::StaticClass()); // acceptable default
```

Blueprints or data assets should be able to override defaults for specific characters, weapons, enemies, or abilities.

## Gameplay Ability Pattern

Abilities should coordinate, not own every detail.

Recommended flow:

```text
1. Ability validates activation.
2. Ability commits cost/cooldown at the intended timing.
3. Ability plays montage.
4. Montage sends gameplay events through AnimNotify.
5. Ability responds to gameplay events.
6. Ability spawns projectile or applies configured effects.
7. Projectile reports hits.
8. GameplayEffects and GameplayCues handle result and presentation.
```

Use common montage event tags:

```text
Event.Montage.Projectile.Spawn
Event.Montage.Ability.Commit
Event.Montage.Ability.End
```

## Status Effect Pattern

Do not create one class for every element/status combination.

Avoid:

```text
GE_IceSlow
GE_PoisonSlow
GE_ElectricSlow
```

Prefer:

```text
GE_StatusSlow
GE_StatusFreeze
GE_Damage_Instant
```

Use tags and data to express identity:

```text
Status.Slow
Status.Freeze
Element.Ice
Element.Poison
GameplayCue.Element.Ice.Impact
```

Use element-specific classes only when the behavior is truly unique:

```text
GE_Ice_ChillStack
GE_Fire_BurnSpread
GE_Electric_ChainShock
```

## Naming

Name classes by behavior, not by every configured combination.

Prefer:

```text
GA_ProjectileAbilityBase
AAbilityProjectile
GE_StatusSlow
GE_StatusFreeze
AN_SendGameplayEvent
```

Avoid:

```text
GA_IceProjectileWithSlowAndDamage
GE_IceSlow
GE_FireSlow
AN_IceProjectileSpawnOnly
```

Specific Blueprint assets can still have themed names:

```text
BP_GA_Viola_IceProjectile
BP_Projectile_IceShard
BP_GE_Ice_ChillStack
```

## Practical Review Checklist

Use this checklist before adding or modifying gameplay code:

```text
Does this class have one clear responsibility?
Can this be configured with a property, tag, GameplayEffect, or data asset?
Will adding a similar ability require editing this same code again?
Is this depending on a concrete class when an interface/component/tag would be enough?
Are VFX/SFX separated into GameplayCue or presentation code?
Are animation timings sent through montage events instead of hard-coded delays?
```

## Project Preference

For this project, prefer:

```text
Composition over inheritance
GameplayTags over enums when content needs to expand
Data assets and Blueprint defaults over hard-coded combinations
GameplayEffects for attribute/status changes
GameplayCues for presentation
AnimNotifies only for timing signals
```

Do not force a large abstraction for a tiny one-off feature. Start simple, but place the first abstraction where a second similar feature is already expected.
