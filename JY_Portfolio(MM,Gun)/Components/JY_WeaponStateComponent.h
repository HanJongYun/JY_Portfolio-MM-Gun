#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "JY_WeaponStateComponent.generated.h"

UCLASS(ClassGroup = (JY))
class UJY_WeaponStateComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	UJY_WeaponStateComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
