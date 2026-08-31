// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/JY_Types.h"
#include "JY_WeaponGlobalData.generated.h"

class UJY_WeaponData;
class UGameplayEffect;

UCLASS(BlueprintType)
class UJY_WeaponGlobalData : public UDataAsset
{
	GENERATED_BODY()

public:
	UJY_WeaponData* GetWeaponData(EJY_WeaponAnimType Type) const;

public:
	/* 무기 종류/데이터 참조 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon")
	TMap<EJY_WeaponAnimType, TObjectPtr<UJY_WeaponData>> Weapons;

	/* 공용 데미지 GameplayEffect, 무기별 차이는 BaseDamage 어트리뷰트 값으로만 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Damage")
	TSubclassOf<UGameplayEffect> DamageGameplayEffectClass;
};
