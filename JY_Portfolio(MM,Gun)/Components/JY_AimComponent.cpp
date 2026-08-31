#include "Components/JY_AimComponent.h"

#include "Actors/JY_Character.h"
#include "Camera/CameraComponent.h"
#include "Components/JY_EquipmentComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Framework/JY_PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

UJY_AimComponent::UJY_AimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);
}

void UJY_AimComponent::BeginPlay() //override
{
	Super::BeginPlay();

	AJY_Character* JYCharacter = Cast<AJY_Character>(GetOwner());
	if (JYCharacter == nullptr)
	{
		return;
	}

	if (USpringArmComponent* CameraBoom = JYCharacter->GetCameraBoom())
	{
		DefaultCameraArmLength = CameraBoom->TargetArmLength;
		DefaultCameraSocketOffset = CameraBoom->SocketOffset;
	}

	if (UCameraComponent* FollowCamera = JYCharacter->GetFollowCamera())
	{
		DefaultCameraFieldOfView = FollowCamera->FieldOfView;
	}
}

void UJY_AimComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //override
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UJY_AimComponent, bWantsToAim);
	DOREPLIFETIME_CONDITION(UJY_AimComponent, TurnDirection, COND_SkipOwner);
}

void UJY_AimComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) //override
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateAimRotation(DeltaTime);
	UpdateAimCamera(DeltaTime);
}

void UJY_AimComponent::StartAim()
{
	/* [Owning Client] */
	const AJY_Character* JYCharacter = Cast<AJY_Character>(GetOwner());
	const UJY_EquipmentComponent* EquipmentComp = JYCharacter != nullptr ? JYCharacter->GetEquipmentComponent() : nullptr;
	if (EquipmentComp == nullptr || EquipmentComp->IsWeaponEquipped() == false)
	{
		return;
	}

	bWantsToAim = true;
	ACharacter* Character = GetOwnerCharacter();
	if (Character != nullptr)
	{
		if (AJY_PlayerController* PC = Cast<AJY_PlayerController>(Character->GetController()))
		{
			PC->SetCrosshairVisible(true);
		}
	}
	Server_SetWantsToAim(true);
}

void UJY_AimComponent::StopAim()
{
	/* [Owning Client] */
	bWantsToAim = false;
	ACharacter* Character = GetOwnerCharacter();
	if (Character != nullptr)
	{
		if (AJY_PlayerController* PC = Cast<AJY_PlayerController>(Character->GetController()))
		{
			PC->SetCrosshairVisible(false);
		}
	}
	Server_SetWantsToAim(false);
}

void UJY_AimComponent::UpdateAimCamera(float DeltaTime)
{
	AJY_Character* JYCharacter = Cast<AJY_Character>(GetOwner());
	if (JYCharacter == nullptr || JYCharacter->IsLocallyControlled() == false)
	{
		return;
	}

	USpringArmComponent* CameraBoom = JYCharacter->GetCameraBoom();
	UCameraComponent* FollowCamera = JYCharacter->GetFollowCamera();
	if (CameraBoom == nullptr || FollowCamera == nullptr)
	{
		return;
	}

	const float TargetArmLength = bWantsToAim == true ? AimCameraTargetArmLength : DefaultCameraArmLength;
	const float TargetFieldOfView = bWantsToAim == true ? AimCameraTargetFieldOfView : DefaultCameraFieldOfView;
	const FVector TargetSocketOffset = bWantsToAim == true ? AimCameraTargetSocketOffset : DefaultCameraSocketOffset;

	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArmLength, DeltaTime, AimCameraInterpSpeed);
	CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetSocketOffset, DeltaTime, AimCameraInterpSpeed);
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, TargetFieldOfView, DeltaTime, AimCameraInterpSpeed));
}

