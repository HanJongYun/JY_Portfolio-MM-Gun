// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayPrediction.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "JY_Types.generated.h"

class UPrimitiveComponent;
class UAnimMontage;
class UJY_WeaponInstance;

UENUM(BlueprintType)
enum class EJY_WeaponAnimType : uint8
{
	Unarmed,
	SMG,
	Pistol,
	Rifle_Monster
};

UENUM(BlueprintType)
enum class EJY_WeaponFireMode : uint8
{
	Hitscan,
	Ballistic
};

USTRUCT(BlueprintType)
struct FJY_BallisticSettings
{
	GENERATED_BODY()

	/* 발사 초기 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1.0", Units = "cm/s"))
	float InitialSpeed = 30000.f;

	/* 수직 가속도(0이면 낙차 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "cm/s^2"))
	float GravityZ = -980.f;

	/* 속력 제곱에 비례하는 공기 저항 계수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float DragScale = 0.00001f;

	/* 충돌 검사 구체 반지름(0이면 라인 검사) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", Units = "cm"))
	float ProjectileRadius = 1.f;

	/* 최대 비행 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", Units = "s"))
	float MaxFlightTime = 3.f;

	/* 최대 누적 이동 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1.0", Units = "cm"))
	float MaxRange = 10000.f;
};

/* 비행 중인 가상 총알 한 발의 현재 상태 */
struct FJY_BallisticShot
{
	/* 발사자 내 총알 구분 번호 */
	uint32 ShotID = 0;

	/* 누적 시뮬레이션 시간 */
	float ElapsedTime = 0.f;

	/* 누적 이동 거리 */
	float TraveledDistance = 0.f;

	/* 충돌 검사 제외 액터(발사자/부착 장비) */
	TArray<TWeakObjectPtr<AActor>> IgnoredActors;

	/* 관통 판정을 처리한 표면 수 */
	int32 ProcessedSurfaceCount = 0;

	/* 이미 처리한 히트 컴포넌트(중복 차감 방지) */
	TArray<TWeakObjectPtr<UPrimitiveComponent>> IgnoredHitComponents;

	/* 예측 연출/서버 멀티캐스트 중복 방지 키 */
	FPredictionKey PredictionKey;

	/* 시뮬레이션 구간 시작 위치 */
	FVector Position = FVector::ZeroVector;

	/* 진행 방향/속력을 가진 속도 */
	FVector Velocity = FVector::ZeroVector;

	/* 발사 순간 복사한 설정(비행 중 불변) */
	FJY_BallisticSettings Settings;

	/* 발사자 */
	TWeakObjectPtr<AActor> Shooter;

	/* 발사에 사용한 무기 인스턴스 */
	TWeakObjectPtr<UJY_WeaponInstance> WeaponInstance;

	/* 남은 관통력 */
	float RemainingPenetrationPower = 0.f;

	/* 발사 순간 복사한 재질별 관통 비용 */
	TMap<TEnumAsByte<EPhysicalSurface>, float> PenetrationCostBySurface;
};

/* 무기 오버레이 상체 포즈 종류 */
UENUM(BlueprintType)
enum class EJY_OverlayPoseType : uint8
{
	StandIdle,
	StandMove,
	CrouchIdle,
	Combat,
	Aim
};

/* 제자리 회전 방향 */
UENUM(BlueprintType)
enum class EJY_TurnDirection : uint8
{
	None,
	Left,
	Right
};

/* 피격 방향 */
UENUM(BlueprintType)
enum class EJY_HitReactDirection : uint8
{
	Front,
	Back,
	Left,
	Right
};

USTRUCT(BlueprintType)
struct FJY_TraversalCheckResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool HasFrontLedge = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector FrontLedgeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector FrontLedgeNormal = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool HasBackLedge = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector BackLedgeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector BackLedgeNormal = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool HasBackFloor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector BackFloorLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ObstacleHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ObstacleDepth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BackLedgeHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UPrimitiveComponent> HitComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimMontage> ChosenMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StartTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PlayRate = 1.f;
};

