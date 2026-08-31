#include "Components/JY_EquipmentComponent.h"

#include "Actors/JY_WeaponActor.h"
#include "Equipment/JY_EquipmentInstance.h"
#include "Data/JY_WeaponData.h"
#include "Data/JY_WeaponGlobalData.h"
#include "Core/JY_GlobalSettings.h"
#include "Components/JY_AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "AbilitySystemInterface.h"
#include "Engine/World.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"

UJY_EquipmentComponent::UJY_EquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UJY_EquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	const UJY_GlobalSettings* Settings = GetDefault<UJY_GlobalSettings>();
	if (Settings != nullptr)
	{
		CachedWeaponGlobalData = Settings->WeaponGlobalData.LoadSynchronous();
	}

	const AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
	{
		return;
	}

	for (EJY_WeaponAnimType Type : DefaultSlots)
	{
		AddWeapon(Type);
	}
}

void UJY_EquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UJY_EquipmentComponent, SpawnedWeapons);
	DOREPLIFETIME(UJY_EquipmentComponent, ActiveSlotIndex);
	DOREPLIFETIME(UJY_EquipmentComponent, ActiveEquipmentInstance);
}

bool UJY_EquipmentComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) //override
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	if (IsValid(ActiveEquipmentInstance) == true)
	{
		bWroteSomething |= Channel->ReplicateSubobject(ActiveEquipmentInstance, *Bunch, *RepFlags);
	}

	return bWroteSomething;
}

void UJY_EquipmentComponent::UninitializeComponent() //override
{
	const AActor* Owner = GetOwner();
	if (Owner != nullptr && Owner->HasAuthority() == true)
	{
		UnequipItem(ActiveEquipmentInstance);
	}

	Super::UninitializeComponent();
}


int32 UJY_EquipmentComponent::AddWeapon(EJY_WeaponAnimType Type)
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
	{
		return INDEX_NONE;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return INDEX_NONE;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.Instigator = Cast<APawn>(Owner);

	AJY_WeaponActor* NewWeapon = World->SpawnActor<AJY_WeaponActor>(AJY_WeaponActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (NewWeapon == nullptr)
	{
		return INDEX_NONE;
	}

	NewWeapon->InitializeFromType(Type);

	const int32 NewIndex = SpawnedWeapons.Add(NewWeapon);

	if (UJY_WeaponData* Data = NewWeapon->GetWeaponData())
	{
		AttachWeaponToSocket(NewWeapon, Data->HolsterSocketName, Data->HolsterRelativeTransform);
	}

	return NewIndex;
}

void UJY_EquipmentComponent::SetActiveSlot(int32 NewIndex)
{
	const AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
	{
		return;
	}

	if (NewIndex < 0 || NewIndex >= SpawnedWeapons.Num())
	{
		NewIndex = INDEX_NONE;
	}

	const int32 OldIndex = ActiveSlotIndex;
	if (NewIndex == OldIndex)
	{
		return;
	}

	/* 이전 무기 어빌리티 회수 */
	UnequipItem(ActiveEquipmentInstance);
	ActiveSlotIndex = NewIndex;

	if (SpawnedWeapons.IsValidIndex(ActiveSlotIndex) && SpawnedWeapons[ActiveSlotIndex] != nullptr)
	{
		EquipItem(SpawnedWeapons[ActiveSlotIndex]);
	}

	/* 몽타주 재생, 부착은 노티파이에서 */
	PlayEquipTransition(OldIndex, NewIndex);
}

void UJY_EquipmentComponent::RequestActiveSlot(int32 Index)
{
	const AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	if (Owner->HasAuthority() == true)
	{
		/* 같은 슬롯이면 토글 해제 */
		SetActiveSlot(Index == ActiveSlotIndex ? INDEX_NONE : Index);
	}
	else
	{
		Server_SetActiveSlot(Index);
	}
}

void UJY_EquipmentComponent::Server_SetActiveSlot_Implementation(int32 Index)
{
	SetActiveSlot(Index == ActiveSlotIndex ? INDEX_NONE : Index);
}

void UJY_EquipmentComponent::OnRep_Weapons()
{
	RefreshAttachments();
}

void UJY_EquipmentComponent::OnRep_ActiveSlot(int32 OldActiveSlotIndex)
{
	PlayEquipTransition(OldActiveSlotIndex, ActiveSlotIndex);
}

void UJY_EquipmentComponent::OnRep_ActiveEquipmentInstance(UJY_EquipmentInstance* OldInstance)
{
	if (OldInstance != nullptr)
	{
		OldInstance->OnUnequipped();
	}

	if (ActiveEquipmentInstance != nullptr)
	{
		ActiveEquipmentInstance->OnEquipped();
	}
}

UJY_EquipmentInstance* UJY_EquipmentComponent::EquipItem(AJY_WeaponActor* Weapon)
{
	const AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false || Weapon == nullptr)
		return nullptr;

	ACharacter* OwnerCharacter = GetOwnerCharacter();
	UJY_WeaponData* WeaponData = Weapon->GetWeaponData();
	if (OwnerCharacter == nullptr || WeaponData == nullptr)
		return nullptr;

	TSubclassOf<UJY_EquipmentInstance> InstanceType = WeaponData->InstanceType;
	if (InstanceType == nullptr)
	{
		InstanceType = UJY_EquipmentInstance::StaticClass();
	}

	ActiveEquipmentInstance = NewObject<UJY_EquipmentInstance>(OwnerCharacter, InstanceType);
	if (ActiveEquipmentInstance == nullptr)
		return nullptr;

	ActiveEquipmentInstance->SetInstigator(Weapon);
	ActiveEquipmentInstance->AddSpawnedActor(Weapon);

	GrantWeaponAbilities();
	ActiveEquipmentInstance->OnEquipped();

	return ActiveEquipmentInstance;
}

