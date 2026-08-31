// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/JY_WeaponGlobalData.h"
#include "Data/JY_WeaponData.h"

UJY_WeaponData* UJY_WeaponGlobalData::GetWeaponData(EJY_WeaponAnimType Type) const
{
	const TObjectPtr<UJY_WeaponData>* Found = Weapons.Find(Type);
	return (Found != nullptr) ? Found->Get() : nullptr;
}
