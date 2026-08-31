#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "JY_PlayerState.generated.h"

class UJY_AbilitySystemComponent;
class UJY_AttributeSet;

UCLASS()
class AJY_PlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:

	AJY_PlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:

	/* 엔진/GAS가 ASC를 찾는 표준 진입점 */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UJY_AbilitySystemComponent* GetJYAbilitySystemComponent() const;

	UJY_AttributeSet* GetAttributeSet() const { return AttributeSet; }

private:

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UJY_AbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UJY_AttributeSet> AttributeSet;
};
