#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "JY_GameplayAbility.generated.h"

/* 어빌리티가 언제 활성화를 시도할지 */
UENUM(BlueprintType)
enum class EJYAbilityActivationPolicy : uint8
{
	OnInputTriggered,
	WhileInputActive,
	OnSpawn
};

UCLASS(Abstract)
class UJY_GameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UJY_GameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	EJYAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JY|Ability Activation")
	EJYAbilityActivationPolicy ActivationPolicy;
};
