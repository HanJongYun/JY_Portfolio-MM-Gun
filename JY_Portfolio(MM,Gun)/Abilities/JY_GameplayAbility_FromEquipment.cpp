#include "Abilities/JY_GameplayAbility_FromEquipment.h"

#include "Equipment/JY_EquipmentInstance.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UJY_GameplayAbility_FromEquipment::UJY_GameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UJY_EquipmentInstance* UJY_GameplayAbility_FromEquipment::GetAssociatedEquipment() const
{
	if (FGameplayAbilitySpec* Spec = UGameplayAbility::GetCurrentAbilitySpec())
	{
		return Cast<UJY_EquipmentInstance>(Spec->SourceObject.Get());
	}

	return nullptr;
}

#if WITH_EDITOR
EDataValidationResult UJY_GameplayAbility_FromEquipment::IsDataValid(FDataValidationContext& Context) const //override
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	if (InstancingPolicy == EGameplayAbilityInstancingPolicy::NonInstanced)
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	{
		Context.AddError(NSLOCTEXT("JY", "EquipmentAbilityMustBeInstanced", "Equipment Ability는 인스턴스형이어야 합니다."));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif
