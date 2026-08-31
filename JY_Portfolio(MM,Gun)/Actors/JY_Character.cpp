// Copyright Epic Games, Inc. All Rights Reserved.


#include "Actors/JY_Character.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "InputActionValue.h"
#include "Components/JY_CharacterMovementComponent.h"
#include "Components/JY_TraversalComponent.h"
#include "MotionWarpingComponent.h"
#include "Components/JY_ProneComponent.h"
#include "Components/JY_HangComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/OverlapResult.h"
#include "Core/JY.h"
#include "Components/JY_HeroComponent.h"
#include "Components/JY_EquipmentComponent.h"
#include "Components/JY_AimComponent.h"
#include "Actors/JY_WeaponActor.h"
#include "Utils/JY_WeaponUtils.h"
#include "Equipment/JY_WeaponInstance.h"
#include "Framework/JY_PlayerState.h"
#include "Framework/JY_PlayerController.h"
#include "Components/JY_AbilitySystemComponent.h"
#include "Data/JY_AbilitySet.h"
#include "Data/JY_PawnData.h"
#include "AbilitySystemGlobals.h"
#include "Core/JY_GameplayTags.h"
#include "GameplayCueManager.h"
#include "GameplayEffectTypes.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Data/JY_AttributeSet.h"
#include "Animation/JY_AnimInstance.h"

AJY_Character::AJY_Character(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UJY_CharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	JYMovementComponent = Cast<UJY_CharacterMovementComponent>(GetCharacterMovement());
	JYMovementComponent->bOrientRotationToMovement = true;
	JYMovementComponent->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	JYMovementComponent->JumpZVelocity = 500.f;
	JYMovementComponent->AirControl = 0.35f;
	JYMovementComponent->MaxWalkSpeed = 500.f;
	JYMovementComponent->MinAnalogWalkSpeed = 20.f;
	JYMovementComponent->BrakingDecelerationWalking = 2000.f;
	JYMovementComponent->BrakingDecelerationFalling = 1500.0f;
	JYMovementComponent->NavAgentProps.bCanCrouch = true;

	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;

	bIsProne = false;
	bIsFullyProne = false;
	bIsHanging = false;
	bFullyHanging = false;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	ProneComponent = CreateDefaultSubobject<UJY_ProneComponent>(TEXT("ProneComponent"));
	HangComponent = CreateDefaultSubobject<UJY_HangComponent>(TEXT("HangComponent"));
	HeroComponent = CreateDefaultSubobject<UJY_HeroComponent>(TEXT("HeroComponent"));
	AimComponent = CreateDefaultSubobject<UJY_AimComponent>(TEXT("AimComponent"));

	ProneBodyCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("ProneBodyCapsule"));
	ProneBodyCapsule->SetupAttachment(GetCapsuleComponent());
	ProneBodyCapsule->InitCapsuleSize(20.f, 80.f);
	ProneBodyCapsule->SetRelativeLocation(FVector(0.07f, -0.05f, 2.11f));
	ProneBodyCapsule->SetRelativeRotation(FRotator(6.91f, 87.43f, 92.24f));
	ProneBodyCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProneBodyCapsule->SetCollisionObjectType(ECC_Pawn);
	ProneBodyCapsule->SetCollisionResponseToAllChannels(ECR_Block);
	ProneBodyCapsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
	ProneBodyCapsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	ProneBodyCapsule->ComponentTags.Add(FName("ProneCollision"));
}

UAbilitySystemComponent* AJY_Character::GetAbilitySystemComponent() const
{
	if (AJY_PlayerState* PS = GetPlayerState<AJY_PlayerState>())
	{
		return PS->GetAbilitySystemComponent();
	}
	return nullptr;
}

void AJY_Character::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //override
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AJY_Character, bIsProne, COND_SimulatedOnly);
	DOREPLIFETIME_CONDITION(AJY_Character, bIsHanging, COND_SimulatedOnly);
}

