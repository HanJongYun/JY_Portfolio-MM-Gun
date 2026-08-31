#pragma once

#include "CoreMinimal.h"
#include "Core/JY_Types.h"
#include "Subsystems/WorldSubsystem.h"
#include "JY_BallisticSubsystem.generated.h"

UCLASS()
class UJY_BallisticSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	uint32 AddBallisticShot(FJY_BallisticShot Shot);
	int32 GetActiveShotCount() const { return ActiveShots.Num(); }

private:
	FVector CalculateAcceleration(const FVector& Velocity, const FJY_BallisticSettings& Settings) const;
	void IntegrateShotRK2(const FVector& Position, const FVector& Velocity, const FJY_BallisticSettings& Settings, float DeltaTime, FVector& OutPosition, FVector& OutVelocity) const;
	/* 로컬 임팩트 큐 재생, 데미지는 서버 보고에서 처리 */
	void HandleBallisticImpact(const FJY_BallisticShot& Shot, const FHitResult& Hit) const;

private:
	TArray<FJY_BallisticShot> ActiveShots;

	/* 0은 무효, 순환 시 1부터 재사용 */
	uint32 NextShotID = 1;

	float SimulationTimeAccumulator = 0.f;
};
