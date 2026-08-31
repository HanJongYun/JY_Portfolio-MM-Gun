#pragma once

#include "Core/JY_Types.h"

class AActor;
struct FHitResult;

namespace JY_WeaponUtils
{
	/* 명중 대상에 무기 데미지 GE 적용, 반환값은 실제 적용 여부(명중 마커 확인용) */
	bool ApplyWeaponDamage(AActor* ShooterActor, AActor* EffectCauser, const FHitResult& HitResult, int32 AbilityLevel = 1);

	/* 공격자 위치 기준 피격 방향(전/후/좌/우) 판정, Character/Monster HandleHealthChanged가 공용 사용 */
	EJY_HitReactDirection CalculateHitReactDirection(const FVector& SelfLocation, const FVector& SelfForward, const FVector& InstigatorLocation);

	/* 표면 하나의 관통 비용을 깎고, 소진되거나 처리 표면 수가 MaxSurfaceCount(기본 8)에 닿으면 true(정지) 반환.
	   히트스캔/탄도가 공용으로 쓰는 관통 판정 헬퍼 */
	bool ApplyPenetrationCost(float PenetrationCost, float& InOutRemainingPower, int32 ProcessedSurfaceCount, int32 MaxSurfaceCount = 8);

	/* 조준 방향 주변 확산 원뿔 안에서 한 발의 방향을 선택 */
	FVector GetRandomDirectionInSpreadCone(const FVector& Direction, float ConeHalfAngleRadians, float SpreadExponent);
}
