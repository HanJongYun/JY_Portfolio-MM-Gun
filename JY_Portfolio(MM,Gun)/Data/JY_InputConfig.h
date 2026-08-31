#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "JY_InputConfig.generated.h"

class UInputAction;

USTRUCT(BlueprintType)
struct FJYInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

UCLASS(BlueprintType, Const)
class UJY_InputConfig : public UDataAsset
{
	GENERATED_BODY()

public:

	UJY_InputConfig(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;

public:

	/* C++ 함수 직접 바인딩 입력(이동/시점), BindNativeAction 처리 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FJYInputAction> NativeInputActions;

	/* GAS 어빌리티로 전달되는 입력, BindAbilityActions->ASC 경유 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FJYInputAction> AbilityInputActions;
};
