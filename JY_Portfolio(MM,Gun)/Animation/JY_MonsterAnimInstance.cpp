// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/JY_MonsterAnimInstance.h"
#include "Actors/JY_Monster.h"
#include "Animation/AnimSequence.h"
#include "KismetAnimationLibrary.h"
#include "GameFramework/Controller.h"
#include "Components/JY_EquipmentComponent.h"

void UJY_MonsterAnimInstance::NativeInitializeAnimation() //override
{
	Super::NativeInitializeAnimation();

	/* [Local Cosmetic - All Clients] */
	JYMonster = Cast<AJY_Monster>(TryGetPawnOwner());
}

void UJY_MonsterAnimInstance::NativeBeginPlay() //override
{
	Super::NativeBeginPlay();

	/* Init 시점엔 폰이 아직 없을 수 있어 재확인 */
	if (JYMonster == nullptr)
	{
		JYMonster = Cast<AJY_Monster>(TryGetPawnOwner());
	}
}

void UJY_MonsterAnimInstance::NativeUpdateAnimation(float DeltaSeconds) //override
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	/* [Local Cosmetic - All Clients] */
	/* 수평 속력만 (Z는 낙하/렉돌과 무관) */
	GroundSpeed = JYMonster != nullptr ? JYMonster->GetVelocity().Size2D() : 0.f;
	bIsMoving = GroundSpeed > KINDA_SMALL_NUMBER;

	MoveDirection = JYMonster != nullptr ? UKismetAnimationLibrary::CalculateDirection(JYMonster->GetVelocity(), JYMonster->GetActorRotation()) : 0.f;

	const AController* MonsterController = JYMonster != nullptr ? JYMonster->GetController() : nullptr;
	if (MonsterController != nullptr)
	{
		/* ±90도를 ±1로 매핑 */
		const FRotator ControlRotation = MonsterController->GetControlRotation();
		const float YawDelta = FRotator::NormalizeAxis(ControlRotation.Yaw - JYMonster->GetActorRotation().Yaw);
		const float PitchDelta = FRotator::NormalizeAxis(ControlRotation.Pitch);
		AimYaw = FMath::Clamp(YawDelta / 90.f, -1.f, 1.f);
		AimPitch = FMath::Clamp(PitchDelta / 90.f, -1.f, 1.f);
	}
	else
	{
		AimYaw = 0.f;
		AimPitch = 0.f;
	}

	/* UJY_AnimInstance::UpdateWeaponState와 같은 조회, 게이팅만 단순화 */
	UJY_EquipmentComponent* EquipmentComp = JYMonster != nullptr ? JYMonster->GetEquipmentComponent() : nullptr;
	FTransform LeftHandTarget;
	bHasLeftHandIKTarget = EquipmentComp != nullptr ? EquipmentComp->GetLeftHandIKTargetWorld(LeftHandTarget) : false;
	LeftHandIKTargetLocation = bHasLeftHandIKTarget ? LeftHandTarget.GetLocation() : FVector::ZeroVector;

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

void UJY_MonsterAnimInstance::RequestHitReact(EJY_HitReactDirection Direction)
{
	/* [Server 포함 전 클라] AJY_Monster::RequestHitReact 답습 */
	if (const TObjectPtr<UAnimSequence>* Found = HitReactSequences.Find(Direction))
	{
		HitReactSequence = *Found;
	}

	++HitReactPlayCount;
	bHitReacting = true;
}
