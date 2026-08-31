#include "Components/JY_WeaponStateComponent.h"

#include "Actors/JY_Character.h"
#include "Components/JY_EquipmentComponent.h"
#include "Equipment/JY_WeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_WeaponStateComponent)

UJY_WeaponStateComponent::UJY_WeaponStateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UJY_WeaponStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) //override
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	/* [Server / Owning Client] PlayerController가 가진 Pawn의 활성 무기만 갱신 */
	AJY_Character* JYCharacter = GetPawn<AJY_Character>();
	UJY_EquipmentComponent* EquipmentComp = JYCharacter != nullptr ? JYCharacter->GetEquipmentComponent() : nullptr;
	UJY_WeaponInstance* WeaponInstance = EquipmentComp != nullptr ? Cast<UJY_WeaponInstance>(EquipmentComp->GetActiveEquipmentInstance()) : nullptr;
	if (WeaponInstance == nullptr)
		return;

	WeaponInstance->Tick(DeltaTime);
}
