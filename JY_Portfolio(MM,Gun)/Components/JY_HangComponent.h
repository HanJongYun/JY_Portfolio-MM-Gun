// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JY_HangComponent.generated.h"

class AJY_Character;
class UJY_TraversalComponent;

UCLASS(ClassGroup = (JY), meta = (BlueprintSpawnableComponent))
class UJY_HangComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_HangComponent();

protected:
	virtual void BeginPlay() override;

public:
	bool CanMoveHang(FVector& OutMoveDirectionLedgeNormal) const;
	FRotator GetHangRotationFromLedgeNormal(const FVector& LedgeNormal) const;

	FORCEINLINE float GetMaxHangSpeed() const { return MaxHangSpeed; }
	FORCEINLINE float GetMaxHangAcceleration() const { return MaxHangAcceleration; }
	FORCEINLINE float GetHangFriction() const { return HangFriction; }
	FORCEINLINE float GetHangBrakingDeceleration() const { return HangBrakingDeceleration; }

protected:
	UPROPERTY(Transient)
	TObjectPtr<AJY_Character> OwnerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UJY_TraversalComponent> TraversalComponent;

	/* 매달림 좌우 최고 속도. */
	UPROPERTY(EditAnywhere, Category = "JY|Hang", meta = (ClampMin = "0", ForceUnits = "cm/s"))
	float MaxHangSpeed = 120.f;

	/* 매달림 가속도(작을수록 늦게 붙음) */
	UPROPERTY(EditAnywhere, Category = "JY|Hang", meta = (ClampMin = "0"))
	float MaxHangAcceleration = 240.f;

	/* 매달림 이동 마찰(방향전환 반응) */
	UPROPERTY(EditAnywhere, Category = "JY|Hang", meta = (ClampMin = "0"))
	float HangFriction = 8.f;

	/* 매달림 정지 감속 */
	UPROPERTY(EditAnywhere, Category = "JY|Hang", meta = (ClampMin = "0", ForceUnits = "cm/s^2"))
	float HangBrakingDeceleration = 2048.f;
};
