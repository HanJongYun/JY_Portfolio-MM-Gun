#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/JY_Types.h"
#include "Curves/CurveFloat.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "JY_WeaponData.generated.h"

class USkeletalMesh;
class UStaticMesh;
class UNiagaraSystem;
class UAnimMontage;
class UAnimSequenceBase;
class UAimOffsetBlendSpace;
class UJY_AbilitySet;
class UJY_EquipmentInstance;
class USoundBase;

UCLASS(BlueprintType, Const)
class UJY_WeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UJY_WeaponData();

public:
	float GetPenetrationCost(EPhysicalSurface SurfaceType) const;

public:
	/* 장착 시 생성할 런타임 장비 객체 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Equipment")
	TSubclassOf<UJY_EquipmentInstance> InstanceType;

	/* 무기 메시 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon")
	TSoftObjectPtr<USkeletalMesh> WeaponMesh;

	/* 장착 손 소켓 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Socket")
	FName GripSocketName = TEXT("weapon_r");

	/* 왼손(보조손) IK 소켓 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Socket")
	FName LeftHandSocketName = TEXT("OffHandIK");

	/* 총구 소켓 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Socket")
	FName MuzzleSocketName = TEXT("Muzzle");

	/* 탄피 배출 소켓 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Socket")
	FName ShellEjectSocketName = TEXT("ShellEject");

	/* 수납 소켓 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Socket")
	FName HolsterSocketName = TEXT("HolsterPoint");

	/* 손 소켓 부착 상대 트랜스폼 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Socket")
	FTransform GripRelativeTransform;

	/* 수납 소켓 부착 상대 트랜스폼 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Socket")
	FTransform HolsterRelativeTransform;

	/* 장착 몽타주. 없으면 즉시 부착 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Anim")
	TObjectPtr<UAnimMontage> EquipMontage;

	/* 수납 몽타주 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Anim")
	TObjectPtr<UAnimMontage> UnequipMontage;

	/* 무기 오버레이 상체 포즈 맵(종류->포즈) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Anim|Overlay")
	TMap<EJY_OverlayPoseType, TObjectPtr<UAnimSequenceBase>> OverlayPoses;

	/* 조준 에임오프셋 블렌드스페이스 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Anim|Overlay")
	TObjectPtr<UAimOffsetBlendSpace> AimOffset;

	/* 장착 시 부여할 어빌리티 셋. 해제 시 회수 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Abilities")
	TObjectPtr<UJY_AbilitySet> AbilitySet;

	/* 발사 판정 방식(즉시 판정/비행) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire")
	EJY_WeaponFireMode FireMode = EJY_WeaponFireMode::Hitscan;

	/* Ballistic 방식 비행 설정 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire", meta = (EditCondition = "FireMode == EJY_WeaponFireMode::Ballistic", EditConditionHides))
	FJY_BallisticSettings BallisticSettings;

	/* 총구 화염 나이아가라 시스템. `User.Trigger`(bool)·`User.Direction`(vector) 노출 필요 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire|Effects")
	TObjectPtr<UNiagaraSystem> MuzzleFlashSystem;

	/* 탄 궤적 나이아가라 시스템. `User.Trigger`(bool)·`User.ImpactPositions`(vector 배열) 노출 필요 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire|Effects")
	TObjectPtr<UNiagaraSystem> TracerSystem;

	/* 탄피 배출 나이아가라 시스템. `User.Trigger`(bool)·`User.ShellEjectStaticMesh`(static mesh) 노출 필요 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire|Effects")
	TObjectPtr<UNiagaraSystem> ShellEjectSystem;

	/* 배출 탄피 메시. 위 탄피 시스템의 `User.ShellEjectStaticMesh`로 전달 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire|Effects")
	TObjectPtr<UStaticMesh> ShellEjectMesh;

	/* 발사음. 총구 소켓에서 직접 재생 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire|Effects")
	TObjectPtr<USoundBase> FireSound;

	/* 히트스캔 최대 검사 거리 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire", meta = (ClampMin = "1.0", Units = "cm"))
	float MaxTraceRange = 10000.f;

	/* 기본 데미지 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire", meta = (ClampMin = "0.0"))
	float BaseDamage = 20.f;

	/* 총알 충돌 검사 구체 반지름 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire", meta = (ClampMin = "0.1", Units = "cm"))
	float BulletTraceRadius = 1.f;

	/* 확산 집중도(값이 클수록 중심에 모임) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire|Spread", meta = (ClampMin = "0.1"))
	float SpreadExponent = 0.8f;

	/* 열 -> 확산 각도 커브 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire|Spread")
	FRuntimeFloatCurve HeatToSpreadCurve;

	/* 열 -> 한 발당 추가 열 커브 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire|Spread")
	FRuntimeFloatCurve HeatToHeatPerShotCurve;

	/* 열 -> 초당 냉각량 커브 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire|Spread")
	FRuntimeFloatCurve HeatToCoolDownPerSecondCurve;

	/* 마지막 발사 후 확산 회복 시작까지 대기 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire|Spread", meta = (ClampMin = "0.0", Units = "s"))
	float SpreadRecoveryCooldownDelay = 0.15f;

	/* 확산이 완전히 회복되면 다음 한 발의 무작위 확산을 없앤다 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire|Spread")
	bool bAllowFirstShotAccuracy = true;

	/* 총 관통력 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire|Penetration", meta = (ClampMin = "0.0"))
	float PenetrationPower = 10.f;

	/* 재질별 관통 비용(기본값은 생성자에서 설정) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Weapon|Fire|Penetration")
	TMap<TEnumAsByte<EPhysicalSurface>, float> PenetrationCostBySurface;

};
