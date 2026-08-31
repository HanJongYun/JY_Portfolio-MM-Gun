// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/JY_BaseCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Actors/JY_WeaponActor.h"
#include "Components/JY_EquipmentComponent.h"
#include "Core/JY_GameplayTags.h"
#include "Framework/JY_PlayerController.h"
#include "GameplayCueManager.h"
#include "GameplayEffectTypes.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Utils/JY_WeaponUtils.h"

AJY_BaseCharacter::AJY_BaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EquipmentComponent = CreateDefaultSubobject<UJY_EquipmentComponent>(TEXT("EquipmentComponent"));
}

UAbilitySystemComponent* AJY_BaseCharacter::GetAbilitySystemComponent() const
{
	return nullptr;
}

FGenericTeamId AJY_BaseCharacter::GetGenericTeamId() const
{
	return TeamId;
}

void AJY_BaseCharacter::ServerReportBallisticImpact_Implementation(const FHitResult& HitResult, FPredictionKey PredictionKey)
{
	if (HasAuthority() == false)
		return;

	ExecuteBallisticImpactCue(HitResult, PredictionKey);

	if (ApplyBallisticImpactDamage(HitResult) == true)
	{
		if (AJY_PlayerController* PC = Cast<AJY_PlayerController>(GetController()))
		{
			PC->ClientPlayHitMarker();
		}
	}
}

/* [Server] */
void AJY_BaseCharacter::ExecuteBallisticImpactCue(const FHitResult& HitResult, FPredictionKey PredictionKey)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC == nullptr)
		return;

	UGameplayCueManager* GameplayCueManager = UAbilitySystemGlobals::Get().GetGameplayCueManager();
	if (GameplayCueManager == nullptr)
		return;

	FGameplayCueParameters CueParameters;
	CueParameters.Location = HitResult.ImpactPoint;
	CueParameters.Normal = HitResult.ImpactNormal;
	CueParameters.PhysicalMaterial = HitResult.PhysMaterial;

	GameplayCueManager->InvokeGameplayCueExecuted_WithParams(ASC, JYGameplayTags::GameplayCue_Weapon_Impact, PredictionKey, CueParameters);
}

/* [Server] */
bool AJY_BaseCharacter::ApplyBallisticImpactDamage(const FHitResult& HitResult)
{
	AActor* WeaponActor = EquipmentComponent != nullptr ? EquipmentComponent->GetActiveWeapon() : nullptr;
	return JY_WeaponUtils::ApplyWeaponDamage(this, WeaponActor, HitResult);
}
