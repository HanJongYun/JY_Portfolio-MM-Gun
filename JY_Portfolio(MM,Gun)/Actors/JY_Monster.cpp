// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/JY_Monster.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Components/JY_AbilitySystemComponent.h"
#include "Data/JY_AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "Components/JY_EquipmentComponent.h"
#include "Core/JY_Types.h"
#include "AI/JY_MonsterAIController.h"
#include "Core/JY.h"
#include "Animation/JY_MonsterAnimInstance.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Utils/JY_WeaponUtils.h"

AJY_Monster::AJY_Monster()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	AbilitySystemComponent = CreateDefaultSubobject<UJY_AbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UJY_AttributeSet>(TEXT("AttributeSet"));

	MaxHealth = 500.f;
	TeamId = FGenericTeamId(1);

	AIControllerClass = AJY_MonsterAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AJY_Monster::BeginPlay() //override
{
	Super::BeginPlay();

	/* [Server] */
	if (HasAuthority() == true)
	{
		const int32 SlotIndex = EquipmentComponent->AddWeapon(InitialWeaponType);
		EquipmentComponent->SetActiveSlot(SlotIndex);
	}
}

void AJY_Monster::Tick(float DeltaSeconds) //override
{
	Super::Tick(DeltaSeconds);

	if (bIsRagdoll == true)
	{
		FollowRagdoll();
	}
}

void AJY_Monster::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (HasAuthority() == false)
		return;

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->SetNumericAttributeBase(UJY_AttributeSet::GetMaxHealthAttribute(), MaxHealth);
	AbilitySystemComponent->SetNumericAttributeBase(UJY_AttributeSet::GetHealthAttribute(), MaxHealth);

	AttributeSet->OnHealthChanged.AddUObject(this, &AJY_Monster::HandleHealthChanged);
}

UAbilitySystemComponent* AJY_Monster::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}


void AJY_Monster::StartRagdoll(const FVector& Impulse)
{
	if (bIsRagdoll == true)
	{
		return;
	}
	bIsRagdoll = true;
	SetActorTickEnabled(true);

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionObjectType(ECC_PhysicsBody);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
		MeshComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		MeshComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

		MeshComp->PhysicsTransformUpdateMode = EPhysicsTransformUpdateMode::ComponentTransformIsKinematic;

		MeshComp->SetAllBodiesBelowSimulatePhysics(PelvisBone, true, true);
		MeshComp->SetAllBodiesBelowPhysicsBlendWeight(PelvisBone, 1.f);
		MeshComp->WakeAllRigidBodies();
		MeshComp->AddImpulse(Impulse, PelvisBone, true);
	}
}

void AJY_Monster::HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float Magnitude, float NewValue)
{
	/* [Server] */
	if (HasAuthority() == false)
	{
		return;
	}

	if (NewValue <= 0.f)
	{
		HandleDeath(DamageInstigator);
		return;
	}

	if (DamageInstigator == nullptr)
	{
		return;
	}

	if (GetTeamAttitudeTowards(*DamageInstigator) == ETeamAttitude::Hostile)
	{
		if (AJY_MonsterAIController* MonsterController = Cast<AJY_MonsterAIController>(GetController()))
		{
			MonsterController->SetTargetActor(DamageInstigator);
		}
	}

	if (bIsRagdoll == true)
	{
		return;
	}

	const FHitResult* HitResult = DamageEffectSpec != nullptr ? DamageEffectSpec->GetContext().GetHitResult() : nullptr;
	if (HitResult == nullptr)
	{
		return;
	}

	const EJY_HitReactDirection Direction = JY_WeaponUtils::CalculateHitReactDirection(GetActorLocation(), GetActorForwardVector(), DamageInstigator->GetActorLocation());
	MulticastRequestHitReact(Direction);
}

void AJY_Monster::HandleDeath(AActor* DamageInstigator)
{
	/* [Server] */
	if (AAIController* MonsterController = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* Brain = MonsterController->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Dead"));
		}
	}

	FVector Impulse = FVector::ZeroVector;
	if (DamageInstigator != nullptr)
	{
		const FVector ToInstigator = (DamageInstigator->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		Impulse = ToInstigator * -1.f * RagdollSettings.LaunchImpulseStrength + FVector::UpVector * (RagdollSettings.LaunchImpulseStrength * RagdollSettings.LaunchVerticalRatio);
	}

	MulticastStartRagdoll(Impulse, true);

	SetLifeSpan(RagdollSettings.DeathDuration);
}

void AJY_Monster::MulticastRequestHitReact_Implementation(EJY_HitReactDirection Direction)
{
	/* [Server 포함 전 클라] */
	if (UJY_MonsterAnimInstance* AnimInstance = Cast<UJY_MonsterAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AnimInstance->RequestHitReact(Direction);
	}
}

void AJY_Monster::MulticastStartRagdoll_Implementation(FVector Impulse, bool bIsDeath)
{
	/* [Server 포함 전 클라] */
	bIsDead = bIsDeath;
	StartRagdoll(Impulse);
}

void AJY_Monster::FollowRagdoll()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (MeshComp == nullptr)
	{
		return;
	}

	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector PelvisLoc = MeshComp->GetSocketLocation(PelvisBone);
	const FRotator PelvisRot = MeshComp->GetSocketRotation(PelvisBone);

	const FVector PelvisFwdDir = FRotationMatrix(PelvisRot).GetUnitAxis(EAxis::Y);
	bFacingUp = (PelvisFwdDir.Z > 0.f);

	float TargetYaw = PelvisRot.Yaw;
	if (bFacingUp == true)
	{
		TargetYaw -= 180.f;
	}
	const FRotator TargetRot(0.f, TargetYaw, 0.f);

	FVector TargetLoc = PelvisLoc;
	{
		FHitResult GroundHit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		if (GetWorld()->LineTraceSingleByChannel(GroundHit, PelvisLoc, PelvisLoc - FVector(0.f, 0.f, HalfHeight + 30.f), ECC_WorldStatic, Params))
		{
			TargetLoc.Z = GroundHit.ImpactPoint.Z + HalfHeight + 1.f;
		}
	}

	if (HasAuthority() == true)
	{
		SetActorLocationAndRotation(TargetLoc, TargetRot);
	}
}
