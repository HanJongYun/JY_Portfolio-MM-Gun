// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JY_CharacterMovementComponent.generated.h"

class AJY_Character;
class UJY_HangComponent;

/* MOVE_Custom일 때 CustomMovementMode에 들어가는 값 */
UENUM(BlueprintType)
enum class EJY_CustomMovementMode : uint8
{
	None = 0,
	Hang = 1,
};

UCLASS(ClassGroup = (JY), meta = (BlueprintSpawnableComponent))
class UJY_CharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UJY_CharacterMovementComponent();

protected:
	virtual void BeginPlay() override;


#pragma region Hang 관련 =============================================================
// 매달리기(Ledge Hang), 의도는 SavedMove 압축플래그로 전달
public:
	/* 매달림 시작 */
	void Hang(bool bClientSimulation = false);

	/* 매달림 종료 */
	void UnHang(bool bClientSimulation = false);

protected:
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

	/* 매달림 좌우 이동 */
	void PhysCustom_Hang(float DeltaTime, int32 Iterations);

#pragma endregion Hang 관련 =========================================================

#pragma region Prone 관련 ============================================================
public:

	/* 엎드림 실행, 상태 세팅 + 노티파이 */
	virtual void Prone(bool bClientSimulation = false);

	/* 엎드리기 해제 */
	virtual void UnProne(bool bClientSimulation = false);

	UFUNCTION(BlueprintPure, Category = "JY|Prone")
	bool IsProning() const;

	virtual bool CanCrouchInCurrentState() const override;
	virtual bool CanProneInCurrentState() const;

	/* 매 틱 의도 -> 상태 전이 */
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;

	/* 이동모드별 최대 속도, Prone 분기 추가 */
	virtual float GetMaxSpeed() const override;

	/* 매달림 중 전용 가속도 추가 */
	virtual float GetMaxAcceleration() const override;

	// SavedMove + 압축플래그로 prone/hang 의도 서버에 전달 
	/* 커스텀 SavedMove 사용 */
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	/* 압축플래그에서 prone/hang 의도 복원 */
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	/* 제자리 턴 감지, 트리거 전용 */
	void UpdateProneTurnState();

protected:

	// 벽 충돌, 앞으로 뻗은 몸이 벽 뚫는 것 방지
	/* CalcVelocity 직후 프론 벽 속도 제약 */
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

	/* 이동 후 몸 캡슐 벽 밖으로 밀어냄 */
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

	/* 벽 방향 속도 성분 제거 */
	void ConstrainProneVelocityForWalls(float DeltaSeconds);

	/* Prone캡슐 벽 겹침 MTD로 해소 */
	void DepenetrateProneBodyFromWalls();

#pragma endregion Prone 관련 ========================================================

protected:
	UPROPERTY(Transient)
	TObjectPtr<AJY_Character> JYCharacter;

public:
	// Hang 
	/* 매달림 입력 의도 */
	uint8 bWantsToHang : 1;

protected:
	UPROPERTY(Transient)
	TObjectPtr<UJY_HangComponent> JYHangComponent;

public:

	// Prone
	/* 엎드리기 입력 의도제 */
	uint8 bWantsToProne : 1;

	/* 엎드려 지상 이동 최대 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JY|Prone", meta = (ClampMin = 0, ForceUnits = "cm/s"))
	float MaxProneSpeed = 150.f;

	/* 엎드렸을 때 캡슐 half-height(언스케일) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JY|Prone", meta = (ClampMin = 0, ForceUnits = "cm"))
	float PronedHalfHeight = 40.f;

	/* 제자리 턴 시작 최소 입력각(도) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JY|Prone", meta = (ClampMin = 0, ClampMax = 180, ForceUnits = "deg"))
	float ProneTurnStartAngle = 70.f;

	/* FrontToFront 판정용 최소 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JY|Prone", meta = (ClampMin = 0, ForceUnits = "cm/s"))
	float ProneTurnMinSpeed = 10.f;

	/* 카메라 전방 콘 반각(도) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JY|Prone", meta = (ClampMin = 0, ClampMax = 180, ForceUnits = "deg"))
	float ProneTurnFrontConeAngle = 70.f;

protected:
	// Prone 벽 충돌 
	/* 마지막 유효 벽 노멀 */
	FVector CachedProneWallNormal = FVector::ZeroVector;

	/* 벽/바닥 구분 임계값 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JY|Prone")
	float ProneWallNormalZThreshold = 0.7f;

	/* 벽 감지 여유 거리, 닿기 전에 미리 감지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JY|Prone")
	float ProneWallSweepSkin = 2.f;
};


// 클라 예측 move에 prone/hang 의도를 싣기 위한 SavedMove 서브클래스

/* 한 프레임 move의 prone/hang 의도를 저장/직렬화 */
class FJY_SavedMove_Character : public FSavedMove_Character
{
	typedef FSavedMove_Character Super;

public:

	/* 이 move 시점 prone 의도 */
	uint8 bWantsToProne : 1;

	/* 이 move 시점 hang 의도 */
	uint8 bWantsToHang : 1;

	/* move 재사용 전 초기화 */
	virtual void Clear() override;

	/* CMC의 현재 의도를 이 move에 캡처 */
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;

	/* 의도를 압축플래그 커스텀 비트로 직렬화 */
	virtual uint8 GetCompressedFlags() const override;
};

/* move 풀, FJY_SavedMove_Character를 만들게 함 */
class FJY_NetworkPredictionData_Client_Character : public FNetworkPredictionData_Client_Character
{
	typedef FNetworkPredictionData_Client_Character Super;

public:

	FJY_NetworkPredictionData_Client_Character(const UCharacterMovementComponent& ClientMovement);

	/* 새 예측 move를 FJY_SavedMove_Character로 생성 */
	virtual FSavedMovePtr AllocateNewMove() override;
};
