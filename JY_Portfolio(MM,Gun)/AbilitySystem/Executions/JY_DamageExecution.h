#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "JY_DamageExecution.generated.h"

UCLASS()
class UJY_DamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UJY_DamageExecution();

public:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	FName HeadBoneName = TEXT("head");

	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "1.0"))
	float HeadshotDamageMultiplier = 2.f;

private:
	FGameplayEffectAttributeCaptureDefinition BaseDamageCaptureDef;
};
