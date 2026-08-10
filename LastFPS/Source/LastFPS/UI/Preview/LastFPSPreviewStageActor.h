#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "LastFPSPreviewStageActor.generated.h"

class UAnimationAsset;
class UAnimInstance;
class ULastFPSPreviewSlotComponent;
class ULastFPSPreviewViewComponent;
class UMaterialInterface;
class USkeletalMesh;
class USkeletalMeshComponent;

/**
 * 주 대상에 소켓으로 붙는 부속 메시. 무기를 낀 캐릭터처럼 여러 메시가 한 몸으로 보여야 할 때 쓴다.
 *
 * 소켓 부착이라 주 대상이 애니메이션으로 움직이면 부속도 본을 따라간다.
 * 무기는 캐릭터와 스켈레톤이 달라 LeaderPose 로 묶을 수 없고 소켓 부착이 맞다.
 */
USTRUCT(BlueprintType)
struct FLastFPSPreviewAttachment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preview")
	TObjectPtr<USkeletalMesh> Mesh = nullptr;

	/** 주 대상 메시의 소켓 이름. 비어 있으면 메시 루트에 붙는다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preview")
	FName SocketName;

	/** 소켓에 스냅한 뒤 적용할 상대 변환. 무기마다 손 정렬이 달라 보정이 필요하다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preview")
	FTransform RelativeTransform = FTransform::Identity;
};

/**
 * 자리 하나에 올릴 대상. 무기든 무기를 낀 캐릭터든 이 계약 하나로 표현한다.
 *
 * 무대는 무엇이 올라오는지 알지 않는다. 올리는 화면이 무엇을 어떻게 보일지 채우고,
 * 무대는 받은 대로 세우기만 한다. 대상이 늘어나도 무대 코드는 그대로다.
 */
USTRUCT(BlueprintType)
struct FLastFPSPreviewSubject
{
	GENERATED_BODY()

	/**
	 * 세울 액터 클래스. 지정하면 이쪽이 우선하고 Mesh 는 무시된다.
	 *
	 * 모듈러 캐릭터처럼 여러 컴포넌트로 조립된 대상은 구성을 코드로 따라 그리면 계속 어긋난다
	 * (파츠 하나 늘 때마다 이쪽도 고쳐야 한다). 클래스를 그대로 세우면 조립 결과가 공짜로 따라온다.
	 *
	 * 빙의하지 않고 스폰하므로 입력·GAS 초기화는 IsLocallyControlled 가드에 걸려 실행되지 않는다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preview")
	TSubclassOf<AActor> ActorClass = nullptr;

	/** 액터 없이 메시 하나만 세울 때 쓴다. 무기처럼 단순한 대상이면 이걸로 충분하다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preview")
	TObjectPtr<USkeletalMesh> Mesh = nullptr;

	/**
	 * 포즈를 잡을 애님 인스턴스. 게임플레이용 ABP 는 대개 소유자가 Character 이고 이동 컴포넌트가
	 * 있다고 가정하므로, 무대에 그대로 꽂으면 T 포즈로 서거나 매 프레임 경고를 뱉는다.
	 * 프리뷰 전용 ABP 가 필요할 때만 지정한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preview")
	TSubclassOf<UAnimInstance> AnimInstanceClass = nullptr;

	/**
	 * 대기 자세 하나만 필요할 때 쓰는 애니메이션. ABP 를 만들지 않아도 된다.
	 * AnimInstanceClass 가 지정돼 있으면 그쪽이 우선하고, 둘 다 없으면 레퍼런스 포즈로 선다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preview")
	TObjectPtr<UAnimationAsset> IdleAnimation = nullptr;

	/**
	 * 켜면 대상이 지금 켜진 시점 카메라를 바라보도록 자리를 돌린다.
	 *
	 * 무대와 카메라 배치가 바뀔 때마다 정면 각도를 손으로 맞추면 계속 어긋난다.
	 * 어느 쪽에서 보든 정면이 나와야 하므로 기본값은 켜짐이다.
	 * 배치한 각도를 그대로 쓰고 싶으면 끈다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preview")
	bool bFaceActiveView = true;

	/** 대상의 정면을 자리 기준으로 맞춰 준다. 액터를 세울 때는 액터 전체가 돌아간다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preview")
	FRotator MeshRotation = FRotator::ZeroRotator;

	/** 주 대상에 함께 보여야 하는 메시들(장착 무기 등). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preview")
	TArray<FLastFPSPreviewAttachment> Attachments;
};

/** 자리 하나에 무대가 만들어 붙인 실제 컴포넌트들. 무대만 쓰는 내부 상태다. */
USTRUCT()
struct FLastFPSPreviewSlotRuntime
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<ULastFPSPreviewSlotComponent> Slot = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> Mesh = nullptr;

	/** ActorClass 로 세운 대상. 자리가 소유하므로 대상이 바뀌면 여기서 파괴한다. */
	UPROPERTY(Transient)
	TObjectPtr<AActor> SpawnedActor = nullptr;

	/** 시점이 바뀔 때마다 이 자리를 다시 돌려야 하는지. 올려 둔 대상이 정한다. */
	bool bFaceActiveView = true;

