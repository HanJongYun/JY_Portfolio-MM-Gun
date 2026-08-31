#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "JY_AttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_MULTICAST_DELEGATE_FiveParams(FJY_AttributeEvent, AActor* /*Instigator*/, AActor* /*Causer*/, const FGameplayEffectSpec* /*Spec*/, float /*Magnitude*/, float /*NewValue*/);

UCLASS()
class UJY_AttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UJY_AttributeSet();

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	ATTRIBUTE_ACCESSORS(UJY_AttributeSet, Health)
	ATTRIBUTE_ACCESSORS(UJY_AttributeSet, MaxHealth)
	ATTRIBUTE_ACCESSORS(UJY_AttributeSet, Damage)
	ATTRIBUTE_ACCESSORS(UJY_AttributeSet, Healing)
	ATTRIBUTE_ACCESSORS(UJY_AttributeSet, BaseDamage)

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

public:
	/* Health 변경마다 브로드캐스트 */
	FJY_AttributeEvent OnHealthChanged;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Attributes")
	FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Attributes")
	FGameplayAttributeData MaxHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", Meta = (HideFromModifiers))
	FGameplayAttributeData Damage;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", Meta = (HideFromModifiers))
	FGameplayAttributeData Healing;

	/* 장착 무기 기본 데미지, 장착 시 WeaponData->BaseDamage로 셋팅 */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData BaseDamage;
};
