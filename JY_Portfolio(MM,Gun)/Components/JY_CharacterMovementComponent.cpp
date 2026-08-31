// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/JY_CharacterMovementComponent.h"
#include "Actors/JY_Character.h"
#include "Components/JY_ProneComponent.h"
#include "Components/JY_HangComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"

UJY_CharacterMovementComponent::UJY_CharacterMovementComponent()
{
	bWantsToProne = false;
	bWantsToHang = false;
}

void UJY_CharacterMovementComponent::BeginPlay() //override
{
	Super::BeginPlay();

	JYCharacter = Cast<AJY_Character>(GetCharacterOwner());
	if (JYCharacter != nullptr)
	{
		JYHangComponent = JYCharacter->GetHangComponent();
	}
}

void UJY_CharacterMovementComponent::Hang(bool bClientSimulation)
{
	if (HasValidData() == false || JYCharacter == nullptr)
		return;

	/* 진입 몽타주 루트모션 속도가 Hang 좌우 이동에 남지 않게 초기화 */
	Velocity = FVector::ZeroVector;
	Acceleration = FVector::ZeroVector;
	UpdateComponentVelocity();
	SetMovementMode(MOVE_Custom, static_cast<uint8>(EJY_CustomMovementMode::Hang));

	/* 시뮬 프록시는 복제된 bIsHanging을 사용 */
	if (bClientSimulation == false)
	{
		JYCharacter->SetIsHanging(true);
	}
}

void UJY_CharacterMovementComponent::UnHang(bool bClientSimulation)
{
	if (HasValidData() == false || JYCharacter == nullptr)
		return;

	/* Hang 좌우 이동 속도/가속도가 다음 이동 상태로 안 넘어가게 초기화 */
	Velocity = FVector::ZeroVector;
	Acceleration = FVector::ZeroVector;
	UpdateComponentVelocity();
	bForceNextFloorCheck = true;

	/* SetIsHanging(false)가 bIsHanging/bFullyHanging을 함께 초기화 */
	if (bClientSimulation == false)
	{
		JYCharacter->SetIsHanging(false);
	}
}

void UJY_CharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations) //override
{
	if (HasAnimRootMotion() == true)
	{
		PhysFlying(DeltaTime, Iterations);
		return;
	}

	switch (static_cast<EJY_CustomMovementMode>(CustomMovementMode))
	{
	case EJY_CustomMovementMode::Hang:
		PhysCustom_Hang(DeltaTime, Iterations);
		return;
	default:
		break;
	}

	Super::PhysCustom(DeltaTime, Iterations);
}

void UJY_CharacterMovementComponent::PhysCustom_Hang(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME || UpdatedComponent == nullptr)
		return;

	/* 시뮬 프록시는 서버 복제 이동을 따라가므로 로컬 트레이스 이동 재계산 안 함 */
	if (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)
		return;

	/* 이어지는 렛지가 없거나 좌우 입력이 없으면 정지 */
	FVector MoveDirectionLedgeNormal;
	if (JYHangComponent == nullptr || JYHangComponent->CanMoveHang(OUT MoveDirectionLedgeNormal) == false)
	{
		Velocity = FVector::ZeroVector;
		return;
	}

	/* 이동할 쪽 렛지 노멀 반대를 정면으로 삼아 벽을 바라보게 함 */
	const FRotator TargetRotation = JYHangComponent->GetHangRotationFromLedgeNormal(MoveDirectionLedgeNormal);

	/* 입력을 접선으로 제약, 엔진 속도계산, 스윕, 실제이동으로 속도 보정 (PhysWalking 참고)*/
	const FVector Acceleration2D(Acceleration.X, Acceleration.Y, 0.f);
	const FVector LedgeTangent = FVector::CrossProduct(FVector::UpVector, MoveDirectionLedgeNormal).GetSafeNormal();

	/* 입력에서 렛지 접선 성분만 남김(앞뒤 제거) */
	Acceleration = LedgeTangent * FVector::DotProduct(Acceleration2D, LedgeTangent);

	CalcVelocity(DeltaTime, JYHangComponent->GetHangFriction(), false, JYHangComponent->GetHangBrakingDeceleration());

	/* 계산된 속도로 스윕 이동+벽 바라보는 각도, 충돌로 덜 갔으면 실제 이동량으로 속도 보정(PhysWalking 참고) */
	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Delta = Velocity * DeltaTime;
	FHitResult MoveHit;
	SafeMoveUpdatedComponent(Delta, TargetRotation.Quaternion(), true, OUT MoveHit);
	Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / DeltaTime;
}