void UJY_AimComponent::UpdateAimRotation(float DeltaTime)
{
	AJY_Character* Character = Cast<AJY_Character>(GetOwnerCharacter());
	if (Character == nullptr)
	{
		return;
	}

	const UJY_EquipmentComponent* EquipmentComp = Character->GetEquipmentComponent();
	const bool bWeaponEquipped = EquipmentComp != nullptr && EquipmentComp->IsWeaponEquipped();
	const bool bIsLocallyControlled = Character->IsLocallyControlled();

	if (bWantsToAim == true && bWeaponEquipped == false && bIsLocallyControlled == true)
	{
		StopAim();
		return;
	}

	if (bWantsToAim == false && bWeaponEquipped == false)
	{
		AimPitch = 0.f;
		bWallAhead = false;

		if (bIsLocallyControlled == true && TurnDirection != EJY_TurnDirection::None)
		{
			TurnDirection = EJY_TurnDirection::None;
			Server_SetTurnDirection(TurnDirection);
		}
		return;
	}

	USkeletalMeshComponent* MeshComp = Character->GetMesh();
	if (MeshComp == nullptr)
	{
		return;
	}

	FRotator AimRotation = Character->GetBaseAimRotation();
	if (bIsLocallyControlled == true)
	{
		const AController* Controller = Character->GetController();
		if (Controller == nullptr)
			return;

		AimRotation = Controller->GetControlRotation();
	}

	const FRotator MeshRootRaw = MeshComp->GetSocketRotation(MeshRootSocketName);
	FRotator MeshRootRotation = MeshRootRaw;
	MeshRootRotation.Yaw += MeshForwardYawCorrection;

	const FRotator Delta = (AimRotation - MeshRootRotation).GetNormalized();
	AimPitch = FMath::Clamp(static_cast<float>(Delta.Pitch), -AimAngleClamp, AimAngleClamp);

	/* TIP 방향은 소유 클라만 결정, 나머진 복제값 유지 */
	if (bIsLocallyControlled == true)
	{
		const EJY_TurnDirection NewTurnDirection = CalculateTurnDirection(Delta.Yaw);
		if (TurnDirection != NewTurnDirection)
		{
			TurnDirection = NewTurnDirection;
			Server_SetTurnDirection(TurnDirection);
		}
	}

	if (bWantsToAim == true)
	{
		UpdateWallAhead(AimRotation);
	}
	else
	{
		bWallAhead = false;
	}
}

void UJY_AimComponent::UpdateWallAhead(const FRotator& AimRotation)
{
	bWallAhead = false;

	const ACharacter* Character = GetOwnerCharacter();
	UWorld* World = GetWorld();
	if (Character == nullptr || World == nullptr)
		return;

	/* 수평 방향 */
	const FVector ForwardXY = FRotator(0.f, AimRotation.Yaw, 0.f).Vector();

	const FVector StartLocation = Character->GetActorLocation();
	const FVector EndLocation = StartLocation + ForwardXY * WallCheckDistance;

	/* 자기 캡슐/무기 무시  */
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(JYWallAhead), false, Character);
	TArray<AActor*> AttachedActors;
	Character->GetAttachedActors(OUT AttachedActors);
	TraceParams.AddIgnoredActors(AttachedActors);

	FHitResult Hit;
	bWallAhead = World->LineTraceSingleByChannel(OUT Hit, StartLocation, EndLocation, ECC_Visibility, TraceParams);
}

EJY_TurnDirection UJY_AimComponent::CalculateTurnDirection(float YawAngle) const
{
	/* 각도 임계값은 ABP ShouldPerformTIP가 판정, 여기선 부호만 */
	if (FMath::IsNearlyZero(YawAngle, 1.f))
		return EJY_TurnDirection::None;

	return YawAngle > 0.f ? EJY_TurnDirection::Right : EJY_TurnDirection::Left;
}

ACharacter* UJY_AimComponent::GetOwnerCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

void UJY_AimComponent::Server_SetWantsToAim_Implementation(bool bNewWantsToAim)
{
	/* [Server] */
	bWantsToAim = bNewWantsToAim;
}

void UJY_AimComponent::Server_SetTurnDirection_Implementation(EJY_TurnDirection InTurnDirection)
{
	/* [Server] */
	TurnDirection = InTurnDirection;
}
