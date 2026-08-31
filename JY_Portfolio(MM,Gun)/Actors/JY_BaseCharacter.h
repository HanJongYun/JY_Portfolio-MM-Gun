// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayPrediction.h"
#include "JY_BaseCharacter.generated.h"

class UJY_EquipmentComponent;
class UAbilitySystemComponent;

UCLASS(abstract)
class AJY_BaseCharacter : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AJY_BaseCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	/* 자식이 각자 소유 위치에 맞게 override */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual FGenericTeamId GetGenericTeamId() const override;

	FORCEINLINE UJY_EquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

	UFUNCTION(Server, Reliable)
	void ServerReportBallisticImpact(const FHitResult& HitResult, FPredictionKey PredictionKey);

protected:
	void ExecuteBallisticImpactCue(const FHitResult& HitResult, FPredictionKey PredictionKey);
	bool ApplyBallisticImpactDamage(const FHitResult& HitResult);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JY|Components")
	TObjectPtr<UJY_EquipmentComponent> EquipmentComponent;

	UPROPERTY(EditDefaultsOnly, Category = "JY|Combat", meta = (ClampMin = "1.0"))
	float MaxHealth = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "JY|Combat")
	FGenericTeamId TeamId = FGenericTeamId(0);
};
