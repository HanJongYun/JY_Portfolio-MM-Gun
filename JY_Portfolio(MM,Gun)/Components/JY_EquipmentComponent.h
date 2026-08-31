#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/JY_AbilitySet.h"
#include "Core/JY_Types.h"
#include "JY_EquipmentComponent.generated.h"

class UJY_WeaponData;
class UJY_WeaponGlobalData;
class AJY_WeaponActor;
class UJY_AbilitySystemComponent;
class UJY_EquipmentInstance;
class UAnimSequenceBase;
class UAimOffsetBlendSpace;
class ACharacter;
class UActorChannel;
class FOutBunch;
struct FReplicationFlags;

UCLASS(ClassGroup = (JY), meta = (BlueprintSpawnableComponent))
class UJY_EquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UJY_EquipmentComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void UninitializeComponent() override;

public:

	/* 무기 스폰 + 수납 부착 */
	int32 AddWeapon(EJY_WeaponAnimType Type);

	UFUNCTION(BlueprintCallable, Category = "JY|Equipment")
	void SetActiveSlot(int32 NewIndex);

	UFUNCTION(BlueprintCallable, Category = "JY|Equipment")
	void RequestActiveSlot(int32 Index);

	UFUNCTION(Server, Reliable)
	void Server_SetActiveSlot(int32 Index);

	int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	UFUNCTION(BlueprintPure, Category = "JY|Equipment")
	UJY_EquipmentInstance* GetActiveEquipmentInstance() const { return ActiveEquipmentInstance; }

	/* 활성 무기 애니메이션 종류, 없으면 Unarmed */
	EJY_WeaponAnimType GetActiveWeaponAnimType() const;

	EJY_WeaponAnimType GetEquippedWeaponType() const { return EquippedWeaponType; }

	UFUNCTION(BlueprintPure, Category = "JY|Equipment")
	bool IsWeaponEquipped() const { return EquippedWeaponType != EJY_WeaponAnimType::Unarmed; }

	EJY_WeaponAnimType GetOverlayType() const { return OverlayType; }

	UFUNCTION(BlueprintPure, Category = "JY|Weapon")
	UAnimSequenceBase* GetActiveOverlayPose(EJY_OverlayPoseType PoseType) const;

	UFUNCTION(BlueprintPure, Category = "JY|Weapon")
	UAimOffsetBlendSpace* GetActiveAimOffset() const;

	/* WeaponSwap 노티파이 콜백, 부착 스왑 */
	void OnWeaponSwapNotify();

	void SetOverlayType(EJY_WeaponAnimType Type) { OverlayType = Type; }
	AJY_WeaponActor* GetActiveWeapon() const;
	bool GetLeftHandIKTargetWorld(FTransform& OutTransform) const;

protected:

	/* 무기 목록 복제, 부착 갱신 */
	UFUNCTION()
	void OnRep_Weapons();

	/* 활성 슬롯 복제, 몽타주 재생 */
	UFUNCTION()
	void OnRep_ActiveSlot(int32 OldActiveSlotIndex);

	/* 장비 객체 복제, 장착/해제 이벤트 */ 
	UFUNCTION()
	void OnRep_ActiveEquipmentInstance(UJY_EquipmentInstance* OldInstance);

protected:

	/* 시작 슬롯 무기 종류 목록 */
	UPROPERTY(EditAnywhere, Category = "JY|Equipment")
	TArray<EJY_WeaponAnimType> DefaultSlots;

	/* 스폰된 무기 액터 목록 */
	UPROPERTY(ReplicatedUsing = OnRep_Weapons)
	TArray<TObjectPtr<AJY_WeaponActor>> SpawnedWeapons;

	/* 활성 슬롯, -1 없음 */
	UPROPERTY(ReplicatedUsing = OnRep_ActiveSlot)
	int32 ActiveSlotIndex = -1;

	/* 현재 장비 객체 */
	UPROPERTY(ReplicatedUsing = OnRep_ActiveEquipmentInstance)
	TObjectPtr<UJY_EquipmentInstance> ActiveEquipmentInstance;

	/* 실제 장착 중 무기 종류 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "JY|Weapon", meta = (AllowPrivateAccess = "true"))
	EJY_WeaponAnimType EquippedWeaponType = EJY_WeaponAnimType::Unarmed;

	/* 오버레이 표시 무기 종류 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "JY|Weapon", meta = (AllowPrivateAccess = "true"))
	EJY_WeaponAnimType OverlayType = EJY_WeaponAnimType::Unarmed;

private:

	/* 장비 객체 생성/등록 */
	UJY_EquipmentInstance* EquipItem(AJY_WeaponActor* Weapon);
	void UnequipItem(UJY_EquipmentInstance* ItemInstance);
	/* equip/unequip 몽타주 재생 */
	void PlayEquipTransition(int32 OldIndex, int32 NewIndex);
	void RefreshAttachments();

	void AttachWeaponToSocket(AJY_WeaponActor* Weapon, FName SocketName, const FTransform& RelativeTransform);

	/* AbilitySet 부여 */
	void GrantWeaponAbilities();

	UJY_AbilitySystemComponent* GetASC() const;
	ACharacter* GetOwnerCharacter() const;

private:
	FJYAbilitySet_GrantedHandles ActiveGrantedHandles;

	UPROPERTY(Transient)
	TObjectPtr<UJY_WeaponGlobalData> CachedWeaponGlobalData;
};
