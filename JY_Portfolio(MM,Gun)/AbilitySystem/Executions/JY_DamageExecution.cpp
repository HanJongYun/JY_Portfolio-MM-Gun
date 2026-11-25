#include "AbilitySystem/Executions/JY_DamageExecution.h"

#include "AbilitySystemComponent.h"
#include "Actors/JY_WeaponActor.h"
#include "Data/JY_WeaponData.h"
#include "Data/JY_AttributeSet.h"
#include "GameplayEffectTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_DamageExecution)

void UJY_DamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	/* [Server] */
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const FGameplayEffectContextHandle Context = Spec.GetContext();

	AActor* Instigator = Context.GetOriginalInstigator();
	AActor* TargetActor = ExecutionParams.GetTargetAbilitySystemComponent() != nullptr
		? ExecutionParams.GetTargetAbilitySystemComponent()->GetAvatarActor_Direct()
		: nullptr;

	if (Instigator != nullptr && Instigator == TargetActor)
		return;

	/* 들고 있는 무기가 아니라 발사한 무기의 데미지 */
	const AJY_WeaponActor* WeaponActor = Cast<AJY_WeaponActor>(Context.GetEffectCauser());
	const UJY_WeaponData* WeaponData = WeaponActor != nullptr ? WeaponActor->GetWeaponData() : nullptr;
	if (WeaponData == nullptr)
		return;

	float BaseDamage = WeaponData->BaseDamage;
	if (BaseDamage <= 0.f)
		return;

	const FHitResult* HitResult = Context.GetHitResult();
	if (HitResult != nullptr && HitResult->BoneName == HeadBoneName)
	{
		BaseDamage *= HeadshotDamageMultiplier;
	}

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UJY_AttributeSet::GetDamageAttribute(), EGameplayModOp::Additive, BaseDamage));
}
