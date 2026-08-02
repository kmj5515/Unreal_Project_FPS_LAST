#include "UI/Preview/LastFPSPreviewStageActor.h"

#include "Animation/AnimationAsset.h"
#include "Animation/AnimInstance.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UI/Framework/LastFPSUITags.h"
#include "UI/Preview/LastFPSPreviewGazeTarget.h"
#include "UI/Preview/LastFPSPreviewSlotComponent.h"
#include "UI/Preview/LastFPSPreviewViewComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSPreviewStage, Log, All);

ALastFPSPreviewStageActor::ALastFPSPreviewStageActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	// 기본 자리와 시점을 C++ 에서 만들어 둔다. BP 를 만들지 않아도 최소 동작은 하고,
	// BP 에서는 위치만 옮기거나 자리를 더 붙이면 된다.
	CharacterSlot = CreateDefaultSubobject<ULastFPSPreviewSlotComponent>(TEXT("CharacterSlot"));
	CharacterSlot->SetupAttachment(RootScene);

	WeaponSlot = CreateDefaultSubobject<ULastFPSPreviewSlotComponent>(TEXT("WeaponSlot"));
	WeaponSlot->SetupAttachment(RootScene);

	// 시점 카메라는 자리가 아니라 루트에 단다. 자리에 달면 드래그로 대상을 돌릴 때 카메라가 함께 돌아
	// 대상이 제자리에서 도는 것으로 보이지 않는다.
	DefaultView = CreateDefaultSubobject<ULastFPSPreviewViewComponent>(TEXT("DefaultView"));
	DefaultView->SetupAttachment(RootScene);

	// 레벨 조명이 쓰는 채널 0 에서 빠져나온다. 무대 BP 의 조명도 채널 2 로 맞춰야 대상이 보인다.
	StageLightingChannels.bChannel0 = false;
	StageLightingChannels.bChannel1 = false;
	StageLightingChannels.bChannel2 = true;
}

void ALastFPSPreviewStageActor::BeginPlay()
{
	Super::BeginPlay();

	// 태그는 생성자에서 넣어도 되지만, 태그 매니저가 준비된 뒤에 잡는 편이 안전하다.
	if (CharacterSlot && !CharacterSlot->GetSlotTag().IsValid())
	{
		CharacterSlot->SetSlotTag(LastFPSUITags::PreviewSlot_Character());
	}
	if (WeaponSlot && !WeaponSlot->GetSlotTag().IsValid())
	{
		WeaponSlot->SetSlotTag(LastFPSUITags::PreviewSlot_Weapon());
	}

	CollectSlotsAndViews();

	// 레벨 시작에 미리 스폰되므로 기본은 꺼진 상태여야 한다. 켜는 것은 무대를 쓰는 화면의 책임이다.
	SetStageActive(false);
}

