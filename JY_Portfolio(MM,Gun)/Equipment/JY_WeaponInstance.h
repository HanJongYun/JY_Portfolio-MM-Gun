#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Equipment/JY_EquipmentInstance.h"
#include "JY_WeaponInstance.generated.h"

UCLASS(BlueprintType, Blueprintable)
class UJY_WeaponInstance : public UJY_EquipmentInstance
{
	GENERATED_BODY()

public:
	UJY_WeaponInstance();

public:
	void Tick(float DeltaSeconds);

	virtual void OnEquipped() override;

	void AddSpread();
	UFUNCTION(BlueprintPure, Category = "JY|Weapon|Spread")
	float GetCurrentHeat() const { return CurrentHeat; }

	UFUNCTION(BlueprintPure, Category = "JY|Weapon|Spread")
	float GetCalculatedSpreadAngle() const { return bHasFirstShotAccuracy ? 0.f : CurrentSpreadAngle; }

	UFUNCTION(BlueprintPure, Category = "JY|Weapon|Spread")
	bool HasFirstShotAccuracy() const { return bHasFirstShotAccuracy; }

	float GetSpreadExponent() const { return SpreadExponent; }

protected:
	UPROPERTY(Transient)
	float SpreadExponent = 0.8f;

	UPROPERTY(Transient)
	FRuntimeFloatCurve HeatToSpreadCurve;

	UPROPERTY(Transient)
	FRuntimeFloatCurve HeatToHeatPerShotCurve;

	UPROPERTY(Transient)
	FRuntimeFloatCurve HeatToCoolDownPerSecondCurve;

	UPROPERTY(Transient)
	float SpreadRecoveryCooldownDelay = 0.15f;

	UPROPERTY(Transient)
	bool bAllowFirstShotAccuracy = true;

private:
	/* 세 커브가 쓰는 전체 열 범위 */
	void ComputeHeatRange(float& OutMinHeat, float& OutMaxHeat) const;

	/* 확산 커브의 최소/최대 각도 */
	void ComputeSpreadRange(float& OutMinSpread, float& OutMaxSpread) const;

	/* 열을 커브 범위 안으로 제한 */
	float ClampHeat(float NewHeat) const;

	/* 발사하지 않는 동안 열 감소 */
	bool UpdateSpread(float DeltaSeconds);

private:
	/* 연사 중 누적되는 현재 총기 열 */
	float CurrentHeat = 0.f;

	/* 현재 열을 커브에 넣어 얻은 기본 확산 각도 */
	float CurrentSpreadAngle = 0.f;

	/* 다음 한 발 무작위 확산 제거 가능 상태 */
	bool bHasFirstShotAccuracy = false;

	/* 조종 클라이언트/서버가 각자 마지막으로 확산 열을 더한 시각 */
	double TimeLastSpreadAdded = 0.0;
};
