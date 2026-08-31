#include "Equipment/JY_WeaponInstance.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Actors/JY_WeaponActor.h"
#include "Data/JY_AttributeSet.h"
#include "Data/JY_WeaponData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_WeaponInstance)

UJY_WeaponInstance::UJY_WeaponInstance()
{
}

/* [Server / Owning Client / Simulated Proxy] */
void UJY_WeaponInstance::OnEquipped() //override
{
	Super::OnEquipped();

	/* Instigator가 이 무기를 만든 WeaponActor라 거기서 WeaponData를 얻어 캐싱 */
	if (AJY_WeaponActor* WeaponActor = Cast<AJY_WeaponActor>(GetInstigator()))
	{
		if (UJY_WeaponData* Data = WeaponActor->GetWeaponData())
		{
			SpreadExponent = Data->SpreadExponent;
			HeatToSpreadCurve = Data->HeatToSpreadCurve;
			HeatToHeatPerShotCurve = Data->HeatToHeatPerShotCurve;
			HeatToCoolDownPerSecondCurve = Data->HeatToCoolDownPerSecondCurve;
			SpreadRecoveryCooldownDelay = Data->SpreadRecoveryCooldownDelay;
			bAllowFirstShotAccuracy = Data->bAllowFirstShotAccuracy;

			/* 기본 데미지를 ASC에 셋팅, Execution이 발사 시 Source 어트리뷰트로 캡처 */
			if (APawn* OwningPawn = GetPawn())
			{
				if (IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(OwningPawn))
				{
					if (UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface->GetAbilitySystemComponent())
					{
						AbilitySystemComponent->ApplyModToAttribute(UJY_AttributeSet::GetBaseDamageAttribute(), EGameplayModOp::Override, Data->BaseDamage);
					}
				}
			}
		}
	}

	/* 커브 범위의 중간 열에서 시작, 이후 회복 단계에서 최소 열까지 내려감 */
	float MinHeat;
	float MaxHeat;
	ComputeHeatRange(OUT MinHeat, OUT MaxHeat);
	CurrentHeat = (MinHeat + MaxHeat) * 0.5f;
	CurrentSpreadAngle = HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat);
}

void UJY_WeaponInstance::AddSpread()
{
	const float HeatPerShot = HeatToHeatPerShotCurve.GetRichCurveConst()->Eval(CurrentHeat);
	CurrentHeat = ClampHeat(CurrentHeat + HeatPerShot);
	CurrentSpreadAngle = HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat);

	UWorld* World = GetWorld();
	if (World != nullptr)
	{
		TimeLastSpreadAdded = World->GetTimeSeconds();
	}
}

/* [Server / Owning Client] */
void UJY_WeaponInstance::Tick(float DeltaSeconds)
{
	/* 자세별 배율은 아직 없어 기본 확산이 최소인지로만 첫발 정확도를 판정 */
	const bool bMinSpread = UpdateSpread(DeltaSeconds);
	bHasFirstShotAccuracy = bAllowFirstShotAccuracy && bMinSpread;
}

void UJY_WeaponInstance::ComputeHeatRange(float& OutMinHeat, float& OutMaxHeat) const
{
	float HeatPerShotMin;
	float HeatPerShotMax;
	HeatToHeatPerShotCurve.GetRichCurveConst()->GetTimeRange(OUT HeatPerShotMin, OUT HeatPerShotMax);

	float CoolDownMin;
	float CoolDownMax;
	HeatToCoolDownPerSecondCurve.GetRichCurveConst()->GetTimeRange(OUT CoolDownMin, OUT CoolDownMax);

	float SpreadMin;
	float SpreadMax;
	HeatToSpreadCurve.GetRichCurveConst()->GetTimeRange(OUT SpreadMin, OUT SpreadMax);

	OutMinHeat = FMath::Min3(HeatPerShotMin, CoolDownMin, SpreadMin);
	OutMaxHeat = FMath::Max3(HeatPerShotMax, CoolDownMax, SpreadMax);
}

void UJY_WeaponInstance::ComputeSpreadRange(float& OutMinSpread, float& OutMaxSpread) const
{
	HeatToSpreadCurve.GetRichCurveConst()->GetValueRange(OUT OutMinSpread, OUT OutMaxSpread);
}

float UJY_WeaponInstance::ClampHeat(float NewHeat) const
{
	float MinHeat;
	float MaxHeat;
	ComputeHeatRange(OUT MinHeat, OUT MaxHeat);
	return FMath::Clamp(NewHeat, MinHeat, MaxHeat);
}

bool UJY_WeaponInstance::UpdateSpread(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
		return false;

	/* 장착 직후 또는 마지막 발사 후 지정된 대기시간이 지난 뒤에만 열 감소 */
	const double TimeSinceSpreadAdded = TimeLastSpreadAdded > 0.0 ? World->GetTimeSeconds() - TimeLastSpreadAdded : BIG_NUMBER;
	if (TimeSinceSpreadAdded > SpreadRecoveryCooldownDelay)
	{
		const float CoolDownPerSecond = HeatToCoolDownPerSecondCurve.GetRichCurveConst()->Eval(CurrentHeat);
		CurrentHeat = ClampHeat(CurrentHeat - CoolDownPerSecond * DeltaSeconds);
		CurrentSpreadAngle = HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat);
	}

	float MinSpread;
	float MaxSpread;
	ComputeSpreadRange(OUT MinSpread, OUT MaxSpread);
	return FMath::IsNearlyEqual(CurrentSpreadAngle, MinSpread, KINDA_SMALL_NUMBER);
}
