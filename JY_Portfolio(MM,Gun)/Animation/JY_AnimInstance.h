// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Core/JY_Types.h"
#include "JY_AnimInstance.generated.h"

class AJY_Character;
class USkeletalMeshComponent;
class UAnimSequence;

UCLASS()
class UJY_AnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/* 재생 중이어도 재호출 시 리트리거 */
	void RequestHitReact(EJY_HitReactDirection Direction);

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	void UpdateWeaponState(float DeltaSeconds);

	UFUNCTION(BlueprintPure, Category = "JY|Weapon", meta = (BlueprintThreadSafe))
	float GetLeftIKAlpha() const;

	void UpdateHitReact(float DeltaSeconds);

	void UpdateProneGroundAlignment();

	UFUNCTION(BlueprintPure, Category = "JY|Debug|Retarget", meta = (BlueprintThreadSafe))
	FVector GetSourceSocketLocation(FName SocketName) const;

	UFUNCTION(BlueprintPure, Category = "JY|Debug|Retarget", meta = (BlueprintThreadSafe))
	FVector GetTargetSocketLocation(FName SocketName) const;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "JY|Combat")
	TObjectPtr<UAnimSequence> HitReactSequence;

	UPROPERTY(BlueprintReadOnly, Category = "JY|Combat")
	float HitReactPlayTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "JY|Combat")
	bool bHitReacting = false;

protected:
	UPROPERTY(Transient)
	TObjectPtr<AJY_Character> JYCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "JY|Weapon")
	EJY_WeaponAnimType WeaponAnimType = EJY_WeaponAnimType::Unarmed;

	/* ABP 왼손 Two Bone IK effector(World Space) 바인딩 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Weapon")
	FVector LeftHandIKTargetLocation = FVector::ZeroVector;

	/* 무장 + 무기 소켓 존재 여부 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Weapon")
	bool bHasLeftHandIKTarget = false;

	/* 방향별 시퀀스, 캐릭터 BP 디폴트에서 채움 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Combat")
	TMap<EJY_HitReactDirection, TObjectPtr<UAnimSequence>> HitReactSequences;

	/* 새 히트 감지용 카운터 */
	int32 HitReactPlayCount = 0;

	/* HitReactPlayCount와 비교해 새 히트 판정 */
	int32 LastAppliedHitReactPlayCount = 0;

	/* AnimGraph pelvis 보정용 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Prone|Ground Alignment")
	float ProneGroundPitch = 0.f;

	/* AnimGraph pelvis 좌우 보정용 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Prone|Ground Alignment")
	float ProneGroundRoll = 0.f;

	/* AnimGraph pelvis Z 보정용 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Prone|Ground Alignment")
	float ProneGroundHeightOffset = 0.f;

	UPROPERTY(Transient)
	TWeakObjectPtr<USkeletalMeshComponent> CachedSourceMesh;

	UPROPERTY(Transient)
	TWeakObjectPtr<USkeletalMeshComponent> CachedSelfMesh;
};
