#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/JY_Types.h"
#include "JY_AimComponent.generated.h"

class ACharacter;

UCLASS(ClassGroup = (JY), meta = (BlueprintSpawnableComponent))
class UJY_AimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_AimComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void StartAim();
	void StopAim();

	UFUNCTION(BlueprintPure, Category = "JY|Aim")
	bool IsAiming() const { return bWantsToAim; }

	UFUNCTION(BlueprintPure, Category = "JY|Aim")
	float GetAimPitch() const { return AimPitch; }

	UFUNCTION(BlueprintPure, Category = "JY|Aim")
	EJY_TurnDirection GetTurnDirection() const { return TurnDirection; }

	UFUNCTION(BlueprintPure, Category = "JY|Aim")
	bool IsWallAhead() const { return bWallAhead; }

protected:
	void UpdateAimRotation(float DeltaTime);
	void UpdateAimCamera(float DeltaTime);
	void UpdateWallAhead(const FRotator& AimRotation);

	UFUNCTION(Server, Reliable)
	void Server_SetWantsToAim(bool bNewWantsToAim);

	UFUNCTION(Server, Reliable)
	void Server_SetTurnDirection(EJY_TurnDirection InTurnDirection);

	EJY_TurnDirection CalculateTurnDirection(float YawAngle) const;

	ACharacter* GetOwnerCharacter() const;

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "JY|Aim", meta = (AllowPrivateAccess = "true"))
	bool bWantsToAim = false;

	/* 각 머신이 로컬 계산 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Aim", meta = (AllowPrivateAccess = "true"))
	float AimPitch = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "JY|Aim", meta = (AllowPrivateAccess = "true"))
	EJY_TurnDirection TurnDirection = EJY_TurnDirection::None;

	UPROPERTY(EditAnywhere, Category = "JY|Aim")
	FName MeshRootSocketName = TEXT("root");

	/* 메쉬 전방축 보정(도) */
	UPROPERTY(EditAnywhere, Category = "JY|Aim")
	float MeshForwardYawCorrection = 90.f;

	UPROPERTY(EditAnywhere, Category = "JY|Aim")
	float AimRotationInterpSpeed = 25.f;

	UPROPERTY(EditAnywhere, Category = "JY|Aim|Camera", meta = (ClampMin = "0.0"))
	float AimCameraTargetArmLength = 250.f;

	UPROPERTY(EditAnywhere, Category = "JY|Aim|Camera", meta = (ClampMin = "5.0", ClampMax = "170.0"))
	float AimCameraTargetFieldOfView = 70.f;

	UPROPERTY(EditAnywhere, Category = "JY|Aim|Camera")
	FVector AimCameraTargetSocketOffset = FVector(0.f, 75.f, 65.f);

	UPROPERTY(EditAnywhere, Category = "JY|Aim|Camera", meta = (ClampMin = "0.0"))
	float AimCameraInterpSpeed = 10.f;

	/* 에임오프셋 축 범위와 맞춤 */
	UPROPERTY(EditAnywhere, Category = "JY|Aim")
	float AimAngleClamp = 90.f;

	UPROPERTY(EditAnywhere, Category = "JY|Aim|Wall", meta = (ClampMin = "0.0", Units = "cm"))
	float WallCheckDistance = 120.f;

private:
	float DefaultCameraArmLength = 0.f;
	float DefaultCameraFieldOfView = 0.f;
	FVector DefaultCameraSocketOffset = FVector::ZeroVector;
	bool bWallAhead = false;
};
