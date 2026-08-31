#include "Components/JY_HeroComponent.h"

#include "Actors/JY_Character.h"
#include "Components/JY_AbilitySystemComponent.h"
#include "Components/JY_EquipmentComponent.h"
#include "Components/JY_AimComponent.h"
#include "Data/JY_PawnData.h"
#include "Components/JY_InputComponent.h"
#include "Data/JY_InputConfig.h"
#include "Core/JY_GameplayTags.h"
#include "Framework/JY_PlayerState.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Controller.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"

UJY_HeroComponent::UJY_HeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UJY_HeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	/* [Owning Client] */
	UJY_InputComponent* InputComp = CastChecked<UJY_InputComponent>(PlayerInputComponent);

	APawn* Pawn = GetPawn<APawn>();
	if (Pawn == nullptr)
	{
		return;
	}

	APlayerController* PC = Pawn->GetController<APlayerController>();
	if (PC != nullptr)
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
		if (Subsystem != nullptr && PawnData != nullptr)
		{
			for (const FJYInputMappingContextAndPriority& Mapping : PawnData->DefaultInputMappings)
			{
				if (Mapping.InputMapping != nullptr)
				{
					Subsystem->AddMappingContext(Mapping.InputMapping, Mapping.Priority);
				}
			}
		}
	}

	const UJY_InputConfig* InputConfig = GetInputConfig();
	if (InputConfig == nullptr)
	{
		return;
	}

	InputComp->BindNativeAction(InputConfig, JYGameplayTags::InputTag_Move,       ETriggerEvent::Triggered, this, &ThisClass::Input_Move,      false);
	InputComp->BindNativeAction(InputConfig, JYGameplayTags::InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ThisClass::Input_LookMouse,  false);
	InputComp->BindNativeAction(InputConfig, JYGameplayTags::InputTag_Look_Stick, ETriggerEvent::Triggered, this, &ThisClass::Input_LookStick,  false);
	InputComp->BindNativeAction(InputConfig, JYGameplayTags::InputTag_Crouch,     ETriggerEvent::Started,   this, &ThisClass::Input_Crouch,     false);
	InputComp->BindNativeAction(InputConfig, JYGameplayTags::InputTag_Prone,      ETriggerEvent::Started,   this, &ThisClass::Input_Prone,      false);
	InputComp->BindNativeAction(InputConfig, JYGameplayTags::InputTag_Equip_Slot1, ETriggerEvent::Started,   this, &ThisClass::Input_Equip_Slot1, false);
	InputComp->BindNativeAction(InputConfig, JYGameplayTags::InputTag_Equip_Slot2, ETriggerEvent::Started,   this, &ThisClass::Input_Equip_Slot2, false);
	InputComp->BindNativeAction(InputConfig, JYGameplayTags::InputTag_Aim,          ETriggerEvent::Started,   this, &ThisClass::Input_Aim_Pressed,  false);
	InputComp->BindNativeAction(InputConfig, JYGameplayTags::InputTag_Aim,          ETriggerEvent::Completed, this, &ThisClass::Input_Aim_Released, false);

	InputComp->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, InputBindHandles);
}

void UJY_HeroComponent::RemovePlayerInput()
{
	APawn* Pawn = GetPawn<APawn>();
	if (Pawn == nullptr)
	{
		return;
	}

	if (UJY_InputComponent* InputComp = Pawn->FindComponentByClass<UJY_InputComponent>())
	{
		InputComp->RemoveBinds(InputBindHandles);
	}

	APlayerController* PC = Pawn->GetController<APlayerController>();
	if (PC != nullptr)
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
		if (Subsystem != nullptr && PawnData != nullptr)
		{
			for (const FJYInputMappingContextAndPriority& Mapping : PawnData->DefaultInputMappings)
			{
				if (Mapping.InputMapping != nullptr)
				{
					Subsystem->RemoveMappingContext(Mapping.InputMapping);
				}
			}
		}
	}
}

