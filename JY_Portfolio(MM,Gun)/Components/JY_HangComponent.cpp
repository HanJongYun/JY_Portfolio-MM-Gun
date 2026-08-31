// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/JY_HangComponent.h"
#include "Actors/JY_Character.h"
#include "Components/JY_TraversalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "CollisionQueryParams.h"
#include "JY_CollisionChannels.h"

UJY_HangComponent::UJY_HangComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UJY_HangComponent::BeginPlay() //override
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AJY_Character>(GetOwner());
	if (OwnerCharacter != nullptr)
	{
		TraversalComponent = OwnerCharacter->FindComponentByClass<UJY_TraversalComponent>();
	}
}

bool UJY_HangComponent::CanMoveHang(FVector& OutMoveDirectionLedgeNormal) const
{
	OutMoveDirectionLedgeNormal = FVector::ZeroVector;

	if (OwnerCharacter == nullptr || TraversalComponent == nullptr)
		return false;

	const UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent();
	const UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement();
	if (Capsule == nullptr || MovementComponent == nullptr)
		return false;

	const FVector CurrentAcceleration = MovementComponent->GetCurrentAcceleration();
	const FVector Acceleration2D(CurrentAcceleration.X, CurrentAcceleration.Y, 0.f);
	if (Acceleration2D.IsNearlyZero())
		return false;

	const FVector ActorLocation = OwnerCharacter->GetActorLocation();
	const FVector ForwardDirection = OwnerCharacter->GetActorForwardVector();

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(JYHangTrace), false);
	TraceParams.AddIgnoredActor(OwnerCharacter);

	/* 정면 벽을 확인 */
	const FVector FrontTraceEnd = ActorLocation + ForwardDirection * 300.f;
	FHitResult FrontHit;
	const bool bFrontHit = GetWorld()->LineTraceSingleByChannel(OUT FrontHit, ActorLocation, FrontTraceEnd,
		JY_TraceChannel::Traversable, TraceParams);
	if (bFrontHit == false || FrontHit.bBlockingHit == false)
		return false;

	/* 옆쪽 이동 가능한지 */
	const FVector LedgeSideAxis = FVector::CrossProduct(FVector::UpVector, FrontHit.ImpactNormal).GetSafeNormal();
	const float LateralDot = FVector::DotProduct(Acceleration2D.GetSafeNormal(), LedgeSideAxis);
	if (FMath::Abs(LateralDot) < 0.1f)
		return false;

	const FVector SideDirection = LedgeSideAxis * FMath::Sign(LateralDot);
	const FVector SideTraceStart = ActorLocation + SideDirection * Capsule->GetScaledCapsuleRadius();
	const FVector SideTraceEnd = SideTraceStart + ForwardDirection * 300.f;

	FHitResult SideHit;
	const bool bSideHit = GetWorld()->LineTraceSingleByChannel(OUT SideHit, SideTraceStart, SideTraceEnd,
		JY_TraceChannel::Traversable, TraceParams);
	if (bSideHit == false || SideHit.bBlockingHit == false)
		return false;

	FJY_TraversalCheckResult LedgeResult;
	if (TraversalComponent->GetLedgeTransforms_BP(SideHit.GetActor(), SideHit.ImpactPoint, SideTraceStart, OUT LedgeResult) == false)
		return false;

	OutMoveDirectionLedgeNormal = LedgeResult.FrontLedgeNormal;
	return true;
}

FRotator UJY_HangComponent::GetHangRotationFromLedgeNormal(const FVector& LedgeNormal) const
{
	return UKismetMathLibrary::MakeRotFromX(-LedgeNormal);
}