bool UJY_CharacterMovementComponent::CanProneInCurrentState() const
{
	return (IsFalling() || IsMovingOnGround())
		&& UpdatedComponent != nullptr
		&& UpdatedComponent->IsSimulatingPhysics() == false;
}

bool UJY_CharacterMovementComponent::IsProning() const
{
	return JYCharacter != nullptr && JYCharacter->IsProne();
}

bool UJY_CharacterMovementComponent::CanCrouchInCurrentState() const //override
{
	// [Server / OwnClient]
	if (IsProning() == true)
	{
		return false;
	}

	return Super::CanCrouchInCurrentState();
}

void UJY_CharacterMovementComponent::Prone(bool bClientSimulation)
{
	if (HasValidData() == false || JYCharacter == nullptr)
	{
		return;
	}

	if (bClientSimulation == false && CanProneInCurrentState() == false)
	{
		return;
	}

	if (CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() == PronedHalfHeight)
	{
		if (bClientSimulation == false)
		{
			JYCharacter->SetIsProne(true);
		}
		JYCharacter->OnStartProne(0.f, 0.f);
		return;
	}

	// 시뮬프록시는 축소 전 CDO 기본 크기로 복원
	if (bClientSimulation && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)
	{
		ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
		CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(
			DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius(),
			DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());

		// 복제 위치 반올림으로 프록시가 물체를 파고드는 것 방지, 축소분(0.01cm)은 다음 틱 AdjustProxyCapsuleSize()가 재적용.
		//   CharacterMovementComponent.h NetProxyShrinkRadius 828 참고.
		bShrinkProxyCapsule = true;
	}

	// 캡슐 축소, half-height는 radius 밑으로 못 내려감(Max3 클램프)
	const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float OldUnscaledRadius = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	const float ClampedPronedHalfHeight = FMath::Max3(0.f, OldUnscaledRadius, PronedHalfHeight);
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(OldUnscaledRadius, ClampedPronedHalfHeight);
	float HalfHeightAdjust = (OldUnscaledHalfHeight - ClampedPronedHalfHeight);
	float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	if (bClientSimulation == false)
	{
		// 캡슐 바닥(발밑)을 같은 자리에 유지하도록 캡슐 중심을 아래로 내림
		if (bCrouchMaintainsBaseLocation)
		{
			UpdatedComponent->MoveComponent(ScaledHalfHeightAdjust * GetGravityDirection(), UpdatedComponent->GetComponentQuat(), true, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
		}
		JYCharacter->SetIsProne(true);

		/* Crouch 상태에서 바로 Prone으로 들어오면 엔진 크라우치 의도/상태(bWantsToCrouch/bIsCrouched)가 안 지워진 채 남음.
		 * 이후 UpdateCharacterStateBeforeMovement()가 Super 호출 시 "크라우치 중인데 CanCrouchInCurrentState()==false"
		 * (Prone 중엔 위에서 false)로 판단해 엔진 UnCrouch()가 같은 프레임에 바로 발동, 캡슐이 서 있는 크기로 되돌아감.
		 * 여기서 미리 크라우치 의도/상태를 꺼서 그 오발동을 막는다 */
		bWantsToCrouch = false;
		CharacterOwner->SetIsCrouched(false);
	}

	bForceNextFloorCheck = true;
	ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
	HalfHeightAdjust = (DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - ClampedPronedHalfHeight);
	ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
	AdjustProxyCapsuleSize();

	//  메시 Z 보정 + 프론콜리전 활성 + 몽타주 재생
	JYCharacter->OnStartProne(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void UJY_CharacterMovementComponent::UnProne(bool bClientSimulation)
{
	if (HasValidData() == false || JYCharacter == nullptr)
	{
		return;
	}

	ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();

	if (CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() == DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight())
	{
		if (bClientSimulation == false)
		{
			JYCharacter->SetIsProne(false);
		}
		JYCharacter->OnEndProne(0.f, 0.f);
		return;
	}

	const float CurrentPronedHalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float HalfHeightAdjust = DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - OldUnscaledHalfHeight;
	const float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
	const FVector PawnLocation = UpdatedComponent->GetComponentLocation();

	if (bClientSimulation == false)
	{
		// 일어설 자리에 서기 캡슐이 들어가는지 검사, 천장에 끼면 못 일어남
		const UWorld* MyWorld = GetWorld();
		const float SweepInflation = UE_KINDA_SMALL_NUMBER * 10.f;
		FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(ProneTrace), false, CharacterOwner);
		FCollisionResponseParams ResponseParam;
		InitCollisionParams(CapsuleParams, ResponseParam);

		// 커진(서기) 캡슐 모양. 음수로 넘겨서 실제로는 확장.
		//   SweepInflation은 딱 맞는 크기로 재면 천장 높이가 캡슐과 정확히 같을 때 부동소수점 오차 때문에
		//   겹치는지 안겹치는지 다를 수 있어서, 더 크게 재서 안전하게 일어나지 못하게 판정하려고 더하는 여유분
		const FCollisionShape StandingCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, -SweepInflation - ScaledHalfHeightAdjust);
		const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();

		// 발밑을 유지한 채 커진 캡슐이 들어갈 위치
		FVector StandingLocation = PawnLocation + (StandingCapsuleShape.GetCapsuleHalfHeight() - CurrentPronedHalfHeight) * -GetGravityDirection();
		bool bEncroached = MyWorld->OverlapBlockingTestByChannel(StandingLocation, GetWorldToGravityTransform(), CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);

		// 천장이 살짝 걸리면 바닥 쪽으로 조금 내려 재시도
		if (bEncroached)
		{
			if (IsMovingOnGround())
			{
				const float MinFloorDist = UE_KINDA_SMALL_NUMBER * 10.f;
				if (CurrentFloor.bBlockingHit && CurrentFloor.FloorDist > MinFloorDist)
				{
					StandingLocation -= (CurrentFloor.FloorDist - MinFloorDist) * -GetGravityDirection();
					bEncroached = MyWorld->OverlapBlockingTestByChannel(StandingLocation, GetWorldToGravityTransform(), CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
				}
			}
		}

		// 그래도 끼면 일어나기 취소(엎드린 채 유지)
		if (bEncroached)
		{
			return;
		}

		// 위치 확정
		UpdatedComponent->MoveComponent(StandingLocation - PawnLocation, UpdatedComponent->GetComponentQuat(), false, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
		bForceNextFloorCheck = true;

		JYCharacter->SetIsProne(false);
	}
	else
	{
		bShrinkProxyCapsule = true;
	}

	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(
		DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius(),
		DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), true);

	AdjustProxyCapsuleSize();

	//  메시 Z 복원 + 프론콜리전 해제 + 몽타주 재생
	JYCharacter->OnEndProne(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void UJY_CharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds) //override
{
	// [Server / OwnClient] 의도(bWantsToProne)로 상태 전이.
	//   시뮬프록시는 계산 안 하고 복제된 bIsProne을 OnRep_IsProne에서 받음
	if (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
	{
		const bool bIsProning = IsProning();
		if (bIsProning && (bWantsToProne == false || CanProneInCurrentState() == false))
		{
			UnProne(false);
		}
		else if (bIsProning == false && bWantsToProne && CanProneInCurrentState())
		{
			Prone(false);
		}

		// prone 전이 직후 제자리 턴 감지
		if (IsProning() == true)
		{
			UpdateProneTurnState();
		}

		if (JYCharacter != nullptr)
		{
			const bool bIsHanging = JYCharacter->GetIsHanging();

			if (bIsHanging && bWantsToHang == false)
			{
				UnHang(false);
			}
			else if (bIsHanging == false && bWantsToHang != 0 && MovementMode == MOVE_Custom)
			{
				Hang(false);
			}
		}

	}

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

float UJY_CharacterMovementComponent::GetMaxSpeed() const //override
{
	switch (MovementMode)
	{
	case MOVE_Walking:
	case MOVE_NavWalking:
		// 우선순위 Prone > Crouch > Walk
		if (IsProning())
		{
			return MaxProneSpeed;
		}
		return IsCrouching() ? MaxWalkSpeedCrouched : MaxWalkSpeed;
	case MOVE_Falling:
		return MaxWalkSpeed;
	case MOVE_Swimming:
		return MaxSwimSpeed;
	case MOVE_Flying:
		return MaxFlySpeed;
	case MOVE_Custom:
		if (JYCharacter != nullptr && JYCharacter->GetIsHanging() && JYHangComponent != nullptr)
		{
			return JYHangComponent->GetMaxHangSpeed();
		}
		return MaxCustomMovementSpeed;
	case MOVE_None:
	default:
		return 0.f;
	}
}

float UJY_CharacterMovementComponent::GetMaxAcceleration() const //override
{
	if (MovementMode == MOVE_Custom && JYCharacter != nullptr && JYCharacter->GetIsHanging() && JYHangComponent != nullptr)
	{
		return JYHangComponent->GetMaxHangAcceleration();
	}
	return Super::GetMaxAcceleration();
}

// SavedMove + 압축플래그, 클라 예측 move에 prone/hang 의도를 실어 서버로 전달
// --- FJY_SavedMove_Character ---

void FJY_SavedMove_Character::Clear() //override
{
	Super::Clear();
	bWantsToProne = false;
	bWantsToHang = false;
}

void FJY_SavedMove_Character::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) //override
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	const UJY_CharacterMovementComponent* JYMovementComp = Cast<UJY_CharacterMovementComponent>(C->GetCharacterMovement());
	if (JYMovementComp != nullptr)
	{
		bWantsToProne = JYMovementComp->bWantsToProne;
		bWantsToHang = JYMovementComp->bWantsToHang;
	}
}

uint8 FJY_SavedMove_Character::GetCompressedFlags() const //override
{
	// Super로 점프/크라우치 비트 유지 후 커스텀 비트 추가
	uint8 Result = Super::GetCompressedFlags();
	if (bWantsToProne)
	{
		Result |= FLAG_Custom_0;
	}
	if (bWantsToHang)
	{
		Result |= FLAG_Custom_1;
	}
	return Result;
}

// --- FJY_NetworkPredictionData_Client_Character ---

FJY_NetworkPredictionData_Client_Character::FJY_NetworkPredictionData_Client_Character(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr FJY_NetworkPredictionData_Client_Character::AllocateNewMove() //override
{
	return FSavedMovePtr(new FJY_SavedMove_Character());
}

FNetworkPredictionData_Client* UJY_CharacterMovementComponent::GetPredictionData_Client() const //override
{
	if (ClientPredictionData == nullptr)
	{
		UJY_CharacterMovementComponent* MutableThis = const_cast<UJY_CharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FJY_NetworkPredictionData_Client_Character(*this);
	}
	return ClientPredictionData;
}

void UJY_CharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags) //override
{
	Super::UpdateFromCompressedFlags(Flags);

	bWantsToProne = ((Flags & FSavedMove_Character::FLAG_Custom_0) != 0);
	bWantsToHang = ((Flags & FSavedMove_Character::FLAG_Custom_1) != 0);
}


// 턴인플레이스, 큰 각도 방향전환 시 제자리 회전. 회전은 루트모션 몽타주가 담당, 여기선 "언제 도는가"만 판정 
void UJY_CharacterMovementComponent::UpdateProneTurnState()
{
	if (JYCharacter == nullptr)
	{
		return;
	}

	// 턴 재생은 ProneComponent 소유, CMC는 감지만 함
	UJY_ProneComponent* ProneComp = JYCharacter->GetProneComponent();
	if (ProneComp == nullptr)
	{
		return;
	}

	// 턴 몽타주 재생 중이면 새 턴 안 함
	if (ProneComp->IsPlayingProneTurn())
	{
		return;
	}

	// 시작 조건: prone 로코모션 중(지상)
	if (IsProning() == false || IsMovingOnGround() == false)
	{
		return;
	}

	// 입력 방향을 목표로, 지상 회전은 Yaw만 필요해 XY 평면으로
	const FVector Accel = GetCurrentAcceleration();
	const FVector InputDir2D(Accel.X, Accel.Y, 0.f);
	if (InputDir2D.IsNearlyZero())
	{
		return;
	}

	// 입력 방향과 현재 Facing의 각도차, 임계 이하면 턴 안 함
	const float InputYaw = InputDir2D.Rotation().Yaw;
	const float ActorYaw = UpdatedComponent->GetComponentRotation().Yaw;
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(ActorYaw, InputYaw);
	if (FMath::Abs(DeltaYaw) <= ProneTurnStartAngle)
	{
		return;
	}

	// 앞 -> 앞 전환 예외: 이동방향과 입력이 모두 카메라 전방 콘 안이면 턴 안 함.
	//   정지 중엔 Velocity 방향이 무의미해서 ProneTurnMinSpeed 이상일 때만 판정
	const FVector Velocity2D(Velocity.X, Velocity.Y, 0.f);
	if (Velocity2D.SizeSquared() > FMath::Square(ProneTurnMinSpeed))
	{
		const float CameraYaw = JYCharacter->GetControlRotation().Yaw;
		const float VelocityYaw = Velocity2D.Rotation().Yaw;
		const float CameraToVelocity = FMath::Abs(FMath::FindDeltaAngleDegrees(CameraYaw, VelocityYaw));
		const float CameraToInput = FMath::Abs(FMath::FindDeltaAngleDegrees(CameraYaw, InputYaw));
		if (CameraToVelocity < ProneTurnFrontConeAngle && CameraToInput < ProneTurnFrontConeAngle)
		{
			return;
		}
	}

	// 턴 시작, 목표는 입력 방향. +Yaw면 오른쪽
	const bool bTurnRight = (DeltaYaw > 0.f);
	ProneComp->StartProneTurn(InputYaw, bTurnRight);
}

// 벽 충돌, 앞으로 뻗은 몸이 벽 뚫는 것 방지. 몸 캡슐(ProneBodyCapsule)이 센서, 처리는 velocity 레이어
void UJY_CharacterMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) //override
{
	Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
	ConstrainProneVelocityForWalls(DeltaTime);
}

void UJY_CharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) //override
{
	// 이동 끝난 뒤 프론이면 벽에 걸친 몸을 밀어냄.
	//   시뮬프록시는 서버가 밀어낸 결과를 복제로 받으므로 제외
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	if (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy && JYCharacter != nullptr && JYCharacter->IsProne() == true)
	{
		DepenetrateProneBodyFromWalls();
	}
}

void UJY_CharacterMovementComponent::ConstrainProneVelocityForWalls(float DeltaSeconds)
{
	// [Server / OwnClient] 벽으로 파고드는 속도 성분 제거.
	//   미리 없애야 루트가 벽을 안 뚫고 사후 되밀림도 안 생김
	if (JYCharacter == nullptr || JYCharacter->IsProne() == false)
	{
		return;
	}

	UCapsuleComponent* ProneCapsule = JYCharacter->GetProneBodyCapsule();
	UWorld* World = GetWorld();
	if (ProneCapsule == nullptr || World == nullptr)
	{
		return;
	}

	// 수평 이동만 처리, 상하는 엔진 바닥처리 소관
	FVector HorizVel = Velocity;
	HorizVel.Z = 0.f;
	const FVector MoveDir = HorizVel.GetSafeNormal();
	if (MoveDir.IsNearlyZero())
	{
		return;
	}

	// 이번 프레임 이동거리 + 여유 두께, 닿기 직전에 속도를 꺾음
	const float LookAhead = HorizVel.Size() * DeltaSeconds + ProneWallSweepSkin;
	const FVector Start = ProneCapsule->GetComponentLocation();
	const FVector End = Start + MoveDir * LookAhead;
	const FCollisionShape ProneShape = FCollisionShape::MakeCapsule(ProneCapsule->GetScaledCapsuleRadius(), ProneCapsule->GetScaledCapsuleHalfHeight());
	const FQuat ProneQuat = ProneCapsule->GetComponentQuat();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProneWallVel), false, CharacterOwner);

	FHitResult Hit;
	const bool bBlocked = World->SweepSingleByChannel(Hit, Start, End, ProneQuat, ECC_WorldStatic, ProneShape, Params);
	if (bBlocked == false)
	{
		CachedProneWallNormal = FVector::ZeroVector;
		return;
	}

	// 걸을 수 있는 면이면 벽 아님, 손대지 않음
	if (Hit.ImpactNormal.Z >= GetWalkableFloorZ())
	{
		CachedProneWallNormal = FVector::ZeroVector;
		return;
	}

	// 시작관통이면 Hit.Normal=0, 직전 유효 노멀 재사용해 방향 떨림 방지
	FVector WallNormal = Hit.Normal;
	if (WallNormal.IsNearlyZero() == true)
	{
		WallNormal = CachedProneWallNormal.IsNearlyZero() == false ? CachedProneWallNormal : Hit.ImpactNormal;
	}

	// 수평 성분만 남김, 바닥이면 0이 되어 무시
	WallNormal.Z = 0.f;
	WallNormal = WallNormal.GetSafeNormal();
	if (WallNormal.IsNearlyZero() == true)
	{
		return;
	}
	CachedProneWallNormal = WallNormal;

	// 벽 방향 성분 제거, 벽 따라 슬라이드
	FVector AdjustedDelta = FVector::VectorPlaneProject(HorizVel * DeltaSeconds, WallNormal);
	if (AdjustedDelta.IsNearlyZero() == true)
	{
		Velocity.X = 0.f;
		Velocity.Y = 0.f;
		return;
	}

	// 첫 벽 따라 이동 시 만나는 두 번째 벽 재검사
	const FVector SecondStart = Hit.Location;
	const FVector SecondMoveDir = AdjustedDelta.GetSafeNormal();
	const FVector SecondEnd = SecondStart + AdjustedDelta + SecondMoveDir * ProneWallSweepSkin;

	FHitResult SecondHit;
	const bool bSecondBlocked = World->SweepSingleByChannel(
		SecondHit, SecondStart, SecondEnd, ProneQuat, ECC_WorldStatic, ProneShape, Params);
	if (bSecondBlocked == true)
	{
		FVector SecondWallNormal = SecondHit.Normal;
		SecondWallNormal.Z = 0.f;
		SecondWallNormal = SecondWallNormal.GetSafeNormal();

		const bool bDifferentWall = SecondWallNormal.IsNearlyZero() == false
			&& FVector::DotProduct(WallNormal, SecondWallNormal) < 1.f - UE_KINDA_SMALL_NUMBER;
		if (bDifferentWall == true)
		{
			SecondHit.Normal = SecondWallNormal;
			SecondHit.ImpactNormal = SecondWallNormal;
			TwoWallAdjust(AdjustedDelta, SecondHit, WallNormal);
			AdjustedDelta.Z = 0.f;
		}
	}

	// 보정된 이동량을 엔진 이동 전에 다시 속도로 환산
	if (DeltaSeconds > UE_KINDA_SMALL_NUMBER)
	{
		Velocity.X = AdjustedDelta.X / DeltaSeconds;
		Velocity.Y = AdjustedDelta.Y / DeltaSeconds;
	}
}