void AJY_Character::Tick(float DeltaSeconds) //override
{
	Super::Tick(DeltaSeconds);
}

void AJY_Character::BeginPlay() //override
{
	Super::BeginPlay();

	MotionWarpingComponent = FindComponentByClass<UMotionWarpingComponent>();
}

void AJY_Character::PossessedBy(AController* NewController) //override
{
	/* [Server] */
	Super::PossessedBy(NewController);
	InitializeAbilitySystem();
}

void AJY_Character::OnRep_PlayerState() //override
{
	/* [Client] */
	Super::OnRep_PlayerState();
	InitializeAbilitySystem();
}

void AJY_Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) //override
{
	if (HeroComponent != nullptr)
	{
		HeroComponent->InitializePlayerInput(PlayerInputComponent);
	}

	// 임시 치트키
	PlayerInputComponent->BindKey(EKeys::V, IE_Pressed, this, &AJY_Character::CheatTeleportForward);
}

void AJY_Character::InitializeAbilitySystem()
{
	AJY_PlayerState* PS = GetPlayerState<AJY_PlayerState>();
	if (PS == nullptr)
	{
		return;
	}

	UJY_AbilitySystemComponent* ASC = PS->GetJYAbilitySystemComponent();
	if (ASC == nullptr)
	{
		return;
	}

	ASC->InitAbilityActorInfo(PS, this);

	/* [Server] */
	if (HasAuthority() == true)
	{
		const UJY_PawnData* PawnData = HeroComponent ? HeroComponent->GetPawnData() : nullptr;
		if (PawnData && PawnData->AbilitySet)
		{
			PawnData->AbilitySet->GiveToAbilitySystem(ASC);
		}

		ASC->SetNumericAttributeBase(UJY_AttributeSet::GetMaxHealthAttribute(), MaxHealth);
		ASC->SetNumericAttributeBase(UJY_AttributeSet::GetHealthAttribute(), MaxHealth);

		if (UJY_AttributeSet* AttrSet = PS->GetAttributeSet())
		{
			AttrSet->OnHealthChanged.AddUObject(this, &AJY_Character::HandleHealthChanged);
		}
	}
}

void AJY_Character::TryJumpOrTraversal_Implementation()
{
	/* [Owning Client] */
	Jump();
}

