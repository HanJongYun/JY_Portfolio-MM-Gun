// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actors/JY_BaseCharacter.h"
#include "Engine/EngineTypes.h"
#include "Core/JY_Types.h"
#include "JY_Monster.generated.h"

class UJY_AbilitySystemComponent;
class UJY_AttributeSet;
struct FGameplayEffectSpec;
class UJY_EquipmentComponent;

/** 사망 렉돌 발사·유지에 쓰는 튜닝값 묶음. */
USTRUCT(BlueprintType)
struct FJY_MonsterRagdollSettings
{
	GENERATED_BODY()

	/** 사망 렉돌 유지 후 액터 파괴까지 대기 시간(초) */
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0, Units = "s"))
	float DeathDuration = 5.0f;

	/** 렉돌 발사 임펄스 세기(사망 전용). */
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0))
	float LaunchImpulseStrength = 500.f;

	/** 렉돌 발사 각도 비율(수직/수평). 1.0=45도(최대 사거리), 낮을수록 납작하게 미끄러지듯 날아간다. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0))
	float LaunchVerticalRatio = 0.7f;
};

UCLASS()
class AJY_Monster : public AJY_BaseCharacter
{
	GENERATED_BODY()

public:
	AJY_Monster();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;

public:
	/** IAbilitySystemInterface, 몬스터 ASC는 자기 자신이 소유한다 */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	const TArray<TObjectPtr<AActor>>& GetPatrolPoints() const { return PatrolPoints; }

	/** 렉돌을 켜고 Impulse 방향으로 날린다. */
	UFUNCTION(BlueprintCallable, Category = "JY|Ragdoll")
	void StartRagdoll(const FVector& Impulse);

protected:
	void HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float Magnitude, float NewValue);
	void HandleDeath(AActor* DamageInstigator);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRequestHitReact(EJY_HitReactDirection Direction);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartRagdoll(FVector Impulse, bool bIsDeath);

	void FollowRagdoll();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "JY|Combat")
	EJY_WeaponAnimType InitialWeaponType = EJY_WeaponAnimType::Pistol;

	UPROPERTY(EditInstanceOnly, Category = "JY|AI|Patrol")
	TArray<TObjectPtr<AActor>> PatrolPoints;

	UPROPERTY(EditAnywhere, Category = "JY|Ragdoll")
	FName PelvisBone = TEXT("pelvis");

	UPROPERTY(EditAnywhere, Category = "JY|Ragdoll")
	FJY_MonsterRagdollSettings RagdollSettings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JY|Ragdoll")
	bool bIsRagdoll = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JY|Ragdoll")
	bool bIsDead = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JY|Ragdoll")
	bool bFacingUp = false;

private:
	UPROPERTY(VisibleAnywhere, Category = "JY|Combat")
	TObjectPtr<UJY_AbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "JY|Combat")
	TObjectPtr<UJY_AttributeSet> AttributeSet;
};
