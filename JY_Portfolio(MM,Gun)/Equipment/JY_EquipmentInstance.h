#pragma once

#include "CoreMinimal.h"
#include "JY_EquipmentInstance.generated.h"

class AActor;
class APawn;
class UJY_EquipmentComponent;

UCLASS(BlueprintType, Blueprintable)
class UJY_EquipmentInstance : public UObject
{
	GENERATED_BODY()

public:
	UJY_EquipmentInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:

	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual UWorld* GetWorld() const override final;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "JY|Equipment")
	UObject* GetInstigator() const { return Instigator; }

	void SetInstigator(UObject* InInstigator) { Instigator = InInstigator; }

	UFUNCTION(BlueprintPure, Category = "JY|Equipment")
	APawn* GetPawn() const;

	/* 요청 Pawn 클래스와 일치할 때만 반환 */
	UFUNCTION(BlueprintPure, Category = "JY|Equipment", meta = (DeterminesOutputType = PawnType))
	APawn* GetTypedPawn(TSubclassOf<APawn> PawnType) const;

	UFUNCTION(BlueprintPure, Category = "JY|Equipment")
	TArray<AActor*> GetSpawnedActors() const { return SpawnedActors; }

	void AddSpawnedActor(AActor* InActor);

	virtual void DestroyEquipmentActors();

	virtual void OnEquipped();

	virtual void OnUnequipped();

protected:

	UFUNCTION(BlueprintImplementableEvent, Category = "JY|Equipment", meta = (DisplayName = "OnEquipped"))
	void K2_OnEquipped();

	UFUNCTION(BlueprintImplementableEvent, Category = "JY|Equipment", meta = (DisplayName = "OnUnequipped"))
	void K2_OnUnequipped();

private:

	UFUNCTION()
	void OnRep_Instigator();

private:

	UPROPERTY(ReplicatedUsing = OnRep_Instigator)
	TObjectPtr<UObject> Instigator;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<AActor>> SpawnedActors;

	friend class UJY_EquipmentComponent;
};