void UJY_CharacterMovementComponent::DepenetrateProneBodyFromWalls()
{
	// [Server / OwnClient] 겹친 몸 캡슐을 MTD(가장 짧게 떼어놓는 방향+거리)만큼 벽 밖으로 밀어냄.
	//   가장 깊은 겹침 하나씩 처리, 모서리 MTD를 한꺼번에 합치면 방향이 떨림
	UCapsuleComponent* ProneCapsule = JYCharacter->GetProneBodyCapsule();
	UWorld* World = GetWorld();
	if (ProneCapsule == nullptr || World == nullptr || UpdatedComponent == nullptr)
	{
		return;
	}

	const FCollisionShape ProneShape = FCollisionShape::MakeCapsule(ProneCapsule->GetScaledCapsuleRadius(), ProneCapsule->GetScaledCapsuleHalfHeight());
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProneBodyMTD), false, CharacterOwner);

	constexpr int32 MaxDepenetrationIterations = 2;
	for (int32 Iteration = 0; Iteration < MaxDepenetrationIterations; ++Iteration)
	{
		const FVector Loc = ProneCapsule->GetComponentLocation();
		const FQuat Quat = ProneCapsule->GetComponentQuat();

		TArray<FOverlapResult> Overlaps;
		World->OverlapMultiByChannel(Overlaps, Loc, Quat, ECC_WorldStatic, ProneShape, Params);
		if (Overlaps.Num() == 0)
		{
			break;
		}

		FVector PushOut = FVector::ZeroVector;
		float DeepestPenetration = 0.f;
		for (const FOverlapResult& Overlap : Overlaps)
		{
			UPrimitiveComponent* Comp = Overlap.GetComponent();
			if (Comp == nullptr)
			{
				continue;
			}

			// MTD.Direction = 밀어낼 방향, MTD.Distance = 겹친 깊이
			FMTDResult MTD;
			if (Comp->ComputePenetration(MTD, ProneShape, Loc, Quat) == false)
			{
				continue;
			}

			// 미는 방향 Z가 GetWalkableFloorZ 이상이면 지면, 벽 아님
			const bool bIsWall = MTD.Direction.Z < GetWalkableFloorZ();
			if (bIsWall == true && MTD.Distance > DeepestPenetration)
			{
				PushOut = MTD.Direction * MTD.Distance;
				DeepestPenetration = MTD.Distance;
			}
		}

		PushOut.Z = 0.f;
		if (PushOut.IsNearlyZero() == true)
		{
			break;
		}

		FHitResult MoveHit;
		SafeMoveUpdatedComponent(PushOut, UpdatedComponent->GetComponentQuat(), true, MoveHit);

		const FVector PushDir = PushOut.GetSafeNormal();
		const float IntoWall = FVector::DotProduct(Velocity, PushDir);
		if (IntoWall < 0.f)
		{
			Velocity -= IntoWall * PushDir;
		}
	}
}
