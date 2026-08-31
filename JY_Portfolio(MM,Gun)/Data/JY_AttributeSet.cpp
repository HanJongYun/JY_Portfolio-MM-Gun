#include "Data/JY_AttributeSet.h"
#include "Core/JY.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UJY_AttributeSet::UJY_AttributeSet()
{
}

void UJY_AttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
}

void UJY_AttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	AActor* Instigator = Context.GetOriginalInstigator();
	AActor* Causer = Context.GetEffectCauser();

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float DamageDone = GetDamage();
		SetDamage(0.f);

		if (DamageDone > 0.f)
		{
			SetHealth(FMath::Clamp(GetHealth() - DamageDone, 0.f, GetMaxHealth()));
			OnHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, DamageDone, GetHealth());
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealingAttribute())
	{
		const float HealingDone = GetHealing();
		SetHealing(0.f);

		if (HealingDone > 0.f)
		{
			SetHealth(FMath::Clamp(GetHealth() + HealingDone, 0.f, GetMaxHealth()));
			OnHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, HealingDone, GetHealth());
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
}

void UJY_AttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UJY_AttributeSet, Health,    COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UJY_AttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UJY_AttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJY_AttributeSet, Health, OldHealth);
}

void UJY_AttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJY_AttributeSet, MaxHealth, OldMaxHealth);
}