	/** 소켓에 매달리는 부속(무기 등). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USkeletalMeshComponent>> Attachments;
};

/**
 * 아웃게임에서 3D 대상을 세워 보여 주는 무대.
 *
 * 무대는 레벨이 소유하고 화면들이 함께 쓴다(ULastFPSPreviewStageSubsystem).
 * 화면을 열 때마다 스폰하면 그 프레임에 스폰·메시 로드 비용을 그대로 내기 때문이다.
 *
 * 자리(Slot)와 시점(View)을 나눠 둔다. 자리마다 대상을 올려 두고 시점만 옮기면,
 * 캐릭터를 치우지 않고도 무기를 볼 수 있고 돌아올 때 다시 조립할 필요가 없다.
 *
 * 표시는 SceneCapture/RenderTarget 대신 시점 카메라로 한다.
 * 화면이 열릴 때 플레이어 뷰타깃을 이 무대로 전환하면 켜져 있는 카메라가 곧 화면이 된다.
 */
UCLASS()
class LASTFPS_API ALastFPSPreviewStageActor : public AActor
{
	GENERATED_BODY()

public:
	ALastFPSPreviewStageActor();

	/**
	 * 지목한 자리에 대상을 올린다. 다른 자리는 건드리지 않는다.
	 * @return 그 태그의 자리를 찾았으면 true.
	 */
	bool SetSubject(const FGameplayTag& SlotTag, const FLastFPSPreviewSubject& InSubject);

	/** 지목한 자리를 비운다. 자리 자체는 남아 다음 대상이 그대로 쓴다. */
	bool ClearSubject(const FGameplayTag& SlotTag);

	/** 지목한 자리의 대상을 좌우로 회전(드래그 조작용). */
	bool AddYaw(const FGameplayTag& SlotTag, float DeltaDegrees);

	/**
	 * 지목한 태그의 카메라로 시점을 옮긴다. 나머지 카메라는 꺼진다.
	 * 플레이어 뷰타깃이 이 무대일 때 활성 카메라가 곧 화면이므로 뷰타깃을 다시 걸 필요는 없다.
	 *
	 * @return 해당 태그의 카메라를 찾아 전환했으면 true.
	 */
	bool SetActiveView(const FGameplayTag& ViewTag);

	/** 지금 켜져 있는 시점의 태그. 없으면 빈 태그. */
	FGameplayTag GetActiveViewTag() const { return ActiveViewTag; }

	/**
	 * 무대를 쓰는 동안만 켠다. 무대는 레벨 수명 동안 살아 있으므로 파괴 대신 이 스위치로 비용을 끊는다.
	 * 꺼진 동안에는 화면 밖 포즈 갱신이 돌지 않는다.
	 */
	void SetStageActive(bool bInActive);

	bool IsStageActive() const { return bStageActive; }

protected:
	virtual void BeginPlay() override;

	/**
	 * 무대에 세운 대상을 레벨 조명에서 떼어낸다.
	 *
	 * 디렉셔널 라이트는 월드 전체에 닿아 무대를 어디에 두든 피할 수 없다. 아웃게임 화면은 노출이
	 * 일정해야 하는데 레벨 시간대나 실내외에 따라 밝기가 흔들린다.
	 * 전용 채널로 옮기고 무대 BP 에 같은 채널의 조명을 두면 무대 밝기를 무대가 단독으로 소유한다.
	 */
	UPROPERTY(EditAnywhere, Category="Preview|Lighting")
	bool bIsolateStageLighting = true;

	/**
	 * 무대 전용 라이팅 채널. 무대 BP 에 두는 조명도 같은 채널로 맞춰야 한다.
	 * 기본은 채널 2 — 레벨 조명이 쓰는 채널 0 과 겹치지 않는다.
	 */
	UPROPERTY(EditAnywhere, Category="Preview|Lighting", meta=(EditCondition="bIsolateStageLighting"))
	FLightingChannels StageLightingChannels;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> RootScene;

	/**
	 * 기본 자리 두 개. 무대 BP 없이도 동작하도록 C++ 에서 만들어 태그까지 달아 둔다.
	 * BP 에서 위치를 옮기거나 자리를 더 붙일 수 있다.
	 */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ULastFPSPreviewSlotComponent> CharacterSlot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ULastFPSPreviewSlotComponent> WeaponSlot;

	/**
	 * 시점이 하나도 배치되지 않은 무대에서도 화면이 검게 나오지 않도록 두는 기본 시점.
	 * 무대 BP 가 ULastFPSPreviewViewComponent 를 붙이면 그쪽이 우선한다.
	 */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ULastFPSPreviewViewComponent> DefaultView;

private:
	/** BP 에 배치된 자리와 시점을 모으고, 자리마다 메시 컴포넌트를 만들어 붙인다. */
	void CollectSlotsAndViews();

	/** 주 대상의 포즈를 결정한다. ABP > 단일 애니메이션 > 레퍼런스 포즈 순으로 고른다. */
	static void ApplyPose(USkeletalMeshComponent& TargetMesh, const FLastFPSPreviewSubject& InSubject);

