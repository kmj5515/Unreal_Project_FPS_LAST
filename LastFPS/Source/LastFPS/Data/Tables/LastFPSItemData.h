#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LastFPSItemData.generated.h"

class UTexture2D;
class ULastFPSWeaponDefinition;

UENUM(BlueprintType)
enum class ELastFPSItemType : uint8
{
	Weapon		UMETA(DisplayName="무기"),
	Module		UMETA(DisplayName="모듈"),
	Consumable	UMETA(DisplayName="소모품"),
	Material	UMETA(DisplayName="재료"),
};

UENUM(BlueprintType)
enum class ELastFPSItemRarity : uint8
{
	Common		UMETA(DisplayName="일반"),
	Rare		UMETA(DisplayName="희귀"),
	Epic		UMETA(DisplayName="영웅"),
	Legendary	UMETA(DisplayName="전설"),
};

/** 등급별 표시 색 — 인벤토리/상점 UI와 드랍 픽업 발광색이 공유하는 단일 출처. */
inline FLinearColor LastFPSGetRarityColor(ELastFPSItemRarity Rarity)
{
	switch (Rarity)
	{
	case ELastFPSItemRarity::Rare:      return FLinearColor(0.1f, 0.4f, 1.f,   1.f); // 파랑
	case ELastFPSItemRarity::Epic:      return FLinearColor(0.6f, 0.1f, 1.f,   1.f); // 보라
	case ELastFPSItemRarity::Legendary: return FLinearColor(1.f,  0.5f, 0.05f, 1.f); // 주황
	case ELastFPSItemRarity::Common:
	default:                            return FLinearColor(0.5f, 0.5f, 0.5f,  1.f); // 회색
	}
}

/**
 * 무기 아이템의 상세 프리뷰(F1) 스펙 시트 값. **표시 전용**이며 현재 전투 로직에는 연동되지 않는다
 * (관통/사거리감소/정확도 등 전투 시스템 미구현). DT_ItemData 의 무기 행에서 저작하고 프리뷰 패널이 그룹별로 읽는다.
 * (게임플레이 실사용 데미지/연사는 ULastFPSWeaponDefinition 이 별도 보유 — 이건 표시용 사본이다.)
 */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSWeaponDisplayStats
{
	GENERATED_BODY()

	// ── 공격 ──
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") float MinDamage = 12.f;       // 공격력(하한)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") float MaxDamage = 18.f;       // 공격력(상한)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") float FireRate = 0.10f;       // 발사 속도(연사 간격, 초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") int32 MagazineSize = 30;      // 장탄량
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") float ReloadTime = 2.0f;      // 재장전 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") float Penetration = 1.0f;     // 관통력

	// ── 치명타·속성 ──
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crit", meta=(ClampMin=0, ClampMax=1)) float CriticalChance = 0.15f;     // 치명타 확률 0~1
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crit") float CriticalMultiplier = 2.0f;   // 치명타 배율(x)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crit") float WeakpointMultiplier = 1.5f;  // 약점 배율(x)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crit") float MultiHitMultiplier = 1.0f;   // 다중 타격 배율(x)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crit", meta=(ClampMin=0, ClampMax=1)) float StatusEffectChance = 0.10f; // 속성 상태효과 부여확률 0~1
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crit") float BreakPower = 10.0f;          // 격파

	// ── 명중·사거리 ──
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Range", meta=(ClampMin=0, ClampMax=1)) float HipFireAccuracy = 0.70f;   // 지향사격 정확도 0~1
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Range", meta=(ClampMin=0, ClampMax=1)) float AdsAccuracy = 0.90f;       // 조준사격 정확도 0~1
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Range") float EffectiveRange = 3000.f;    // 유효 사거리(cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Range") float MaxRange = 8000.f;          // 최대 사거리(cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Range", meta=(ClampMin=0, ClampMax=1)) float DamageFalloffPercent = 0.50f; // 거리별 감소비율 0~1

	// ── 기동 ──
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mobility") float MoveSpeed = 600.f;             // 이동속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mobility") float MoveSpeedWhileFiring = 400.f;  // 사격 중 이동속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mobility") float MoveSpeedWhileAiming = 350.f;  // 조준 중 이동속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mobility") float SprintSpeed = 900.f;           // 전력 질주 속도
};

/**
 * 아이템 1종의 정의 — DataTable 행.
 * 인벤토리 UI가 읽는 정의(메타데이터)이며, 런타임 보유량/장착 상태는 별도 서브시스템에서 관리(추후).
 */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta=(MultiLine=true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	ELastFPSItemType ItemType = ELastFPSItemType::Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	ELastFPSItemRarity Rarity = ELastFPSItemRarity::Common;

	/** 슬롯 1칸에 최대 겹치는 수량 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	int32 MaxStackSize = 1;

	/**
	 * 무기 아이템의 3D 프리뷰용 무기 정의 (ItemType==Weapon 일 때만 채운다).
	 * F1 프리뷰가 이 정의의 SkeletalMesh 를 배치한다(P3). 무기가 아니면 비워 둠.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	TSoftObjectPtr<ULastFPSWeaponDefinition> WeaponDefinition;

	/** 무기 상세 프리뷰(F1) 스펙 시트 — 무기 행에서만 저작(표시 전용). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	FLastFPSWeaponDisplayStats WeaponStats;
};
