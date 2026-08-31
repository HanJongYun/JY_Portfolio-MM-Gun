#include "Abilities/JY_GameplayAbility.h"

UJY_GameplayAbility::UJY_GameplayAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationPolicy = EJYAbilityActivationPolicy::OnInputTriggered;
}