void ALastFPSPreviewStageActor::CollectSlotsAndViews()
{
	Slots.Reset();

	TArray<ULastFPSPreviewSlotComponent*> SlotComponents;
	GetComponents<ULastFPSPreviewSlotComponent>(SlotComponents);
	for (ULastFPSPreviewSlotComponent* SlotComponent : SlotComponents)
	{
		if (!SlotComponent)
		{
			continue;
		}

		const FGameplayTag SlotTag = SlotComponent->GetSlotTag();
		if (!SlotTag.IsValid())
		{
			UE_LOG(LogLastFPSPreviewStage, Warning,
				TEXT("%s: '%s' 자리에 SlotTag 가 없어 지목할 수 없습니다."),
				*GetName(), *SlotComponent->GetName());
			continue;
		}

		if (Slots.Contains(SlotTag))
		{
			UE_LOG(LogLastFPSPreviewStage, Warning,
				TEXT("%s: '%s' 태그를 쓰는 자리가 둘 이상입니다. 먼저 찾은 것만 씁니다."),
				*GetName(), *SlotTag.ToString());
			continue;
		}

		// 메시는 무대가 만들어 붙인다. BP 저작자는 자리 위치만 잡으면 된다.
		USkeletalMeshComponent* SlotMesh = NewObject<USkeletalMeshComponent>(this);
		SlotMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SlotMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		SlotMesh->RegisterComponent();
		SlotMesh->AttachToComponent(SlotComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SlotMesh->SetVisibility(false, true);

		FLastFPSPreviewSlotRuntime SlotRuntime;
		SlotRuntime.Slot = SlotComponent;
		SlotRuntime.Mesh = SlotMesh;
		Slots.Add(SlotTag, MoveTemp(SlotRuntime));
	}

	if (Slots.Num() == 0)
	{
		UE_LOG(LogLastFPSPreviewStage, Warning,
			TEXT("%s: 자리가 하나도 없습니다. 무대에 아무것도 세울 수 없습니다."), *GetName());
	}

	Views.Reset();
	GetComponents<ULastFPSPreviewViewComponent>(Views);
	if (Views.Num() == 0)
	{
		UE_LOG(LogLastFPSPreviewStage, Warning,
			TEXT("%s: 시점 카메라가 하나도 없습니다. 무대를 보면 화면이 비어 보입니다."), *GetName());
		return;
	}

	// 어느 시점이 켜질지 배치 순서에 좌우되지 않도록 전부 끈 뒤, 첫 시점 하나만 켜 둔다.
	for (ULastFPSPreviewViewComponent* View : Views)
	{
		if (View)
		{
			View->SetActive(false);
		}
	}
	if (ULastFPSPreviewViewComponent* FirstView = Views[0])
	{
		FirstView->SetActive(true);
		ActiveViewTag = FirstView->GetViewTag();
	}
}

FLastFPSPreviewSlotRuntime* ALastFPSPreviewStageActor::FindSlot(const FGameplayTag& SlotTag)
{
	FLastFPSPreviewSlotRuntime* Found = Slots.Find(SlotTag);
	if (!Found)
	{
		// 태그 오타나 자리 누락을 조용히 넘기면 아무것도 안 보이는 이유를 찾을 수 없다.
		UE_LOG(LogLastFPSPreviewStage, Warning,
			TEXT("%s: '%s' 자리를 찾지 못했습니다."), *GetName(), *SlotTag.ToString());
	}
	return Found;
}

bool ALastFPSPreviewStageActor::SetSubject(const FGameplayTag& SlotTag, const FLastFPSPreviewSubject& InSubject)
{
	FLastFPSPreviewSlotRuntime* SlotRuntime = FindSlot(SlotTag);
	if (!SlotRuntime || !SlotRuntime->Mesh)
	{
		return false;
	}

	// 액터로 세우는 경로. 조립된 대상은 클래스를 그대로 세워야 구성이 어긋나지 않는다.
	if (InSubject.ActorClass)
	{
		RebuildSpawnedActor(*SlotRuntime, InSubject);

		// 액터가 자기 메시를 들고 오므로 자리의 메시 컴포넌트는 비워 둔다.
		if (USkeletalMeshComponent* SlotMeshComponent = SlotRuntime->Mesh)
		{
			SlotMeshComponent->SetSkeletalMeshAsset(nullptr);
			SlotMeshComponent->SetVisibility(false, true);
		}
		RebuildAttachments(*SlotRuntime, InSubject.Attachments);

		if (SlotRuntime->Slot)
		{
			SlotRuntime->Slot->SetRelativeRotation(FRotator::ZeroRotator);
		}
		SlotRuntime->bFaceActiveView = InSubject.bFaceActiveView;
		ApplyLightingChannels(*SlotRuntime);
		ApplyTickState(*SlotRuntime, bStageActive);
		RefreshAlignment();
		return true;
	}

	if (!InSubject.Mesh)
	{
		return ClearSubject(SlotTag);
	}

	// 메시만 세우는 경로로 넘어왔으니 이전에 세워 둔 액터는 치운다.
	if (SlotRuntime->SpawnedActor)
	{
		SlotRuntime->SpawnedActor->Destroy();
		SlotRuntime->SpawnedActor = nullptr;
	}

	USkeletalMeshComponent& SlotMesh = *SlotRuntime->Mesh;
	SlotMesh.SetSkeletalMeshAsset(InSubject.Mesh);
	ApplyPose(SlotMesh, InSubject);

	SlotMesh.SetRelativeRotation(InSubject.MeshRotation);
	SlotMesh.SetVisibility(true, true);

	// 소켓 위치는 메시가 바뀐 뒤라야 유효하므로 포즈까지 정한 다음에 붙인다.
	RebuildAttachments(*SlotRuntime, InSubject.Attachments);

	// 대상이 바뀌면 이전 대상을 보던 각도가 남지 않게 정면으로 되돌린다.
	if (SlotRuntime->Slot)
	{
		SlotRuntime->Slot->SetRelativeRotation(FRotator::ZeroRotator);
	}
	SlotRuntime->bFaceActiveView = InSubject.bFaceActiveView;
	ApplyLightingChannels(*SlotRuntime);
	ApplyTickState(*SlotRuntime, bStageActive);

	// 대상이 바뀌면 바운드도 바뀐다. 방향과 초점을 정해진 순서로 함께 다시 잡는다.
	RefreshAlignment();
	return true;
}

bool ALastFPSPreviewStageActor::ClearSubject(const FGameplayTag& SlotTag)
{
	FLastFPSPreviewSlotRuntime* SlotRuntime = FindSlot(SlotTag);
	if (!SlotRuntime)
	{
		return false;
	}

	RebuildAttachments(*SlotRuntime, {});

	if (SlotRuntime->SpawnedActor)
	{
		SlotRuntime->SpawnedActor->Destroy();
		SlotRuntime->SpawnedActor = nullptr;
	}

	if (USkeletalMeshComponent* SlotMesh = SlotRuntime->Mesh)
	{
		SlotMesh->SetAnimInstanceClass(nullptr);
		SlotMesh->SetSkeletalMeshAsset(nullptr);
		SlotMesh->SetVisibility(false, true);
	}
	return true;
}

bool ALastFPSPreviewStageActor::AddYaw(const FGameplayTag& SlotTag, const float DeltaDegrees)
{
	FLastFPSPreviewSlotRuntime* SlotRuntime = FindSlot(SlotTag);
	if (!SlotRuntime || !SlotRuntime->Slot)
	{
		return false;
	}

	SlotRuntime->Slot->AddLocalRotation(FRotator(0.f, DeltaDegrees, 0.f));
	return true;
}

void ALastFPSPreviewStageActor::ApplyPose(
	USkeletalMeshComponent& TargetMesh, const FLastFPSPreviewSubject& InSubject)
{
	if (InSubject.AnimInstanceClass)
	{
		TargetMesh.SetAnimationMode(EAnimationMode::AnimationBlueprint);
		TargetMesh.SetAnimInstanceClass(InSubject.AnimInstanceClass);
		return;
	}

	TargetMesh.SetAnimInstanceClass(nullptr);

	if (InSubject.IdleAnimation)
	{
		// PlayAnimation 이 단일 노드 모드로 바꾼 뒤 재생까지 처리한다. ABP 를 만들 필요가 없다.
		TargetMesh.PlayAnimation(InSubject.IdleAnimation, /*bLooping=*/true);
		return;
	}

	// 지정이 없으면 레퍼런스 포즈. 무기처럼 정지 상태면 이걸로 충분하다.
	TargetMesh.SetAnimationMode(EAnimationMode::AnimationCustomMode);
}

void ALastFPSPreviewStageActor::TrimMeshComponents(
	TArray<TObjectPtr<USkeletalMeshComponent>>& Pool, const int32 KeepCount)
{
	// 남는 컴포넌트를 걷어낸다. 대상이 바뀔 때마다 쌓이면 컴포넌트가 샌다.
	for (int32 Index = Pool.Num() - 1; Index >= KeepCount; --Index)
	{
		if (USkeletalMeshComponent* Spare = Pool[Index])
		{
			Spare->DestroyComponent();
		}
		Pool.RemoveAt(Index);
	}
}

USkeletalMeshComponent* ALastFPSPreviewStageActor::AcquireMeshComponent(
	TArray<TObjectPtr<USkeletalMeshComponent>>& Pool, const int32 Index)
{
	if (Pool.IsValidIndex(Index) && Pool[Index])
	{
		return Pool[Index];
	}

	USkeletalMeshComponent* NewMesh = NewObject<USkeletalMeshComponent>(this);
	NewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NewMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	NewMesh->RegisterComponent();

	if (Pool.IsValidIndex(Index))
	{
		Pool[Index] = NewMesh;
	}
	else
	{
		Pool.Add(NewMesh);
	}
	return NewMesh;
}

void ALastFPSPreviewStageActor::ConfigureSpawnedActorForPreview(
	AActor& SpawnedActor, const FLastFPSPreviewSubject& InSubject)
{
	// 전시용이라 물리·이동·게임플레이 틱이 필요 없다. 스켈레탈 메시 컴포넌트는 자기 틱으로
	// 포즈를 갱신하므로 액터 틱을 꺼도 애니메이션은 돈다.
	SpawnedActor.SetActorEnableCollision(false);
	SpawnedActor.SetActorTickEnabled(false);

	if (ACharacter* SpawnedCharacter = Cast<ACharacter>(&SpawnedActor))
	{
		if (UCharacterMovementComponent* Movement = SpawnedCharacter->GetCharacterMovement())
		{
			// 중력에 끌려 무대 아래로 떨어지지 않게 한다.
			Movement->DisableMovement();
			Movement->SetComponentTickEnabled(false);
		}
	}

	// 지정이 없으면 액터 BP 에 저작된 ABP 를 그대로 둔다. 빙의가 안 돼 이동 값이 0 이라
	// 대개 대기 자세로 서지만, 이동을 전제로 만든 ABP 면 아래 두 방법으로 덮어쓴다.
	if (!InSubject.AnimInstanceClass && !InSubject.IdleAnimation)
	{
		return;
	}

	// 포즈는 주 메시에만 물린다. 의상 파츠는 LeaderPose 로 주 메시를 따라가므로 건드리면 어긋난다.
	USkeletalMeshComponent* PrimaryMesh = ResolvePrimaryMesh(SpawnedActor);
	if (!PrimaryMesh)
	{
		return;
	}

	if (InSubject.AnimInstanceClass)
	{
		PrimaryMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		PrimaryMesh->SetAnimInstanceClass(InSubject.AnimInstanceClass);
		return;
	}

	// PlayAnimation 이 단일 노드 모드로 바꾼 뒤 재생까지 처리한다. 자세 하나면 ABP 가 필요 없다.
	PrimaryMesh->PlayAnimation(InSubject.IdleAnimation, /*bLooping=*/true);
}

void ALastFPSPreviewStageActor::RebuildSpawnedActor(
	FLastFPSPreviewSlotRuntime& SlotRuntime, const FLastFPSPreviewSubject& InSubject)
{
	// 같은 클래스가 이미 서 있으면 다시 만들지 않는다. 탭을 오갈 때마다 캐릭터를 재생성하면
	// 그 프레임에 스폰 비용을 그대로 낸다.
	if (SlotRuntime.SpawnedActor && SlotRuntime.SpawnedActor->GetClass() == InSubject.ActorClass)
	{
		ConfigureSpawnedActorForPreview(*SlotRuntime.SpawnedActor, InSubject);
		return;
	}

	if (SlotRuntime.SpawnedActor)
	{
		SlotRuntime.SpawnedActor->Destroy();
		SlotRuntime.SpawnedActor = nullptr;
	}

	if (!InSubject.ActorClass || !SlotRuntime.Slot)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.ObjectFlags = RF_Transient;
	SpawnParameters.Owner = this;

	AActor* NewActor = World->SpawnActor<AActor>(
		InSubject.ActorClass,
		SlotRuntime.Slot->GetComponentTransform(),
		SpawnParameters);
	if (!NewActor)
	{
		UE_LOG(LogLastFPSPreviewStage, Error,
			TEXT("%s: 프리뷰 대상 액터를 스폰하지 못했습니다: %s"),
			*GetName(), *GetNameSafe(InSubject.ActorClass));
		return;
	}

	NewActor->AttachToComponent(
		SlotRuntime.Slot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	NewActor->SetActorRelativeRotation(InSubject.MeshRotation);

	ConfigureSpawnedActorForPreview(*NewActor, InSubject);
	SlotRuntime.SpawnedActor = NewActor;
}

USkeletalMeshComponent* ALastFPSPreviewStageActor::ResolvePrimaryMesh(const AActor& InActor)
{
	if (const ACharacter* SpawnedCharacter = Cast<ACharacter>(&InActor))
	{
		if (USkeletalMeshComponent* CharacterMesh = SpawnedCharacter->GetMesh())
		{
			return CharacterMesh;
		}
	}

	// Character 가 아니면 LeaderPose 에 묶이지 않은 첫 메시를 주 메시로 본다.
	TArray<USkeletalMeshComponent*> SpawnedMeshes;
	InActor.GetComponents<USkeletalMeshComponent>(SpawnedMeshes);
	for (USkeletalMeshComponent* SpawnedMesh : SpawnedMeshes)
	{
		if (SpawnedMesh && !SpawnedMesh->LeaderPoseComponent.IsValid())
		{
			return SpawnedMesh;
		}
	}
	return nullptr;
}

USkeletalMeshComponent* ALastFPSPreviewStageActor::ResolveAttachParentMesh(
	const FLastFPSPreviewSlotRuntime& SlotRuntime)
{
	// 액터를 세운 자리는 그 액터의 주 메시가 기준이다. 소켓이 거기 있기 때문이다.
	if (const AActor* SpawnedActor = SlotRuntime.SpawnedActor)
	{
		if (USkeletalMeshComponent* PrimaryMesh = ResolvePrimaryMesh(*SpawnedActor))
		{
			return PrimaryMesh;
		}
	}

	return SlotRuntime.Mesh;
}

void ALastFPSPreviewStageActor::RebuildAttachments(
	FLastFPSPreviewSlotRuntime& SlotRuntime, const TArray<FLastFPSPreviewAttachment>& InAttachments)
{
	TrimMeshComponents(SlotRuntime.Attachments, InAttachments.Num());

	USkeletalMeshComponent* AttachParentMesh = ResolveAttachParentMesh(SlotRuntime);
	if (!AttachParentMesh)
	{
		return;
	}

	for (int32 Index = 0; Index < InAttachments.Num(); ++Index)
	{
		const FLastFPSPreviewAttachment& Attachment = InAttachments[Index];
		USkeletalMeshComponent* AttachmentMesh = AcquireMeshComponent(SlotRuntime.Attachments, Index);

		AttachmentMesh->SetSkeletalMeshAsset(Attachment.Mesh);

		if (!Attachment.SocketName.IsNone() && !AttachParentMesh->DoesSocketExist(Attachment.SocketName))
		{
			// 소켓 이름이 틀리면 무기가 메시 원점(발밑)에 붙어 조용히 이상하게 보인다.
			UE_LOG(LogLastFPSPreviewStage, Warning,
				TEXT("%s: '%s' 소켓이 %s 에 없어 부속이 메시 원점에 붙습니다."),
				*GetName(), *Attachment.SocketName.ToString(), *AttachParentMesh->GetName());
		}

		// 소켓에 스냅한 뒤 보정 변환을 얹는다. 소켓이 없으면 메시 루트에 붙는다.
		AttachmentMesh->AttachToComponent(
			AttachParentMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			Attachment.SocketName);
		AttachmentMesh->SetRelativeTransform(Attachment.RelativeTransform);
		AttachmentMesh->SetVisibility(Attachment.Mesh != nullptr, true);
	}
}

bool ALastFPSPreviewStageActor::SetActiveView(const FGameplayTag& ViewTag)
{
	if (!ViewTag.IsValid())
	{
		// 지목이 없으면 지금 시점을 유지한다. 대상만 올리고 시점은 그대로 두는 경우다.
		return false;
	}

	ULastFPSPreviewViewComponent* TargetView = nullptr;
	for (ULastFPSPreviewViewComponent* View : Views)
	{
		if (View && View->GetViewTag() == ViewTag)
		{
			TargetView = View;
			break;
		}
	}

	if (!TargetView)
	{
		UE_LOG(LogLastFPSPreviewStage, Warning,
			TEXT("%s: '%s' 시점 카메라를 찾지 못해 시점을 바꾸지 않았습니다."),
			*GetName(), *ViewTag.ToString());
		return false;
	}

	// 액터가 뷰타깃일 때 켜져 있는 카메라 컴포넌트가 곧 화면이 된다. 하나만 남긴다.
	for (ULastFPSPreviewViewComponent* View : Views)
	{
		if (View)
		{
			View->SetActive(View == TargetView);
		}
	}

	ActiveViewTag = ViewTag;

	// 시점이 바뀌면 정면과 초점을 함께 다시 잡는다. 순서는 RefreshAlignment 가 단독으로 소유한다.
	RefreshAlignment();
	return true;
}

void ALastFPSPreviewStageActor::ApplyLightingChannels(const FLastFPSPreviewSlotRuntime& SlotRuntime) const
{
	if (!bIsolateStageLighting)
	{
		return;
	}

	const auto MoveToStageChannels = [this](UPrimitiveComponent* Primitive)
	{
		if (Primitive)
		{
			Primitive->SetLightingChannels(
				StageLightingChannels.bChannel0,
				StageLightingChannels.bChannel1,
				StageLightingChannels.bChannel2);
		}
	};

	MoveToStageChannels(SlotRuntime.Mesh);
	for (USkeletalMeshComponent* AttachmentMesh : SlotRuntime.Attachments)
	{
		MoveToStageChannels(AttachmentMesh);
	}

	// 세운 액터는 모듈러 파츠까지 프리미티브가 여럿이다. 하나라도 빠지면 그 부분만 레벨 조명을 받아
	// 몸과 옷의 밝기가 어긋난다.
	if (const AActor* SpawnedActor = SlotRuntime.SpawnedActor)
	{
		TArray<UPrimitiveComponent*> SpawnedPrimitives;
		SpawnedActor->GetComponents<UPrimitiveComponent>(SpawnedPrimitives);
		for (UPrimitiveComponent* SpawnedPrimitive : SpawnedPrimitives)
		{
			MoveToStageChannels(SpawnedPrimitive);
		}
	}
}

void ALastFPSPreviewStageActor::RefreshAlignment()
{
	// 1) 대상이 카메라를 보게 돌린다. 이때 메시 바운드 중심이 움직인다.
	for (TPair<FGameplayTag, FLastFPSPreviewSlotRuntime>& SlotPair : Slots)
	{
		ApplySlotFacing(SlotPair.Value);
	}

	// 2) 그 다음에 카메라 초점을 잡는다. 순서가 뒤집히면 직전 프레임의 낡은 바운드를 겨눈다.
	if (ULastFPSPreviewViewComponent* ActiveView = FindActiveView())
	{
		ApplyViewFocus(*ActiveView);
	}

	// 3) 카메라가 최종 위치를 잡은 뒤에 시선 타깃을 넘긴다.
	PushGazeTarget();
}

void ALastFPSPreviewStageActor::PushGazeTarget()
{
	const ULastFPSPreviewViewComponent* ActiveView = FindActiveView();
	if (!ActiveView)
	{
		return;
	}

	const FVector GazeLocation = ActiveView->GetComponentLocation();
	const float GazeAlpha = ActiveView->GetGazeAlpha();

	for (const TPair<FGameplayTag, FLastFPSPreviewSlotRuntime>& SlotPair : Slots)
	{
		const FLastFPSPreviewSlotRuntime& SlotRuntime = SlotPair.Value;

		USkeletalMeshComponent* PoseMesh = SlotRuntime.SpawnedActor
			? ResolvePrimaryMesh(*SlotRuntime.SpawnedActor)
			: SlotRuntime.Mesh.Get();
		if (!PoseMesh)
		{
			continue;
		}

		UAnimInstance* AnimInstance = PoseMesh->GetAnimInstance();
		if (AnimInstance && AnimInstance->Implements<ULastFPSPreviewGazeTarget>())
		{
			ILastFPSPreviewGazeTarget::Execute_SetPreviewGazeTarget(AnimInstance, GazeLocation, GazeAlpha);
		}
	}
}

ULastFPSPreviewViewComponent* ALastFPSPreviewStageActor::FindActiveView() const
{
	for (ULastFPSPreviewViewComponent* View : Views)
	{
		if (View && View->IsActive())
		{
			return View;
		}
	}
	return nullptr;
}

void ALastFPSPreviewStageActor::ApplySlotFacing(FLastFPSPreviewSlotRuntime& SlotRuntime)
{
	if (!SlotRuntime.bFaceActiveView || !SlotRuntime.Slot)
	{
		return;
	}

	const ULastFPSPreviewViewComponent* ActiveView = FindActiveView();
	if (!ActiveView)
	{
		return;
	}

	const FVector SlotLocation = SlotRuntime.Slot->GetComponentLocation();
	const FVector ToView = ActiveView->GetComponentLocation() - SlotLocation;

	// 수평 성분만 쓴다. 카메라가 위에 있다고 대상이 뒤로 눕거나 앞으로 숙이면 안 된다.
	const FVector FlatToView(ToView.X, ToView.Y, 0.0);
	if (FlatToView.IsNearlyZero())
	{
		UE_LOG(LogLastFPSPreviewStage, Warning,
			TEXT("%s: 시점이 자리 바로 위/아래에 있어 정면 방향을 정할 수 없습니다."), *GetName());
		return;
	}

	// 자리의 전방(+X)이 카메라를 향하게 돌린다. 대상은 자리에 스냅돼 있어 함께 돌아간다.
	SlotRuntime.Slot->SetWorldRotation(FlatToView.Rotation());
}

FVector ALastFPSPreviewStageActor::GetSlotFocusLocation(const FGameplayTag& SlotTag)
{
	const FLastFPSPreviewSlotRuntime* SlotRuntime = Slots.Find(SlotTag);
	if (!SlotRuntime)
	{
		return GetActorLocation();
	}

	// 자리 원점이 아니라 메시 바운드 중심을 겨눈다. 캐릭터는 발이 원점이라
	// 원점을 겨누면 카메라가 바닥을 보게 된다.
	// 액터로 세운 대상은 액터 전체 바운드가 기준이다.
	if (const AActor* SpawnedActor = SlotRuntime->SpawnedActor)
	{
		FVector BoundsOrigin = FVector::ZeroVector;
		FVector BoundsExtent = FVector::ZeroVector;
		SpawnedActor->GetActorBounds(/*bOnlyCollidingComponents=*/false, BoundsOrigin, BoundsExtent);
		return BoundsOrigin;
	}

	if (const USkeletalMeshComponent* SlotMesh = SlotRuntime->Mesh)
	{
		if (SlotMesh->GetSkeletalMeshAsset())
		{
			return SlotMesh->Bounds.Origin;
		}
	}

	return SlotRuntime->Slot ? SlotRuntime->Slot->GetComponentLocation() : GetActorLocation();
}

void ALastFPSPreviewStageActor::ApplyViewFocus(ULastFPSPreviewViewComponent& View)
{
	const FGameplayTag FocusSlotTag = View.GetFocusSlotTag();
	if (!FocusSlotTag.IsValid())
	{
		return;
	}

	if (!View.ShouldLookAtFocusSlot() && !View.ShouldFocusDepthOfField())
	{
		return;
	}

	const FVector FocusLocation = GetSlotFocusLocation(FocusSlotTag);
	const FVector ViewLocation = View.GetComponentLocation();
	const FVector ToFocus = FocusLocation - ViewLocation;

	if (ToFocus.IsNearlyZero())
	{
		// 카메라가 대상 안에 들어가 있으면 바라볼 방향이 없다. 배치 실수를 조용히 넘기지 않는다.
		UE_LOG(LogLastFPSPreviewStage, Warning,
			TEXT("%s: '%s' 시점이 대상과 같은 위치라 초점을 맞출 수 없습니다."),
			*GetName(), *View.GetViewTag().ToString());
		return;
	}

	if (View.ShouldLookAtFocusSlot())
	{
		View.SetWorldRotation(ToFocus.Rotation());
	}

	if (View.ShouldFocusDepthOfField())
	{
		// 조리개 등 나머지 심도 설정은 BP 에 저작된 값을 그대로 두고 거리만 맞춘다.
		View.PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;
		View.PostProcessSettings.DepthOfFieldFocalDistance = static_cast<float>(ToFocus.Size());
	}
}

void ALastFPSPreviewStageActor::ApplyTickState(
	const FLastFPSPreviewSlotRuntime& SlotRuntime, const bool bInActive)
{
	const EVisibilityBasedAnimTickOption TickOption = bInActive
		? EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones
		: EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	if (USkeletalMeshComponent* SlotMesh = SlotRuntime.Mesh)
	{
		SlotMesh->VisibilityBasedAnimTickOption = TickOption;
		SlotMesh->SetComponentTickEnabled(bInActive);
	}

	for (USkeletalMeshComponent* AttachmentMesh : SlotRuntime.Attachments)
	{
		if (AttachmentMesh)
		{
			AttachmentMesh->VisibilityBasedAnimTickOption = TickOption;
			AttachmentMesh->SetComponentTickEnabled(bInActive);
		}
	}

	// 세워 둔 액터의 메시도 화면 밖에서 본을 돌리지 않게 함께 끈다.
	if (const AActor* SpawnedActor = SlotRuntime.SpawnedActor)
	{
		TArray<USkeletalMeshComponent*> SpawnedMeshes;
		SpawnedActor->GetComponents<USkeletalMeshComponent>(SpawnedMeshes);
		for (USkeletalMeshComponent* SpawnedMesh : SpawnedMeshes)
		{
			if (SpawnedMesh)
			{
				SpawnedMesh->VisibilityBasedAnimTickOption = TickOption;
				SpawnedMesh->SetComponentTickEnabled(bInActive);
			}
		}
	}
}

void ALastFPSPreviewStageActor::SetStageActive(const bool bInActive)
{
	bStageActive = bInActive;

	SetActorHiddenInGame(!bInActive);

	// ChildActorComponent 로 붙인 배경은 별도 액터라 SetActorHiddenInGame 이 전파되지 않는다.
	// 그대로 두면 메뉴를 닫아도 배경이 계속 그려진다.
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);
	for (AActor* AttachedActor : AttachedActors)
	{
		if (AttachedActor)
		{
			AttachedActor->SetActorHiddenInGame(!bInActive);
		}
	}

	// 꺼진 동안 포즈 갱신을 멈춘다. 무대는 화면 밖에 있어 AlwaysTick 이면 계속 본을 돌린다.
	for (const TPair<FGameplayTag, FLastFPSPreviewSlotRuntime>& SlotPair : Slots)
	{
		ApplyTickState(SlotPair.Value, bInActive);
	}
}