void AJY_Character::HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float Magnitude, float NewValue)
{
	/* [Server] */
	if (HasAuthority() == false)
	{
		return;
	}

	if (DamageInstigator == nullptr)
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

void AJY_Character::MulticastRequestHitReact_Implementation(EJY_HitReactDirection Direction)
{
	/* [Server 포함 전 클라] */
	if (UJY_AnimInstance* AnimInstance = Cast<UJY_AnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AnimInstance->RequestHitReact(Direction);
	}
}

void AJY_Character::SetWantsToHang(bool bNewWantsToHang)
{
	if (IsLocallyControlled() == false)
	{
		return;
	}

	if (UJY_CharacterMovementComponent* JYCMC = GetJYCharacterMovement())
	{
		JYCMC->bWantsToHang = bNewWantsToHang;
	}
}

bool AJY_Character::GetIsHanging() const
{
	return bIsHanging;
}

void AJY_Character::SetIsHanging(bool bNewHanging)
{
	/* [Server / AutonomousProxy] */
	bIsHanging = bNewHanging;
	bFullyHanging = false;
}

void AJY_Character::OnRep_IsHanging()
{
	/* [SimulatedProxy] */
	if (UJY_CharacterMovementComponent* JYCMC = GetJYCharacterMovement())
	{
		JYCMC->bWantsToHang = bIsHanging;

		if (bIsHanging == true)
		{
			JYCMC->Hang(true);
		}
		else
		{
			JYCMC->UnHang(true);
		}
	}

	bFullyHanging = false;
}

float AJY_Character::GetAnimCurveValue(FName CurveName) const
{
	float Value = 0.f;
	if (GetMesh() != nullptr && GetMesh()->GetAnimInstance() != nullptr)
	{
		Value = GetMesh()->GetAnimInstance()->GetCurveValue(CurveName);
	}

	return Value;
}

bool AJY_Character::WantsToProne() const
{
	const UJY_CharacterMovementComponent* JYCMC = GetJYCharacterMovement();
	return JYCMC != nullptr && JYCMC->bWantsToProne;
}

void AJY_Character::Prone()
{
	if (UJY_CharacterMovementComponent* JYCMC = GetJYCharacterMovement())
	{
		JYCMC->bWantsToProne = true;
	}
}

void AJY_Character::UnProne()
{
	if (UJY_CharacterMovementComponent* JYCMC = GetJYCharacterMovement())
	{
		JYCMC->bWantsToProne = false;
	}
}

void AJY_Character::SetIsProne(bool bNewProne)
{
	bIsProne = bNewProne;
}

void AJY_Character::OnRep_IsProne()
{
	/* [SimulatedProxy] */
	if (UJY_CharacterMovementComponent* JYCMC = GetJYCharacterMovement())
	{
		if (IsProne() == true)
		{
			JYCMC->bWantsToProne = true;
			JYCMC->Prone(true);
		}
		else
		{
			JYCMC->bWantsToProne = false;
			JYCMC->UnProne(true);
		}

		JYCMC->bNetworkUpdateReceived = true;
	}
}

void AJY_Character::OnStartProne(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	const AJY_Character* CharacterCDO = GetDefault<AJY_Character>(GetClass());
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (MeshComp != nullptr && CharacterCDO->GetMesh() != nullptr)
	{
		FVector& MeshRelativeLocation = MeshComp->GetRelativeLocation_DirectMutable();
		MeshRelativeLocation.Z = CharacterCDO->GetMesh()->GetRelativeLocation().Z + HalfHeightAdjust;
		BaseTranslationOffset.Z = MeshRelativeLocation.Z;
	}
	else
	{
		BaseTranslationOffset.Z = CharacterCDO->BaseTranslationOffset.Z + HalfHeightAdjust;
	}

	if (ProneComponent != nullptr)
	{
		ProneComponent->SetProneBodyCollisionActive(true);
		ProneComponent->OnProneStarted();
	}

	if (ProneMontage != nullptr)
	{
		PlayAnimMontage(ProneMontage);
	}

	K2_OnStartProne(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void AJY_Character::OnEndProne(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	const AJY_Character* CharacterCDO = GetDefault<AJY_Character>(GetClass());
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (MeshComp != nullptr && CharacterCDO->GetMesh() != nullptr)
	{
		FVector& MeshRelativeLocation = MeshComp->GetRelativeLocation_DirectMutable();
		MeshRelativeLocation.Z = CharacterCDO->GetMesh()->GetRelativeLocation().Z;
		BaseTranslationOffset.Z = MeshRelativeLocation.Z;
	}
	else
	{
		BaseTranslationOffset.Z = CharacterCDO->BaseTranslationOffset.Z;
	}

	if (ProneComponent != nullptr)
	{
		ProneComponent->SetProneBodyCollisionActive(false);
		ProneComponent->OnProneEnded();
	}

	bIsFullyProne = false;

	if (ExitProneMontage != nullptr)
	{
		PlayAnimMontage(ExitProneMontage);
	}

	K2_OnEndProne(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void AJY_Character::CheatTeleportForward()
{
	/* [Client] */
	Server_CheatTeleportForward();
}

void AJY_Character::Server_CheatTeleportForward_Implementation()
{
	/* [Server] */
	const float CheatTeleportDistance = 500.f;
	const FVector Target = GetActorLocation() + GetActorForwardVector() * CheatTeleportDistance;
	SetActorLocation(Target, false, nullptr, ETeleportType::TeleportPhysics);
}
