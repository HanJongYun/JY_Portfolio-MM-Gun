// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/Notifies/JY_AnimNotify_ProneGroundReady.h"
#include "Components/SkeletalMeshComponent.h"
#include "Actors/JY_Character.h"

void UJY_AnimNotify_ProneGroundReady::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
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

	Character->SetFullyProne(true);
}

FString UJY_AnimNotify_ProneGroundReady::GetNotifyName_Implementation() const
{
	return TEXT("Prone Ground Ready");
}
