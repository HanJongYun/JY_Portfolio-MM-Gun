// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JY_ProneComponent.generated.h"

class AJY_Character;
class USkeletalMeshComponent;
class UAnimMontage;

/* 지면 정렬 트레이스  */
USTRUCT()
struct FJY_ProneGroundTraceSettings
{
	GENERATED_BODY()

	/* 앞쪽 트레이스 기준점 거리 */
	UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ForceUnits="cm"))
	float FrontTraceDistance = 70.f;

	/* 뒤쪽 트레이스 기준점 거리 */
	UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ForceUnits="cm"))
	float RearTraceDistance = 50.f;

	/* 트레이스 시작 높이 */
	UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ForceUnits="cm"))
	float TraceUpDistance = 50.f;

	/* 트레이스 종료 거리 */
	UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ForceUnits="cm"))
	float TraceDownDistance = 500.f;

	/* 좌우 트레이스 기준점 거리, Roll 계산용 */
	UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ForceUnits="cm"))
	float SideTraceDistance = 25.f;

	/* 최대 Pitch 제한 */
	UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ClampMax="90.0", ForceUnits="deg"))
	float MaxPitch = 45.f;

	/* 최대 Roll 제한 */
	UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ClampMax="90.0", ForceUnits="deg"))
	float MaxRoll = 45.f;

	/* pelvis 본과 지면 사이 목표 수직 거리 */
	UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ForceUnits="cm"))
	float PelvisGroundDistance = 39.f;

	/* Pitch/Roll 보간 속도 */
	UPROPERTY(EditAnywhere, meta=(ClampMin="0.0"))
	float AngleInterpSpeed = 8.f;

	/* 높이 보간 속도 */
	UPROPERTY(EditAnywhere, meta=(ClampMin="0.0"))
	float HeightInterpSpeed = 8.f;

};

USTRUCT(BlueprintType)
struct FJY_ProneAnimInput
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float GroundPitch = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float GroundRoll = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float GroundHeightOffset = 0.f;
};

UCLASS(ClassGroup = (JY), meta = (BlueprintSpawnableComponent))
class UJY_ProneComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_ProneComponent();

public:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void OnProneStarted();
	void OnProneEnded();

	FORCEINLINE float GetProneGroundPitch() const { return ProneInput.GroundPitch; }
	FORCEINLINE float GetProneGroundRoll() const { return ProneInput.GroundRoll; }
	FORCEINLINE float GetProneGroundHeightOffset() const { return ProneInput.GroundHeightOffset; }

	void StartProneTurn(float TargetYaw, bool bTurnRight);
	bool IsPlayingProneTurn() const;
	void SetProneBodyCollisionActive(bool bActive);

protected:
// 지면 정렬
	/* 트레이스로 목표 Pitch/Roll/HeightOffset 갱신 */
	void UpdateProneGroundAlignment();

	/* 아래로 라인트레이스해 바닥 찾기 */
	bool TraceProneGroundPoint(const FVector& TraceOrigin, FHitResult& OutHit) const;

	/* 앞/뒤 Hit으로 Pitch 계산 */
	float ComputeProneGroundPitch(const FVector& FrontImpact, const FVector& RearImpact) const;

	/* 좌/우 Hit으로 Roll 계산 */
	float ComputeProneGroundRoll(const FVector& LeftImpact, const FVector& RightImpact) const;

	/* pelvis 목표 높이 오프셋 계산 */
	float ComputeProneGroundHeightOffset(const FVector& FrontImpact, const FVector& RearImpact, const FVector& PelvisWorldLocation, const FVector& ActorLocation, const FVector& Forward2D) const;

	/* 지면 트레이스 갱신 */
	void UpdateProneTraceTargets();

	/* 목표값 보간해 최종값 갱신 */
	void LerpGroundAlignment(float DeltaTime);

	/* 이동/회전 없으면 트레이스 스킵 판정, PrevYaw 갱신 */
	bool ShouldSkipProneTrace(float DeltaTime);

// 제자리 턴
	/* 목표 Yaw로 돌면 몸이 벽 뚫는지 검사 */
	bool WouldProneBodyClipWall(float TargetYaw) const;

protected:
	UPROPERTY(Transient)
	TObjectPtr<AJY_Character> OwnerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> OwnerMesh;

// 지면 정렬
	/* 지면 정렬 트레이스/보간 튜닝값 */
	UPROPERTY(EditAnywhere, Category="JY|Prone|Ground Alignment")
	FJY_ProneGroundTraceSettings GroundTraceSettings;

	/* 목표 Pitch, 스무딩 전 원값 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="JY|Prone|Ground Alignment")
	float TargetProneGroundPitch = 0.f;

	/* 목표 Roll, 스무딩 전 원값 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="JY|Prone|Ground Alignment")
	float TargetProneGroundRoll = 0.f;

	/* 목표 높이 오프셋, 스무딩 전 원값 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="JY|Prone|Ground Alignment")
	float TargetProneGroundHeightOffset = 0.f;

	/* AnimInstance가 읽어가는 최종 출력값 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="JY|Prone|Anim Input")
	FJY_ProneAnimInput ProneInput;

	/* 정지 판정 수평 속도 임계 */
	UPROPERTY(EditAnywhere, Category = "JY|Prone|Ground Alignment", meta = (ClampMin = "0.0", ForceUnits = "cm/s"))
	float ProneMoveSpeedEps = 2.f;

	/* 정지 판정 각속도 임계 */
	UPROPERTY(EditAnywhere, Category = "JY|Prone|Ground Alignment", meta = (ClampMin = "0.0", ForceUnits = "deg/s"))
	float ProneYawRateEps = 5.f;

	/* 각속도 계산용 직전 Yaw */
	float PrevYaw = 0.f;

// 제자리 턴
	/* 왼쪽 제자리 턴 몽타주 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JY|Prone|Turn", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimMontage> ProneTurnLeftMontage;

	/* 오른쪽 제자리 턴 몽타주 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JY|Prone|Turn", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimMontage> ProneTurnRightMontage;

	/* 벽/바닥 구분 임계값 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JY|Prone|Turn", meta=(AllowPrivateAccess="true", ClampMin="0.0", ClampMax="1.0"))
	float ProneTurnWallNormalZThreshold = 0.7f;
};
