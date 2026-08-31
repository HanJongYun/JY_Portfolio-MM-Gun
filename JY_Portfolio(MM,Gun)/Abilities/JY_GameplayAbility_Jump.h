#pragma once

#include "CoreMinimal.h"
#include "Abilities/JY_GameplayAbility.h"
#include "JY_GameplayAbility_Jump.generated.h"

UCLASS()
class UJY_GameplayAbility_Jump : public UJY_GameplayAbility
{
	GENERATED_BODY()

public:
	UJY_GameplayAbility_Jump(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
