#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/JY_Types.h"
#include "JY_WeaponActor.generated.h"

class USkeletalMeshComponent;
class UJY_WeaponData;
class AJY_WeaponFireActor;

UCLASS()
class AJY_WeaponActor : public AActor
{
	GENERATED_BODY()

public:
	AJY_WeaponActor();

protected:
	virtual void BeginPlay() override;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeFromType(EJY_WeaponAnimType InType);

	EJY_WeaponAnimType GetWeaponType() const { return WeaponType; }
	UJY_WeaponData* GetWeaponData() const { return CachedData; }
	USkeletalMeshComponent* GetMeshComponent() const { return MeshComp; }

	bool TryGetMuzzleTransform(FTransform& OutMuzzleTransform) const;
	void PlayFireEffects(const TArray<FVector>& ImpactPositions);

protected:
	UFUNCTION()
	void OnRep_WeaponType();

private:
	void ApplyWeaponType();

protected:
	UPROPERTY(VisibleAnywhere, Category = "JY|Weapon")
	TObjectPtr<USkeletalMeshComponent> MeshComp;

	/** 종류만 복제 — 데이터·메시는 각 머신이 전역에서 조회 */
	UPROPERTY(ReplicatedUsing = OnRep_WeaponType)
	EJY_WeaponAnimType WeaponType = EJY_WeaponAnimType::Unarmed;

private:
	UPROPERTY(Transient)
	TObjectPtr<UJY_WeaponData> CachedData;

	UPROPERTY(Transient)
	TObjectPtr<AJY_WeaponFireActor> WeaponFire;
};
