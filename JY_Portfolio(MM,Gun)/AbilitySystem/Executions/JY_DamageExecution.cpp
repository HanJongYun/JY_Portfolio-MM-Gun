#include "AbilitySystem/Executions/JY_DamageExecution.h"

#include "AbilitySystemComponent.h"
#include "Data/JY_AttributeSet.h"
#include "GameplayEffectTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_DamageExecution)

UJY_DamageExecution::UJY_DamageExecution()
{
	/* Source(공격자) ASC의 BaseDamage 캡쳐 (GE Spec 만들어지는 시점(총 쏜 시점)으로 고정) */
	BaseDamageCaptureDef = FGameplayEffectAttributeCaptureDefinition(UJY_AttributeSet::GetBaseDamageAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
	RelevantAttributesToCapture.Add(BaseDamageCaptureDef);
}


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

	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;

	float BaseDamage = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(BaseDamageCaptureDef, EvaluateParameters, BaseDamage);

	if (BaseDamage <= 0.f)
		return;

	const FHitResult* HitResult = Context.GetHitResult();
	if (HitResult != nullptr && HitResult->BoneName == HeadBoneName)
	{
		BaseDamage *= HeadshotDamageMultiplier;
	}

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UJY_AttributeSet::GetDamageAttribute(), EGameplayModOp::Additive, BaseDamage));
}
