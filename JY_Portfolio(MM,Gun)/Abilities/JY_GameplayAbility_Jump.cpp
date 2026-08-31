#include "Abilities/JY_GameplayAbility_Jump.h"
#include "Actors/JY_Character.h"

UJY_GameplayAbility_Jump::UJY_GameplayAbility_Jump(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

void UJY_GameplayAbility_Jump::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (AJY_Character* Character = Cast<AJY_Character>(ActorInfo->AvatarActor.Get()))
	{
		Character->TryJumpOrTraversal();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
