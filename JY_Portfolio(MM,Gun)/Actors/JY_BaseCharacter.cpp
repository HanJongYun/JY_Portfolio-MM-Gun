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

void AJY_BaseCharacter::ServerReportBallisticImpact_Implementation(const FHitResult& HitResult, FPredictionKey PredictionKey, AJY_WeaponActor* WeaponActor)
{
	if (HasAuthority() == false)
		return;

	ExecuteBallisticImpactCue(HitResult, PredictionKey);

	if (ApplyBallisticImpactDamage(HitResult, WeaponActor) == true)
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
bool AJY_BaseCharacter::ApplyBallisticImpactDamage(const FHitResult& HitResult, AJY_WeaponActor* WeaponActor)
{
	/* 클라가 보낸 무기는 이 캐릭터가 가진 무기일 때만 사용 */
	if (WeaponActor == nullptr || WeaponActor->GetOwner() != this)
		return false;

	return JY_WeaponUtils::ApplyWeaponDamage(this, WeaponActor, HitResult);
}
