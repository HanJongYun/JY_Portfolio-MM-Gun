#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "JY_PawnData.generated.h"

class UInputMappingContext;
class UJY_InputConfig;
class UJY_AbilitySet;

USTRUCT(BlueprintType)
struct FJYInputMappingContextAndPriority
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<const UInputMappingContext> InputMapping = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	int32 Priority = 0;
};

UCLASS(BlueprintType, Const)
class UJY_PawnData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<const UJY_AbilitySet> AbilitySet = nullptr;

	/* IA/InputTag 매핑표 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<const UJY_InputConfig> InputConfig = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", Meta = (TitleProperty = "InputMapping"))
	TArray<FJYInputMappingContextAndPriority> DefaultInputMappings;
};
