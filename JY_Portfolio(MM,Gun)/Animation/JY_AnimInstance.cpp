// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/JY_AnimInstance.h"
#include "Components/JY_ProneComponent.h"
#include "Components/JY_EquipmentComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"

#include "Actors/JY_Character.h"

void UJY_AnimInstance::NativeInitializeAnimation() //override
{
	Super::NativeInitializeAnimation();

	JYCharacter = Cast<AJY_Character>(TryGetPawnOwner());
	CachedSourceMesh = JYCharacter != nullptr ? JYCharacter->GetMesh() : nullptr;
	CachedSelfMesh = GetSkelMeshComponent();
}

void UJY_AnimInstance::NativeBeginPlay() //override
{
	Super::NativeBeginPlay();

	if (JYCharacter == nullptr)
	{
		JYCharacter = Cast<AJY_Character>(TryGetPawnOwner());
	}
}

void UJY_AnimInstance::NativeUpdateAnimation(float DeltaSeconds) //override
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	UpdateWeaponState(DeltaSeconds);
	UpdateHitReact(DeltaSeconds);
	UpdateProneGroundAlignment();
}

void UJY_AnimInstance::UpdateProneGroundAlignment()
{
	if (JYCharacter == nullptr || JYCharacter->IsFullyProne() == false)
		return;

	const UJY_ProneComponent* ProneComp = JYCharacter->GetProneComponent();
	if (ProneComp == nullptr)
		return;

	ProneGroundPitch = ProneComp->GetProneGroundPitch();
	ProneGroundRoll = ProneComp->GetProneGroundRoll();
	ProneGroundHeightOffset = ProneComp->GetProneGroundHeightOffset();
}

void UJY_AnimInstance::UpdateWeaponState(float DeltaSeconds)
{
	const UJY_EquipmentComponent* EquipmentComp = JYCharacter != nullptr ? JYCharacter->GetEquipmentComponent() : nullptr;
	if (EquipmentComp == nullptr)
	{
		return;
	}

	WeaponAnimType = EquipmentComp->GetActiveWeaponAnimType();

	FTransform LeftHandTarget;
	bHasLeftHandIKTarget = EquipmentComp->GetLeftHandIKTargetWorld(LeftHandTarget);
	LeftHandIKTargetLocation = bHasLeftHandIKTarget ? LeftHandTarget.GetLocation() : FVector::ZeroVector;
}

float UJY_AnimInstance::GetLeftIKAlpha() const
{
	if (bHasLeftHandIKTarget == false)
	{
		return 0.f;
	}

	/* 커브가 [-1,1]로 되어 있어 [0,1]로 환산 */
	static const FName WeaponLeftHandIKCurveName(TEXT("WeaponLeftHandIK"));
	return FMath::Clamp((GetCurveValue(WeaponLeftHandIKCurveName) + 1.f) * 0.5f, 0.f, 1.f);
}

void UJY_AnimInstance::RequestHitReact(EJY_HitReactDirection Direction)
{
	/* [Server 포함 전 클라] */
	if (const TObjectPtr<UAnimSequence>* Found = HitReactSequences.Find(Direction))
	{
		HitReactSequence = *Found;
	}

	++HitReactPlayCount;
	bHitReacting = true;
}

void UJY_AnimInstance::UpdateHitReact(float DeltaSeconds)
{
	if (bHitReacting == false)
		return;

	if (HitReactPlayCount != LastAppliedHitReactPlayCount)
	{
		HitReactPlayTime = 0.f;
		LastAppliedHitReactPlayCount = HitReactPlayCount;
	}
	else
	{
		HitReactPlayTime += DeltaSeconds;
	}

	const float SequenceLength = HitReactSequence != nullptr ? HitReactSequence->GetPlayLength() : 0.f;
	if (HitReactPlayTime >= SequenceLength)
	{
		bHitReacting = false;
	}
}

FVector UJY_AnimInstance::GetSourceSocketLocation(FName SocketName) const
{
	return CachedSourceMesh.IsValid() ? CachedSourceMesh->GetSocketLocation(SocketName) : FVector::ZeroVector;
}

FVector UJY_AnimInstance::GetTargetSocketLocation(FName SocketName) const
{
	return CachedSelfMesh.IsValid() ? CachedSelfMesh->GetSocketLocation(SocketName) : FVector::ZeroVector;
}