void UJY_HeroComponent::Input_Move(const FInputActionValue& Value)
{
	AJY_Character* Character = Cast<AJY_Character>(GetPawn<APawn>());
	if (Character == nullptr)
	{
		return;
	}

	const FVector2D MoveValue = Value.Get<FVector2D>();
	float Right = MoveValue.X;
	float Forward = MoveValue.Y;

	AController* Controller = Character->GetController();
	if (Controller == nullptr)
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = Character->GetMesh();
	UAnimInstance* AnimInstance = MeshComp != nullptr ? MeshComp->GetAnimInstance() : nullptr;
	if (AnimInstance != nullptr)
	{
		float MoveInputScale = 0.f;
		if (AnimInstance->GetCurveValue(FName("MoveInputAll"), OUT MoveInputScale))
		{
			Forward *= MoveInputScale;
			Right *= MoveInputScale;
		}
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	Character->AddMovementInput(ForwardDirection, Forward);
	Character->AddMovementInput(RightDirection, Right);
}

void UJY_HeroComponent::Input_LookMouse(const FInputActionValue& Value)
{
	ApplyLookInput(Value);
}

void UJY_HeroComponent::Input_LookStick(const FInputActionValue& Value)
{
	ApplyLookInput(Value);
}

void UJY_HeroComponent::ApplyLookInput(const FInputActionValue& Value)
{
	AJY_Character* Character = Cast<AJY_Character>(GetPawn<APawn>());
	if (Character == nullptr)
	{
		return;
	}

	const FVector2D LookValue = Value.Get<FVector2D>();

	float LookScale = 1.f;
	UJY_AimComponent* AimComp = Character->GetAimComponent();
	if (AimComp != nullptr && AimComp->IsAiming() == true)
	{
		LookScale = AimLookSensitivityScale;
	}

	Character->AddControllerYawInput(LookValue.X * LookScale);
	Character->AddControllerPitchInput(LookValue.Y * LookScale);
}

void UJY_HeroComponent::Input_Crouch(const FInputActionValue& Value)
{
	AJY_Character* Character = Cast<AJY_Character>(GetPawn<APawn>());
	if (Character == nullptr)
	{
		return;
	}

	if (Character->IsProne() == true)
	{
		return;
	}

	const UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	const bool bCrouchRequested = MovementComponent != nullptr && MovementComponent->bWantsToCrouch;

	if (Character->bIsCrouched == true || bCrouchRequested == true)
	{
		Character->UnCrouch();
	}
	else
	{
		Character->Crouch();
	}
}

void UJY_HeroComponent::Input_Equip_Slot1(const FInputActionValue& Value)
{
	AJY_Character* Character = Cast<AJY_Character>(GetPawn<APawn>());
	if (Character == nullptr)
	{
		return;
	}

	UJY_EquipmentComponent* EquipmentComp = Character->GetEquipmentComponent();
	if (EquipmentComp == nullptr)
	{
		return;
	}

	EquipmentComp->RequestActiveSlot(0);
}

void UJY_HeroComponent::Input_Equip_Slot2(const FInputActionValue& Value)
{
	AJY_Character* Character = Cast<AJY_Character>(GetPawn<APawn>());
	if (Character == nullptr)
	{
		return;
	}

	UJY_EquipmentComponent* EquipmentComp = Character->GetEquipmentComponent();
	if (EquipmentComp == nullptr)
	{
		return;
	}

	EquipmentComp->RequestActiveSlot(1);
}

void UJY_HeroComponent::Input_Aim_Pressed(const FInputActionValue& Value)
{
	AJY_Character* Character = Cast<AJY_Character>(GetPawn<APawn>());
	if (Character == nullptr)
	{
		return;
	}

	if (UJY_AimComponent* AimComp = Character->GetAimComponent())
	{
		AimComp->StartAim();
	}
}

void UJY_HeroComponent::Input_Aim_Released(const FInputActionValue& Value)
{
	AJY_Character* Character = Cast<AJY_Character>(GetPawn<APawn>());
	if (Character == nullptr)
	{
		return;
	}

	if (UJY_AimComponent* AimComp = Character->GetAimComponent())
	{
		AimComp->StopAim();
	}
}

void UJY_HeroComponent::Input_Prone(const FInputActionValue& Value)
{
	AJY_Character* Character = Cast<AJY_Character>(GetPawn<APawn>());
	if (Character == nullptr)
	{
		return;
	}

	if (bCanToggleProne == false)
	{
		return;
	}

	/* 의도만 토글, 실제 전이는 CMC가 매 틱 판정 */
	if (Character->IsProne() == false)
	{
		Character->Prone();
	}
	else
	{
		Character->UnProne();
	}

	bCanToggleProne = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ProneToggleTimer, this, &UJY_HeroComponent::ResetProneToggle, ProneToggleCooldown, false);
	}
}

void UJY_HeroComponent::ResetProneToggle()
{
	bCanToggleProne = true;
}

void UJY_HeroComponent::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	APawn* Pawn = GetPawn<APawn>();
	if (Pawn == nullptr)
	{
		return;
	}

	AJY_PlayerState* PS = Pawn->GetPlayerState<AJY_PlayerState>();
	if (PS == nullptr)
	{
		return;
	}

	if (UJY_AbilitySystemComponent* ASC = PS->GetJYAbilitySystemComponent())
	{
		ASC->AbilityInputTagPressed(InputTag);
	}
}

void UJY_HeroComponent::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	APawn* Pawn = GetPawn<APawn>();
	if (Pawn == nullptr)
	{
		return;
	}

	AJY_PlayerState* PS = Pawn->GetPlayerState<AJY_PlayerState>();
	if (PS == nullptr)
	{
		return;
	}

	if (UJY_AbilitySystemComponent* ASC = PS->GetJYAbilitySystemComponent())
	{
		ASC->AbilityInputTagReleased(InputTag);
	}
}

const UJY_InputConfig* UJY_HeroComponent::GetInputConfig() const
{
	return PawnData != nullptr ? PawnData->InputConfig : nullptr;
}
