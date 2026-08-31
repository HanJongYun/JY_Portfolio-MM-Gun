// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/Notifies/JY_AnimNotify_WeaponSwap.h"
#include "Components/SkeletalMeshComponent.h"
#include "Actors/JY_Character.h"
#include "Components/JY_EquipmentComponent.h"

void UJY_AnimNotify_WeaponSwap::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp == nullptr)
	{
		return;
	}

	AJY_Character* Character = Cast<AJY_Character>(MeshComp->GetOwner());
	if (Character == nullptr)
	{
		return;
	}

	UJY_EquipmentComponent* EquipmentComp = Character->GetEquipmentComponent();
	if (EquipmentComp != nullptr)
	{
		EquipmentComp->OnWeaponSwapNotify();
	}
}

FString UJY_AnimNotify_WeaponSwap::GetNotifyName_Implementation() const
{
	return TEXT("Weapon Swap");
}
