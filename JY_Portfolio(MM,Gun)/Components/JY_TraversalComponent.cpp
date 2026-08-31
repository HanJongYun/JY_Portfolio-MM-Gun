// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/JY_TraversalComponent.h"
#include "Actors/JY_Character.h"


UJY_TraversalComponent::UJY_TraversalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UJY_TraversalComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AJY_Character>(GetOwner());
}
