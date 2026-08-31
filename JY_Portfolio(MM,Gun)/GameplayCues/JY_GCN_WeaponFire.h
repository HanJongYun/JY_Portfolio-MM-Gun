#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Burst.h"
#include "JY_GCN_WeaponFire.generated.h"

UCLASS()
class UJY_GCN_WeaponFire : public UGameplayCueNotify_Burst
{
	GENERATED_BODY()

protected:
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;
};