void UJY_EquipmentComponent::UnequipItem(UJY_EquipmentInstance* ItemInstance)
{
	const AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false || ItemInstance == nullptr)
		return;

	ItemInstance->OnUnequipped();
	ActiveGrantedHandles.TakeFromAbilitySystem(GetASC());

	if (ActiveEquipmentInstance == ItemInstance)
	{
		ActiveEquipmentInstance = nullptr;
	}
}

void UJY_EquipmentComponent::PlayEquipTransition(int32 OldIndex, int32 NewIndex)
{
	/* 뽑기면 EquipMontage, 수납이면 UnequipMontage */
	UAnimMontage* Montage = nullptr;
	const bool bEquipping = (NewIndex != INDEX_NONE);

	const int32 MontageSourceIndex = bEquipping ? NewIndex : OldIndex;
	if (SpawnedWeapons.IsValidIndex(MontageSourceIndex) && SpawnedWeapons[MontageSourceIndex] != nullptr)
	{
		if (UJY_WeaponData* WeaponData = SpawnedWeapons[MontageSourceIndex]->GetWeaponData())
		{
			Montage = bEquipping ? WeaponData->EquipMontage : WeaponData->UnequipMontage;
		}
	}

	/* 몽타주 재생, 부착은 노티파이에서 */
	if (Montage != nullptr)
	{
		ACharacter* Character = GetOwnerCharacter();
		USkeletalMeshComponent* Mesh = (Character != nullptr) ? Character->GetMesh() : nullptr;
		UAnimInstance* AnimInstance = (Mesh != nullptr) ? Mesh->GetAnimInstance() : nullptr;
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(Montage);
			return;
		}
	}

	/* 몽타주 없으면 즉시 스왑 */
	RefreshAttachments();
}

void UJY_EquipmentComponent::OnWeaponSwapNotify()
{
	/* WeaponSwap 노티파이, 부착 스왑 */
	RefreshAttachments();
	EquippedWeaponType = GetActiveWeaponAnimType();
}

void UJY_EquipmentComponent::RefreshAttachments()
{
	for (int32 Index = 0; Index < SpawnedWeapons.Num(); ++Index)
	{
		AJY_WeaponActor* Weapon = SpawnedWeapons[Index];
		if (Weapon == nullptr)
		{
			continue;
		}

		UJY_WeaponData* WeaponData = Weapon->GetWeaponData();
		if (WeaponData == nullptr)
		{
			continue;
		}

		const bool bActive = (Index == ActiveSlotIndex);
		const FName SocketName = bActive ? WeaponData->GripSocketName : WeaponData->HolsterSocketName;
		const FTransform& RelativeTransform = bActive ? WeaponData->GripRelativeTransform : WeaponData->HolsterRelativeTransform;

		AttachWeaponToSocket(Weapon, SocketName, RelativeTransform);
	}
}

