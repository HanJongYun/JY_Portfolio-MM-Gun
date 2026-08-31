#include "Data/JY_WeaponData.h"

UJY_WeaponData::UJY_WeaponData()
{
	/* 재질별 기본 관통 비용(총마다 애셋에서 조정). Default 999(정지)/Wood 5/Glass 3/Metal 10 */
	PenetrationCostBySurface.Add(SurfaceType_Default, 999.f);
	PenetrationCostBySurface.Add(SurfaceType1, 5.f);
	PenetrationCostBySurface.Add(SurfaceType2, 3.f);
	PenetrationCostBySurface.Add(SurfaceType3, 10.f);

	/* 확산 커브 기본값(총마다 애셋에서 조정) */
	FRichCurve* SpreadCurve = HeatToSpreadCurve.GetRichCurve();
	SpreadCurve->AddKey(0.f, 1.5f);
	SpreadCurve->AddKey(7.f, 5.f);
	SpreadCurve->AddKey(12.f, 9.f);

	FRichCurve* HeatPerShotCurve = HeatToHeatPerShotCurve.GetRichCurve();
	HeatPerShotCurve->AddKey(0.f, 0.6f);
	HeatPerShotCurve->AddKey(4.f, 0.6f);
	HeatPerShotCurve->AddKey(5.f, 0.75f);

	FRichCurve* CoolDownCurve = HeatToCoolDownPerSecondCurve.GetRichCurve();
	CoolDownCurve->AddKey(0.f, 8.f);
	CoolDownCurve->AddKey(12.f, 8.f);
}

float UJY_WeaponData::GetPenetrationCost(EPhysicalSurface SurfaceType) const
{
	/* 미등록 재질은 관통불가 */
	const float* FoundCost = PenetrationCostBySurface.Find(SurfaceType);
	return FoundCost != nullptr ? *FoundCost : TNumericLimits<float>::Max();
}