	/** 부속 메시 컴포넌트를 요청 개수만큼 맞춘다. 남는 것은 지우고 모자라면 만든다. */
	void RebuildAttachments(FLastFPSPreviewSlotRuntime& SlotRuntime, const TArray<FLastFPSPreviewAttachment>& InAttachments);

	/** 컴포넌트 풀을 요청 개수에 맞춘다. 남는 것은 파괴하고 모자라면 만들어 채운다. */
	USkeletalMeshComponent* AcquireMeshComponent(TArray<TObjectPtr<USkeletalMeshComponent>>& Pool, int32 Index);
	static void TrimMeshComponents(TArray<TObjectPtr<USkeletalMeshComponent>>& Pool, int32 KeepCount);

	/** 자리에 액터를 세운다. 이미 같은 클래스가 서 있으면 다시 만들지 않는다. */
	void RebuildSpawnedActor(FLastFPSPreviewSlotRuntime& SlotRuntime, const FLastFPSPreviewSubject& InSubject);

	/**
	 * 부속을 매달 기준 메시. 액터를 세웠으면 그 액터의 주 메시가 기준이다.
	 * 자리의 메시 컴포넌트에 붙이면 소켓이 없어 무기가 발밑에 뜬다.
	 */
	static USkeletalMeshComponent* ResolveAttachParentMesh(const FLastFPSPreviewSlotRuntime& SlotRuntime);

	/**
	 * 액터의 주 스켈레탈 메시. Character 면 GetMesh(), 아니면 LeaderPose 에 묶이지 않은 첫 메시다.
	 * 의상 파츠는 주 메시를 따라가므로 포즈를 직접 물리면 안 된다.
	 */
	static USkeletalMeshComponent* ResolvePrimaryMesh(const AActor& InActor);

	/** 지금 켜져 있는 시점 카메라. 없으면 null. */
	ULastFPSPreviewViewComponent* FindActiveView() const;

	/** 자리에 올라온 모든 프리미티브를 무대 전용 라이팅 채널로 옮긴다. */
	void ApplyLightingChannels(const FLastFPSPreviewSlotRuntime& SlotRuntime) const;

	/** 무대와 자식 액터의 배경·조명을 전용 라이팅 채널로 통일한다. */
	void ApplyStageLightingIsolation();

	/**
	 * 활성 시점의 위치를 각 자리의 애님 인스턴스에 알린다.
	 * 계약을 구현하지 않은 애님 인스턴스는 건너뛴다 — 시선 추적은 선택 기능이다.
	 */
	void PushGazeTarget();

	/** 자리를 돌려 대상이 활성 시점을 바라보게 한다. 요청하지 않은 자리는 건드리지 않는다. */
	void ApplySlotFacing(FLastFPSPreviewSlotRuntime& SlotRuntime);

	/**
	 * 자리 방향과 시점 초점을 정해진 순서로 다시 맞춘다.
	 *
	 * 둘은 서로의 결과를 입력으로 쓴다. 자리를 돌리면 메시 바운드 중심이 움직이고, 시점 초점은 그
	 * 바운드를 겨눈다. 부르는 순서가 경로마다 다르면 같은 대상이 열 때마다 다른 곳에 잡힌다.
	 * 순서를 여기 한 곳에 못박아 어느 경로로 들어와도 같은 결과가 나오게 한다.
	 */
	void RefreshAlignment();

	/** 세운 액터를 전시용으로 길들인다. 충돌·이동·틱을 끄고 대기 자세를 물린다. */
	static void ConfigureSpawnedActorForPreview(AActor& SpawnedActor, const FLastFPSPreviewSubject& InSubject);

	/** 자리 하나의 모든 메시에 틱·포즈 갱신 설정을 적용한다. */
	static void ApplyTickState(const FLastFPSPreviewSlotRuntime& SlotRuntime, bool bInActive);

	/** 태그로 찾은 자리. 없으면 경고를 남기고 null. */
	FLastFPSPreviewSlotRuntime* FindSlot(const FGameplayTag& SlotTag);

	/**
	 * 시점이 지목한 자리를 겨누도록 회전과 초점 거리를 맞춘다.
	 * 대상이 바뀌면 바운드도 바뀌므로 시점 전환과 대상 교체 양쪽에서 다시 부른다.
	 */
	void ApplyViewFocus(ULastFPSPreviewViewComponent& View);

	/** 자리에 올라온 메시의 바운드 중심(월드). 메시가 없으면 자리 원점. */
	FVector GetSlotFocusLocation(const FGameplayTag& SlotTag);

	/** 자리 태그 → 실제 컴포넌트. BeginPlay 에서 한 번 구성한다. */
	UPROPERTY(Transient)
	TMap<FGameplayTag, FLastFPSPreviewSlotRuntime> Slots;

	/** 배치된 시점 카메라들. 태그로 골라 하나만 켠다. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULastFPSPreviewViewComponent>> Views;

	FGameplayTag ActiveViewTag;

	/** 초기 상태는 꺼짐. 스폰만 해 두고 실제 비용은 무대를 쓸 때 낸다. */
	bool bStageActive = false;
};
