// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actors/JY_BaseCharacter.h"
#include "Engine/TimerHandle.h"
#include "Logging/LogMacros.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayPrediction.h"
#include "Core/JY_Types.h"
#include "JY_Character.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UJY_CharacterMovementComponent;
class UJY_ProneComponent;
class UJY_HangComponent;
class UAnimMontage;
class UCapsuleComponent;
class UMotionWarpingComponent;
class UJY_HeroComponent;
class UJY_EquipmentComponent;
class UJY_AimComponent;
class UAbilitySystemComponent;
struct FGameplayEffectSpec;

UCLASS(abstract)
class AJY_Character : public AJY_BaseCharacter
{
	GENERATED_BODY()

public:
	AJY_Character(const FObjectInitializer& ObjectInitializer);

public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE UJY_CharacterMovementComponent* GetJYCharacterMovement() const { return JYMovementComponent; }
	FORCEINLINE UJY_AimComponent* GetAimComponent() const { return AimComponent; }
	FORCEINLINE UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }
	FORCEINLINE class UCapsuleComponent* GetProneBodyCapsule() const { return ProneBodyCapsule; }
	FORCEINLINE UJY_ProneComponent* GetProneComponent() const { return ProneComponent; }
	FORCEINLINE UJY_HangComponent* GetHangComponent() const { return HangComponent; }
	FORCEINLINE UAnimMontage* GetProneMontage() const { return ProneMontage; }

	FORCEINLINE bool IsFullyHanging() const { return bIsHanging && bFullyHanging; }
	FORCEINLINE void SetFullyHanging(bool bValue) { bFullyHanging = bValue; }

	FORCEINLINE bool IsProne() const { return bIsProne; }
	UFUNCTION(BlueprintPure, Category = "JY|Prone")
	FORCEINLINE bool IsFullyProne() const { return bIsProne && bIsFullyProne; }
	FORCEINLINE void SetFullyProne(bool bValue) { bIsFullyProne = bValue; }

#pragma region Components 관련 =======================================================

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="JY|Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="JY|Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="JY|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UJY_ProneComponent> ProneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="JY|Traversal", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UJY_HangComponent> HangComponent;

	UPROPERTY(Transient)
	TObjectPtr<UJY_CharacterMovementComponent> JYMovementComponent;

#pragma endregion Components 관련 ===================================================


#pragma region GAS 관련 =============================================================
protected:
	void InitializeAbilitySystem();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="JY|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UJY_HeroComponent> HeroComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="JY|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UJY_AimComponent> AimComponent;

#pragma endregion GAS 관련 ==========================================================


#pragma region Traversal 관련 ============================================================

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="JY|Traversal")
	void TryJumpOrTraversal();
	virtual void TryJumpOrTraversal_Implementation();

#pragma endregion Traversal 관련 ========================================================


#pragma region Combat 관련 ===========================================================

protected:
	void HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float Magnitude, float NewValue);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRequestHitReact(EJY_HitReactDirection Direction);

#pragma endregion Combat 관련 =======================================================


#pragma region Hang 관련 =============================================================
public:
	UFUNCTION(BlueprintCallable, Category="JY|Traversal")
	void SetWantsToHang(bool bNewWantsToHang);

	UFUNCTION(BlueprintPure, Category="JY|Traversal")
	bool GetIsHanging() const;

	void SetIsHanging(bool bNewHanging);

	UFUNCTION()
	void OnRep_IsHanging();

	UFUNCTION(BlueprintPure, Category="JY|Animation")
	float GetAnimCurveValue(FName CurveName) const;

protected:

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_IsHanging, Category="JY|Traversal", meta=(AllowPrivateAccess="true"))
	uint8 bIsHanging : 1;

	UPROPERTY(BlueprintReadOnly, Transient, Category="JY|Traversal", meta=(AllowPrivateAccess="true"))
	uint8 bFullyHanging : 1;

#pragma endregion Hang 관련 =========================================================


#pragma region Prone 관련 ============================================================
public:
	UFUNCTION(BlueprintPure, Category="JY|Prone")
	bool WantsToProne() const;

	UFUNCTION(BlueprintCallable, Category = "JY|Prone")
	void Prone();

	UFUNCTION(BlueprintCallable, Category = "JY|Prone")
	void UnProne();

	void SetIsProne(bool bNewProne);

	UFUNCTION()
	virtual void OnRep_IsProne();

	virtual void OnStartProne(float HalfHeightAdjust, float ScaledHalfHeightAdjust);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnStartProne", ScriptName = "OnStartProne"))
	void K2_OnStartProne(float HalfHeightAdjust, float ScaledHalfHeightAdjust);

	virtual void OnEndProne(float HalfHeightAdjust, float ScaledHalfHeightAdjust);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnEndProne", ScriptName = "OnEndProne"))
	void K2_OnEndProne(float HalfHeightAdjust, float ScaledHalfHeightAdjust);
protected:

// [치트] V키 앞으로 텔레포트
	void CheatTeleportForward();

	// [Client -> Server RPC] 서버에서 실제 텔레포트 → 위치 복제로 전 클라 반영.
	UFUNCTION(Server, Reliable)
	void Server_CheatTeleportForward();
// ~[치트] =====================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JY|Prone", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimMontage> ProneMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JY|Prone", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimMontage> ExitProneMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="JY|Prone|Collision", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCapsuleComponent> ProneBodyCapsule;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsProne, Category="JY|Prone", meta=(AllowPrivateAccess="true"))
	uint8 bIsProne : 1;

	UPROPERTY(BlueprintReadOnly, Transient, Category="JY|Prone", meta=(AllowPrivateAccess="true"))
	uint8 bIsFullyProne : 1;

	UPROPERTY(Transient)
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

#pragma endregion Prone 관련 ========================================================
};
