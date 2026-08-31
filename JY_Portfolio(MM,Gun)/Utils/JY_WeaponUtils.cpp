#include "Utils/JY_WeaponUtils.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Core/JY.h"
#include "Core/JY_GlobalSettings.h"
#include "Data/JY_WeaponGlobalData.h"
#include "GameFramework/Actor.h"

/* [Server] 명중 대상에 무기 데미지 GE 적용 */
bool JY_WeaponUtils::ApplyWeaponDamage(AActor* ShooterActor, AActor* EffectCauser, const FHitResult& HitResult, int32 AbilityLevel)
{
	IAbilitySystemInterface* SourceInterface = Cast<IAbilitySystemInterface>(ShooterActor);
	UAbilitySystemComponent* SourceAbilitySystemComponent = SourceInterface != nullptr ? SourceInterface->GetAbilitySystemComponent() : nullptr;
	if (SourceAbilitySystemComponent == nullptr)
	{
		return false;
	}

	IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(HitResult.GetActor());
	UAbilitySystemComponent* TargetAbilitySystemComponent = TargetInterface != nullptr ? TargetInterface->GetAbilitySystemComponent() : nullptr;
	if (TargetAbilitySystemComponent == nullptr)
	{
		return false;
	}

	const UJY_GlobalSettings* Settings = GetDefault<UJY_GlobalSettings>();
	UJY_WeaponGlobalData* GlobalData = Settings != nullptr ? Settings->WeaponGlobalData.LoadSynchronous() : nullptr;
	const TSubclassOf<UGameplayEffect> DamageGameplayEffectClass = GlobalData != nullptr ? GlobalData->DamageGameplayEffectClass : nullptr;
	if (DamageGameplayEffectClass == nullptr)
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceAbilitySystemComponent->MakeEffectContext();
	EffectContext.AddHitResult(HitResult);
	EffectContext.AddInstigator(ShooterActor, EffectCauser);

	FGameplayEffectSpecHandle SpecHandle = SourceAbilitySystemComponent->MakeOutgoingSpec(DamageGameplayEffectClass, AbilityLevel, EffectContext);
	if (SpecHandle.IsValid() == false)
	{
		return false;
	}

	SourceAbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetAbilitySystemComponent);
	return true;
}

EJY_HitReactDirection JY_WeaponUtils::CalculateHitReactDirection(const FVector& SelfLocation, const FVector& SelfForward, const FVector& InstigatorLocation)
{
	const FVector ToInstigator = (InstigatorLocation - SelfLocation).GetSafeNormal2D();

	const float ForwardCosAngle = FVector::DotProduct(SelfForward, ToInstigator);
	float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(ForwardCosAngle));
	if (FVector::CrossProduct(SelfForward, ToInstigator).Z < 0.f)
	{
		AngleDeg *= -1.f;
	}

	EJY_HitReactDirection Direction = EJY_HitReactDirection::Front;
	if (AngleDeg > 135.f || AngleDeg < -135.f)
	{
		Direction = EJY_HitReactDirection::Back;
	}
	else if (AngleDeg > 45.f)
	{
		Direction = EJY_HitReactDirection::Right;
	}
	else if (AngleDeg < -45.f)
	{
		Direction = EJY_HitReactDirection::Left;
	}

	return Direction;
}

bool JY_WeaponUtils::ApplyPenetrationCost(float PenetrationCost, float& InOutRemainingPower, int32 ProcessedSurfaceCount, int32 MaxSurfaceCount)
{
	InOutRemainingPower -= PenetrationCost;
	return InOutRemainingPower <= 0.f || ProcessedSurfaceCount >= MaxSurfaceCount;
}

FVector JY_WeaponUtils::GetRandomDirectionInSpreadCone(const FVector& Direction, float ConeHalfAngleRadians, float SpreadExponent)
{
	if (ConeHalfAngleRadians <= 0.f)
		return Direction.GetSafeNormal();

	const float ConeHalfAngleDegrees = FMath::RadiansToDegrees(ConeHalfAngleRadians);
	const float AngleFromCenter = FMath::Pow(FMath::FRand(), SpreadExponent) * ConeHalfAngleDegrees;
	const float AngleAroundCenter = FMath::FRand() * 360.f;

	const FQuat DirectionRotation(Direction.Rotation());
	const FQuat FromCenterRotation(FRotator(0.f, AngleFromCenter, 0.f));
	const FQuat AroundCenterRotation(FRotator(0.f, 0.f, AngleAroundCenter));
	FQuat SpreadRotation = DirectionRotation * AroundCenterRotation * FromCenterRotation;
	SpreadRotation.Normalize();
	return SpreadRotation.RotateVector(FVector::ForwardVector);
}
