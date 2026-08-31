#pragma once

#include "CoreMinimal.h"
#include "Abilities/JY_GameplayAbility_FromEquipment.h"
#include "GameplayTagContainer.h"
#include "JY_GameplayAbility_WeaponFire.generated.h"

class AJY_WeaponActor;
class UJY_WeaponInstance;

UCLASS()
class UJY_GameplayAbility_WeaponFire : public UJY_GameplayAbility_FromEquipment
{
	GENERATED_BODY()

public:
	UJY_GameplayAbility_WeaponFire(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	/* 조준 중 아니거나 전방 막힘 */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void HandleFireDelayFinished();

	/* 예측 타겟 데이터 생성 */
	void StartWeaponTargeting();
	void OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag);

	void ExecuteFireCue(const FGameplayAbilityTargetDataHandle& TargetData);
	void ExecuteImpactCues(const FGameplayAbilityTargetDataHandle& TargetData);

	/* 데미지 적용, 히트마커 적용 */
	void ApplyDamageToHits(const FGameplayAbilityTargetDataHandle& TargetData);

	AJY_WeaponActor* GetWeaponActor() const;
	FVector GetWeaponTargetingSourceLocation() const;
	/* 조준 시작점 조정 */
	FTransform GetTargetingTransform() const;

	/* 히트스캔 타겟 생성 */
	bool PerformHitscanTargeting(TArray<FHitResult>& OutHitResults) const;
	/* 관통 스윕 처리 */
	bool WeaponSweepWithSurfacePenetration(const FVector& StartTrace, const FVector& EndTrace, float TraceRadius, TArray<FHitResult>& OutHitResults) const;

	/* 탄도 시작 위치/방향만 생성 */
	bool PerformBallisticTargeting(TArray<FHitResult>& OutHitResults) const;
	/* 로컬 탄도 시뮬레이션 등록 */
	bool RegisterBallisticShot(const FGameplayAbilityTargetDataHandle& TargetData);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JY|Weapon Fire")
	float FireDelayTimeSecs;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JY|Weapon Fire", meta = (Categories = "GameplayCue"))
	FGameplayTag ImpactGameplayCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JY|Weapon Fire", meta = (Categories = "GameplayCue"))
	FGameplayTag FireGameplayCueTag;

private:
	FDelegateHandle TargetDataReadyCallbackDelegateHandle;
	FTimerHandle FireDelayTimerHandle;
};
