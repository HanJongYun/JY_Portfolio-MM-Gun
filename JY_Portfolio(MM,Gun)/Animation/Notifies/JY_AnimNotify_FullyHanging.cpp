// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/Notifies/JY_AnimNotify_FullyHanging.h"
#include "Components/SkeletalMeshComponent.h"
#include "Actors/JY_Character.h"

void UJY_AnimNotify_FullyHanging::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
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

	Character->SetFullyHanging(true);
}

FString UJY_AnimNotify_FullyHanging::GetNotifyName_Implementation() const
{
	return TEXT("Fully Hanging");
}
