// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/JY_Types.h"
#include "JY_TraversalComponent.generated.h"

class AJY_Character;

UCLASS(Blueprintable, BlueprintType, ClassGroup=(JY), meta=(BlueprintSpawnableComponent))
class UJY_TraversalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_TraversalComponent();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "JY|Traversal")
	bool GetLedgeTransforms_BP(AActor* HitActor, FVector HitLocation, FVector ActorLocation, FJY_TraversalCheckResult& OutResult);

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category="JY|Traversal", meta=(AllowPrivateAccess="true"))
	TObjectPtr<AJY_Character> OwnerCharacter;
};
