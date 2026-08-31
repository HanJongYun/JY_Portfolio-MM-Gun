// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/JY_ProneComponent.h"
#include "Actors/JY_Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "MotionWarpingComponent.h"
#include "Components/CapsuleComponent.h"

UJY_ProneComponent::UJY_ProneComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UJY_ProneComponent::BeginPlay() //override
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AJY_Character>(GetOwner());
	OwnerMesh = (OwnerCharacter != nullptr) ? OwnerCharacter->GetMesh() : nullptr;

	// 메시(애님 업데이트)가 이 컴포넌트 뒤에 틱하도록 순서 보장
	if (OwnerMesh != nullptr)
	{
		OwnerMesh->AddTickPrerequisiteComponent(this);
	}
}

void UJY_ProneComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) //override
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (OwnerCharacter == nullptr)
		return;

	if (ShouldSkipProneTrace(DeltaTime) == false)
	{
		UpdateProneTraceTargets();
	}

	LerpGroundAlignment(DeltaTime);
}

void UJY_ProneComponent::StartProneTurn(float TargetYaw, bool bTurnRight)
{
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	// 방향에 맞는 몽타주 선택
	UAnimMontage* TurnMontage = bTurnRight ? ProneTurnRightMontage : ProneTurnLeftMontage;
	UMotionWarpingComponent* MotionWarping = OwnerCharacter->GetMotionWarpingComponent();
	if (TurnMontage == nullptr || MotionWarping == nullptr)
	{
		return;
	}

	// 목표 Yaw로 돌면 몸이 벽 뚫는지 검사, 뚫으면 턴 취소
	if (WouldProneBodyClipWall(TargetYaw))
	{
		return;
	}

	// 위치는 제자리 유지, 회전은 목표 Yaw만
	const FVector CurrentLocation = OwnerCharacter->GetActorLocation();
	const FRotator TargetRotation(0.f, TargetYaw, 0.f);
	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(FName("ProneTurn"), CurrentLocation, TargetRotation);

	OwnerCharacter->PlayAnimMontage(TurnMontage);
}

bool UJY_ProneComponent::IsPlayingProneTurn() const
{
	if (OwnerCharacter == nullptr)
	{
		return false;
	}

	const USkeletalMeshComponent* MeshComp = OwnerCharacter->GetMesh();
	const UAnimInstance* AnimInstance = (MeshComp != nullptr) ? MeshComp->GetAnimInstance() : nullptr;
	if (AnimInstance == nullptr)
	{
		return false;
	}

	return (ProneTurnLeftMontage != nullptr && AnimInstance->Montage_IsPlaying(ProneTurnLeftMontage))
		|| (ProneTurnRightMontage != nullptr && AnimInstance->Montage_IsPlaying(ProneTurnRightMontage));
}

bool UJY_ProneComponent::WouldProneBodyClipWall(float TargetYaw) const
{
	if (OwnerCharacter == nullptr)
	{
		return false;
	}

	UCapsuleComponent* ProneBodyCapsule = OwnerCharacter->GetProneBodyCapsule();
	if (ProneBodyCapsule == nullptr)
	{
		return false;
	}

	// 제자리 턴은 액터 위치 축 회전. 몸 캡슐을 턴 완료 후 위치/회전으로 옮겨 오버랩 검사
	const FVector Pivot = OwnerCharacter->GetActorLocation();
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(OwnerCharacter->GetActorRotation().Yaw, TargetYaw);
	const FQuat TurnRot(FVector::UpVector, FMath::DegreesToRadians(DeltaYaw));

	const FVector TestLoc = Pivot + TurnRot.RotateVector(ProneBodyCapsule->GetComponentLocation() - Pivot);
	const FQuat TestRot = TurnRot * ProneBodyCapsule->GetComponentQuat();
	const FCollisionShape BodyShape = FCollisionShape::MakeCapsule(ProneBodyCapsule->GetScaledCapsuleRadius(), ProneBodyCapsule->GetScaledCapsuleHalfHeight());

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProneTurnClip), false, OwnerCharacter);
	TArray<FOverlapResult> Overlaps;
	OwnerCharacter->GetWorld()->OverlapMultiByChannel(Overlaps, TestLoc, TestRot, ECC_WorldStatic, BodyShape, Params);

	bool bWallBlocked = false;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		UPrimitiveComponent* Comp = Overlap.GetComponent();
		if (Comp == nullptr)
		{
			continue;
		}

		// MTD.Direction은 캡슐을 겹침에서 빼내는 방향(면 노멀 쪽). 위쪽이면 바닥/완경사, 수평이면 벽
		FMTDResult MTD;
		if (Comp->ComputePenetration(MTD, BodyShape, TestLoc, TestRot) == false)
		{
			continue;
		}

		const bool bIsWall = FMath::Abs(MTD.Direction.Z) < ProneTurnWallNormalZThreshold;
		if (bIsWall)
		{
			bWallBlocked = true;
		}
	}

	// 수직 벽 겹침만 턴 차단, 바닥은 무시
	return bWallBlocked;
}

