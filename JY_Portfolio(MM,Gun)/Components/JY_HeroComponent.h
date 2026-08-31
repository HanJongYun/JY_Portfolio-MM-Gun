#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "GameplayTagContainer.h"
#include "JY_HeroComponent.generated.h"

class UInputComponent;
class UJY_InputConfig;
class UJY_PawnData;
struct FInputActionValue;

UCLASS(meta = (BlueprintSpawnableComponent))
class UJY_HeroComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UJY_HeroComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	/* 캐릭터 SetupPlayerInputComponent에서 호출 */
	void InitializePlayerInput(UInputComponent* PlayerInputComponent);
	void RemovePlayerInput();

	FORCEINLINE const UJY_PawnData* GetPawnData() const { return PawnData; }

private:
	void Input_Move(const FInputActionValue& Value);
	void Input_LookMouse(const FInputActionValue& Value);
	void Input_LookStick(const FInputActionValue& Value);
	void ApplyLookInput(const FInputActionValue& Value);
	void Input_Crouch(const FInputActionValue& Value);

	void Input_Prone(const FInputActionValue& Value);

	void Input_Equip_Slot1(const FInputActionValue& Value);
	void Input_Equip_Slot2(const FInputActionValue& Value);
	void Input_Aim_Pressed(const FInputActionValue& Value);
	void Input_Aim_Released(const FInputActionValue& Value);
	void ResetProneToggle();

	void Input_AbilityInputTagPressed(FGameplayTag InputTag);
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);

	const UJY_InputConfig* GetInputConfig() const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "JY|Input")
	TObjectPtr<const UJY_PawnData> PawnData;

	TArray<uint32> InputBindHandles;

	bool bCanToggleProne = true;

	UPROPERTY(EditAnywhere, Category = "JY|Input|Prone")
	float ProneToggleCooldown = 3.f;

	FTimerHandle ProneToggleTimer;

	UPROPERTY(EditAnywhere, Category = "JY|Input|Aim")
	float AimLookSensitivityScale = 0.5f;
};
