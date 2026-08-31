#include "Framework/JY_PlayerState.h"
#include "Components/JY_AbilitySystemComponent.h"
#include "Data/JY_AttributeSet.h"

AJY_PlayerState::AJY_PlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UJY_AbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UJY_AttributeSet>(TEXT("AttributeSet"));

	SetNetUpdateFrequency(100.f);
}

UAbilitySystemComponent* AJY_PlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UJY_AbilitySystemComponent* AJY_PlayerState::GetJYAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
