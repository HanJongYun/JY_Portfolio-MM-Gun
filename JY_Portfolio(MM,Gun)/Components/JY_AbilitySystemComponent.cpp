#include "Components/JY_AbilitySystemComponent.h"

#include "Abilities/JY_GameplayAbility.h"

UJY_AbilitySystemComponent::UJY_AbilitySystemComponent()
{
}

void UJY_AbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	/* [Owning Client] */
	if (InputTag.IsValid() == false)
	{
		return;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (AbilitySpec.Ability != nullptr && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
			InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
		}
	}
}

void UJY_AbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	/* [Owning Client] */
	if (InputTag.IsValid() == false)
	{
		return;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (AbilitySpec.Ability != nullptr && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
			InputHeldSpecHandles.Remove(AbilitySpec.Handle);
		}
	}
}

void UJY_AbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	/* [Owning Client] */
	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();

	/* WhileInputActive 정책, 비활성 어빌리티 계속 재시도 */
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		if (const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability != nullptr && AbilitySpec->IsActive() == false)
			{
				const UJY_GameplayAbility* JYAbilityCDO = Cast<UJY_GameplayAbility>(AbilitySpec->Ability);
				if (JYAbilityCDO != nullptr && JYAbilityCDO->GetActivationPolicy() == EJYAbilityActivationPolicy::WhileInputActive)
				{
					AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
				}
			}
		}
	}

	/* OnInputTriggered 정책만 여기서 활성화 */
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability != nullptr)
			{
				AbilitySpec->InputPressed = true;

				if (AbilitySpec->IsActive())
				{
					AbilitySpecInputPressed(*AbilitySpec);
				}
				else
				{
					const UJY_GameplayAbility* JYAbilityCDO = Cast<UJY_GameplayAbility>(AbilitySpec->Ability);
					if (JYAbilityCDO != nullptr && JYAbilityCDO->GetActivationPolicy() == EJYAbilityActivationPolicy::OnInputTriggered)
					{
						AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
					}
				}
			}
		}
	}

	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(AbilitySpecHandle);
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability != nullptr)
			{
				AbilitySpec->InputPressed = false;

				if (AbilitySpec->IsActive())
				{
					AbilitySpecInputReleased(*AbilitySpec);
				}
			}
		}
	}

	/* Held는 안 비움, 키 뗄 때까지 유지 */
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UJY_AbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}