void UJY_ProneComponent::SetProneBodyCollisionActive(bool bActive)
{
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	UCapsuleComponent* ProneBodyCapsule = OwnerCharacter->GetProneBodyCapsule();
	if (ProneBodyCapsule == nullptr)
	{
		return;
	}

	// 이동 차단은 루트 캡슐 담당, 여긴 Query 전용
	ProneBodyCapsule->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void UJY_ProneComponent::OnProneStarted()
{
	if (OwnerCharacter == nullptr)
		return;

	if (OwnerCharacter->GetNetMode() == NM_DedicatedServer)
		return;

	PrevYaw = OwnerCharacter->GetActorRotation().Yaw;
	SetComponentTickEnabled(true);
}

void UJY_ProneComponent::OnProneEnded()
{
	TargetProneGroundPitch = 0.f;
	TargetProneGroundRoll = 0.f;
	TargetProneGroundHeightOffset = 0.f;

	ProneInput.GroundPitch = 0.f;
	ProneInput.GroundRoll = 0.f;
	ProneInput.GroundHeightOffset = 0.f;

	SetComponentTickEnabled(false);
}

/* 프론 지면 정렬 목표 갱신 */
void UJY_ProneComponent::UpdateProneTraceTargets()
{
	if (OwnerCharacter->IsFullyProne() == false)
	{
		TargetProneGroundPitch = 0.f;
		TargetProneGroundRoll = 0.f;
		TargetProneGroundHeightOffset = 0.f;
		return;
	}

	UpdateProneGroundAlignment();
}

bool UJY_ProneComponent::TraceProneGroundPoint(const FVector& TraceOrigin, FHitResult& OutHit) const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
		return false;

	const FVector TraceStart = TraceOrigin + FVector::UpVector * GroundTraceSettings.TraceUpDistance;
	const FVector TraceEnd = TraceOrigin - FVector::UpVector * GroundTraceSettings.TraceDownDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ProneGroundTrace), false, OwnerCharacter);
	const FCollisionObjectQueryParams ObjectQueryParams(ECC_WorldStatic);

	return World->LineTraceSingleByObjectType(OUT OutHit, TraceStart, TraceEnd, ObjectQueryParams, QueryParams);
}

void UJY_ProneComponent::UpdateProneGroundAlignment()
{
	UWorld* World = GetWorld();
	if (World == nullptr || OwnerCharacter == nullptr)
		return;

	FVector Forward2D = OwnerCharacter->GetActorForwardVector();
	Forward2D.Z = 0.f;
	Forward2D.Normalize();
	if (Forward2D.IsNearlyZero())
		return;

	FVector Right2D = OwnerCharacter->GetActorRightVector();
	Right2D.Z = 0.f;
	Right2D.Normalize();

	const FVector ActorLocation = OwnerCharacter->GetActorLocation();
	const FVector FrontTraceOrigin = ActorLocation + Forward2D * GroundTraceSettings.FrontTraceDistance;
	const FVector RearTraceOrigin = ActorLocation - Forward2D * GroundTraceSettings.RearTraceDistance;
	const FVector RightTraceOrigin = ActorLocation + Right2D * GroundTraceSettings.SideTraceDistance;
	const FVector LeftTraceOrigin = ActorLocation - Right2D * GroundTraceSettings.SideTraceDistance;

	USkeletalMeshComponent* MeshComp = OwnerMesh;
	const bool bHasPelvisBone = MeshComp != nullptr && MeshComp->DoesSocketExist(TEXT("pelvis"));
	const FVector PelvisWorldLocation = bHasPelvisBone ? MeshComp->GetSocketLocation(TEXT("pelvis")) : ActorLocation;

	FHitResult FrontHit;
	FHitResult RearHit;
	FHitResult LeftHit;
	FHitResult RightHit;
	const bool bFrontHit = TraceProneGroundPoint(FrontTraceOrigin, OUT FrontHit);
	const bool bRearHit = TraceProneGroundPoint(RearTraceOrigin, OUT RearHit);
	const bool bLeftHit = TraceProneGroundPoint(LeftTraceOrigin, OUT LeftHit);
	const bool bRightHit = TraceProneGroundPoint(RightTraceOrigin, OUT RightHit);

	if (bFrontHit && bRearHit)
	{
		TargetProneGroundPitch = ComputeProneGroundPitch(FrontHit.ImpactPoint, RearHit.ImpactPoint);
		TargetProneGroundHeightOffset = ComputeProneGroundHeightOffset(FrontHit.ImpactPoint, RearHit.ImpactPoint, PelvisWorldLocation, ActorLocation, Forward2D);
	}
	else
	{
		TargetProneGroundPitch = 0.f;
		TargetProneGroundHeightOffset = 0.f;
	}

	if (bLeftHit && bRightHit)
	{
		TargetProneGroundRoll = ComputeProneGroundRoll(LeftHit.ImpactPoint, RightHit.ImpactPoint);
	}
	else
	{
		TargetProneGroundRoll = 0.f;
	}

}