void UJY_EquipmentComponent::AttachWeaponToSocket(AJY_WeaponActor* Weapon, FName SocketName, const FTransform& RelativeTransform)
{
	if (Weapon == nullptr)
	{
		return;
	}

	ACharacter* Character = GetOwnerCharacter();
	if (Character == nullptr)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (Mesh == nullptr)
	{
		return;
	}

	Weapon->AttachToComponent(Mesh, FAttachmentTransformRules::KeepRelativeTransform, SocketName);
	Weapon->SetActorRelativeTransform(RelativeTransform);
}

void UJY_EquipmentComponent::GrantWeaponAbilities()
{
	/* [Server] */
	UJY_AbilitySystemComponent* ASC = GetASC();
	if (ASC == nullptr)
	{
		return;
	}

	if (ActiveSlotIndex < 0 || ActiveSlotIndex >= SpawnedWeapons.Num())
	{
		return;
	}

	AJY_WeaponActor* Weapon = SpawnedWeapons[ActiveSlotIndex];
	if (Weapon == nullptr)
	{
		return;
	}

	UJY_WeaponData* WeaponData = Weapon->GetWeaponData();
	if (WeaponData == nullptr || WeaponData->AbilitySet == nullptr)
	{
		return;
	}

	if (ActiveEquipmentInstance == nullptr)
	{
		return;
	}

	WeaponData->AbilitySet->GiveToAbilitySystem(ASC, OUT &ActiveGrantedHandles, ActiveEquipmentInstance);
}

UJY_AbilitySystemComponent* UJY_EquipmentComponent::GetASC() const
{
	if (IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return Cast<UJY_AbilitySystemComponent>(AbilitySystemInterface->GetAbilitySystemComponent());
	}

	return nullptr;
}

ACharacter* UJY_EquipmentComponent::GetOwnerCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

AJY_WeaponActor* UJY_EquipmentComponent::GetActiveWeapon() const
{
	if (SpawnedWeapons.IsValidIndex(ActiveSlotIndex))
	{
		return SpawnedWeapons[ActiveSlotIndex];
	}
	return nullptr;
}

bool UJY_EquipmentComponent::GetLeftHandIKTargetWorld(FTransform& OutTransform) const
{
	AJY_WeaponActor* Weapon = GetActiveWeapon();
	if (Weapon == nullptr)
	{
		return false;
	}

	UJY_WeaponData* WeaponData = Weapon->GetWeaponData();
	USkeletalMeshComponent* WeaponMesh = Weapon->GetMeshComponent();
	if (WeaponData == nullptr || WeaponMesh == nullptr)
	{
		return false;
	}

	if (WeaponMesh->DoesSocketExist(WeaponData->LeftHandSocketName) == false)
	{
		return false;
	}

	OutTransform = WeaponMesh->GetSocketTransform(WeaponData->LeftHandSocketName, RTS_World);
	return true;
}

EJY_WeaponAnimType UJY_EquipmentComponent::GetActiveWeaponAnimType() const
{
	if (ActiveSlotIndex < 0 || ActiveSlotIndex >= SpawnedWeapons.Num())
	{
		return EJY_WeaponAnimType::Unarmed;
	}

	AJY_WeaponActor* Weapon = SpawnedWeapons[ActiveSlotIndex];
	return (Weapon != nullptr) ? Weapon->GetWeaponType() : EJY_WeaponAnimType::Unarmed;
}

UAnimSequenceBase* UJY_EquipmentComponent::GetActiveOverlayPose(EJY_OverlayPoseType PoseType) const
{
	UJY_WeaponGlobalData* GlobalData = CachedWeaponGlobalData;
	if (GlobalData == nullptr)
	{
		return nullptr;
	}

	UJY_WeaponData* WeaponData = GlobalData->GetWeaponData(OverlayType);
	if (WeaponData == nullptr)
	{
		return nullptr;
	}

	const TObjectPtr<UAnimSequenceBase>* Found = WeaponData->OverlayPoses.Find(PoseType);
	return (Found != nullptr) ? Found->Get() : nullptr;
}

UAimOffsetBlendSpace* UJY_EquipmentComponent::GetActiveAimOffset() const
{
	UJY_WeaponGlobalData* GlobalData = CachedWeaponGlobalData;
	if (GlobalData == nullptr)
	{
		return nullptr;
	}

	UJY_WeaponData* WeaponData = GlobalData->GetWeaponData(OverlayType);
	return (WeaponData != nullptr) ? WeaponData->AimOffset.Get() : nullptr;
}
