// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Core/JY_Types.h"
#include "JY_MonsterAnimInstance.generated.h"

class AJY_Monster;
class UAnimSequence;

UCLASS()
class UJY_MonsterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/* 재생 중이어도 재호출 시 리트리거 */
	void RequestHitReact(EJY_HitReactDirection Direction);

protected:
	virtual void NativeInitializeAnimation() override;
	/* Init 시점엔 폰이 없을 수 있어 재확인 */
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:
	/* AnimGraph SequenceEvaluator Sequence 핀 바인딩 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Combat")
	TObjectPtr<UAnimSequence> HitReactSequence;

	/* AnimGraph SequenceEvaluator ExplicitTime 핀 바인딩 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Combat")
	float HitReactPlayTime = 0.f;

	/* AnimGraph ApplyAdditive Alpha 핀 바인딩 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Combat")
	bool bHitReacting = false;

	/* BS_Idle_Walk_Run Speed(0~600) 축 바인딩 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Locomotion")
	float GroundSpeed = 0.f;

	/* BS_Idle_Walk_Run Direction 축 바인딩, RequestHitReact의 Direction과 이름 충돌 피함 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Locomotion")
	float MoveDirection = 0.f;

	/* Idle/Walk 전이 조건용 캐시 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Locomotion")
	bool bIsMoving = false;

	/* AO_Rifle X축 바인딩, 카메라 없어 ControlRotation 사용 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Aim")
	float AimYaw = 0.f;

	/* AO_Rifle Y축 바인딩 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Aim")
	float AimPitch = 0.f;

	/* AnimGraph Two Bone IK Effector Target(World Space) 바인딩 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Weapon")
	FVector LeftHandIKTargetLocation = FVector::ZeroVector;

	/* Two Bone IK Alpha 바인딩, 몬스터는 커브 없이 이 값만으로 게이팅 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Weapon")
	bool bHasLeftHandIKTarget = false;

protected:
	/* 초기화 시 1회 캐싱 */
	UPROPERTY(Transient)
	TObjectPtr<AJY_Monster> JYMonster;

	/* 방향별 시퀀스, BP 디폴트에서 채움 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Combat")
	TMap<EJY_HitReactDirection, TObjectPtr<UAnimSequence>> HitReactSequences;

	/* 새 히트 감지용 카운터, AnimGraph 미노출 */
	int32 HitReactPlayCount = 0;

	/* HitReactPlayCount와 비교해 새 히트 판정 */
	int32 LastAppliedHitReactPlayCount = 0;
};
