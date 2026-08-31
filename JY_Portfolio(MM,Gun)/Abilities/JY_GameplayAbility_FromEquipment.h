#pragma once

#include "CoreMinimal.h"
#include "Abilities/JY_GameplayAbility.h"
#include "JY_GameplayAbility_FromEquipment.generated.h"

class UJY_EquipmentInstance;

UCLASS(Abstract)
class UJY_GameplayAbility_FromEquipment : public UJY_GameplayAbility
{
	GENERATED_BODY()

public:
	UJY_GameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	UFUNCTION(BlueprintCallable, Category = "JY|Ability")
	UJY_EquipmentInstance* GetAssociatedEquipment() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
