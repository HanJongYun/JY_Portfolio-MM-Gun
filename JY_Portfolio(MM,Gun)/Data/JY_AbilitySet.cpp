#include "Data/JY_AbilitySet.h"
#include "Components/JY_AbilitySystemComponent.h"
#include "Abilities/JY_GameplayAbility.h"

void FJYAbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		AbilitySpecHandles.Add(Handle);
	}
}

void FJYAbilitySet_GrantedHandles::TakeFromAbilitySystem(UJY_AbilitySystemComponent* ASC)
{
	/* [Server] */
	if (ASC == nullptr)
		return;

	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}

	AbilitySpecHandles.Reset();
}

void UJY_AbilitySet::GiveToAbilitySystem(UJY_AbilitySystemComponent* ASC, FJYAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject) const
{
	/* [Server] */
	check(ASC);
	if (ASC->IsOwnerActorAuthoritative() == false)
		return;

	for (const FJYAbilitySet_GameplayAbility& Entry : GrantedGameplayAbilities)
	{
		if (Entry.Ability == nullptr)
		{
			continue;
		}

		FGameplayAbilitySpec AbilitySpec(Entry.Ability, Entry.AbilityLevel);
		AbilitySpec.SourceObject = SourceObject;

		/* 입력태그를 스펙에 심음 -> ASC::AbilityInputTagPressed가 이 태그로 어빌리티 탐색 */
		if (Entry.InputTag.IsValid())
		{
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(Entry.InputTag);
		}

		FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(AbilitySpec);

		if (OutGrantedHandles != nullptr)
		{
			OutGrantedHandles->AddAbilitySpecHandle(Handle);
		}
	}
}
