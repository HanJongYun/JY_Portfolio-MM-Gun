#include "Equipment/JY_EquipmentInstance.h"

#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_EquipmentInstance)

UJY_EquipmentInstance::UJY_EquipmentInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UWorld* UJY_EquipmentInstance::GetWorld() const //override
{
	APawn* OwningPawn = GetPawn();
	return OwningPawn != nullptr ? OwningPawn->GetWorld() : nullptr;
}

void UJY_EquipmentInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //override
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Instigator);
	DOREPLIFETIME(ThisClass, SpawnedActors);
}

APawn* UJY_EquipmentInstance::GetPawn() const
{
	/* EquipmentInstance의 Outer를 소유 Pawn으로 사용 */
	return Cast<APawn>(GetOuter());
}

APawn* UJY_EquipmentInstance::GetTypedPawn(TSubclassOf<APawn> PawnType) const
{
	UClass* ActualPawnType = PawnType;
	if (ActualPawnType == nullptr || GetOuter() == nullptr || GetOuter()->IsA(ActualPawnType) == false)
		return nullptr;

	return Cast<APawn>(GetOuter());
}

void UJY_EquipmentInstance::AddSpawnedActor(AActor* InActor)
{
	if (InActor != nullptr)
	{
		SpawnedActors.AddUnique(InActor);
	}
}

void UJY_EquipmentInstance::DestroyEquipmentActors()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor != nullptr)
		{
			Actor->Destroy();
		}
	}
}

void UJY_EquipmentInstance::OnEquipped()
{
	K2_OnEquipped();
}

void UJY_EquipmentInstance::OnUnequipped()
{
	K2_OnUnequipped();
}

void UJY_EquipmentInstance::OnRep_Instigator()
{
}
