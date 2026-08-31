// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "JY_GlobalSettings.generated.h"

class UJY_WeaponGlobalData;

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "JY Global Settings"))
class UJY_GlobalSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, config, Category = "JY|Weapon")
	TSoftObjectPtr<UJY_WeaponGlobalData> WeaponGlobalData;
};