float UJY_ProneComponent::ComputeProneGroundPitch(const FVector& FrontImpact, const FVector& RearImpact) const
{
	const FVector GroundSlopeDirection = FrontImpact - RearImpact;
	return FMath::Clamp(GroundSlopeDirection.Rotation().Pitch, -GroundTraceSettings.MaxPitch, GroundTraceSettings.MaxPitch);
}

float UJY_ProneComponent::ComputeProneGroundRoll(const FVector& LeftImpact, const FVector& RightImpact) const
{
	const FVector GroundRollDirection = RightImpact - LeftImpact;
	return FMath::Clamp(GroundRollDirection.Rotation().Pitch, -GroundTraceSettings.MaxRoll, GroundTraceSettings.MaxRoll);
}

float UJY_ProneComponent::ComputeProneGroundHeightOffset(
	const FVector& FrontImpact, const FVector& RearImpact, const FVector& PelvisWorldLocation, const FVector& ActorLocation, const FVector& Forward2D) const
{
	const float PelvisAlongAxis = FVector::DotProduct(PelvisWorldLocation - ActorLocation, Forward2D);
	const float TraceSpan = GroundTraceSettings.FrontTraceDistance + GroundTraceSettings.RearTraceDistance;
	const float T = TraceSpan > KINDA_SMALL_NUMBER ? (PelvisAlongAxis + GroundTraceSettings.RearTraceDistance) / TraceSpan : 0.5f;
	const float GroundZUnderPelvis = FMath::Lerp(RearImpact.Z, FrontImpact.Z, T);
	const float DesiredPelvisZ = GroundZUnderPelvis + GroundTraceSettings.PelvisGroundDistance;
	const float HeightOffset = DesiredPelvisZ - PelvisWorldLocation.Z;
	return ProneInput.GroundHeightOffset + HeightOffset;
}

/* 목표값 보간해 최종 기울기/높이 갱신 */
void UJY_ProneComponent::LerpGroundAlignment(float DeltaTime)
{
	const bool bProne = OwnerCharacter != nullptr && OwnerCharacter->IsProne() == true;
	const float TargetPitch = bProne ? TargetProneGroundPitch : 0.f;
	const float TargetRoll = bProne ? TargetProneGroundRoll : 0.f;
	const float TargetHeightOffset = bProne ? TargetProneGroundHeightOffset : 0.f;

	ProneInput.GroundPitch = FMath::FInterpTo(ProneInput.GroundPitch, TargetPitch, DeltaTime, GroundTraceSettings.AngleInterpSpeed);
	ProneInput.GroundRoll = FMath::FInterpTo(ProneInput.GroundRoll, TargetRoll, DeltaTime, GroundTraceSettings.AngleInterpSpeed);
	ProneInput.GroundHeightOffset = FMath::FInterpTo(ProneInput.GroundHeightOffset, TargetHeightOffset, DeltaTime, GroundTraceSettings.HeightInterpSpeed);
}

bool UJY_ProneComponent::ShouldSkipProneTrace(float DeltaTime)
{
	const float CurrentYaw = OwnerCharacter->GetActorRotation().Yaw;
	const float YawRate = DeltaTime > KINDA_SMALL_NUMBER ? FMath::Abs(FMath::FindDeltaAngleDegrees(PrevYaw, CurrentYaw)) / DeltaTime : 0.f;
	PrevYaw = CurrentYaw;

	const bool bMoving = OwnerCharacter->GetVelocity().SizeSquared2D() > FMath::Square(ProneMoveSpeedEps);
	const bool bRotating = YawRate > ProneYawRateEps;
	return bMoving == false && bRotating == false;
}
